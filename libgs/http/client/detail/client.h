
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H

namespace libgs::http { namespace detail
{

template <concepts::stream>
struct protocol_name {
	static constexpr auto name = "http";
};

#if LIBGS_OPENSSL_SUPPORT
template <core_concepts::exec Exec>
struct protocol_name
<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> {
	static constexpr auto name = "https";
};
#endif //LIBGS_OPENSSL_SUPPORT

template <concepts::stream Stream>
constexpr auto protocol_name_v = protocol_name<Stream>::name;

} //namespace detail

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_client<ConnectionPool,Version>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)
	using socket_t = connection_t::socket_t;

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_pool(io_context()) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec) :
		m_pool(exec) {}

	explicit impl(connection_pool_t &&pool) :
		m_pool(std::move(pool)) {}

public:
	template <protocol::method_enum Method>
	[[nodiscard]] sys_expected<context_ptr<Method>> make_context(req_info info) noexcept
	{
		if( strtls::to_lower(info.url.protocol()) != detail::protocol_name_v<socket_t> )
		{
			return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = connection_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		endpoint_t ep;
		std::error_code error;

		auto addr = asio::ip::make_address(info.url.address().data(), error);
		if( not error )
			ep = {addr, info.url.port()};
		else
		{
			using resolver_t = asio::ip::basic_resolver<protocol_t>;
			resolver_t resolver(m_pool.get_executor());

			auto results = resolver.resolve(info.url.address(), "", error);
			if( error )
				return sys_unexpected(error);

			else if( results.empty() )
			{
				return sys_unexpected (
					make_error_code(std::errc::host_unreachable)
				);
			}
			ep = {results.begin()->endpoint().address(), info.url.port()};
		}
		auto expected = m_pool.get(ep);
		if( not expected )
			return sys_unexpected(expected.error());

		return std::make_shared<context_t<Method>>(request_t<Method>(
			std::move(*expected), std::move(info.url), std::move(info.arg)
		));
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<context_ptr<Method>>> co_make_context(req_info info,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( strtls::to_lower(info.url.protocol()) != detail::protocol_name_v<socket_t> )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = connection_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		using namespace libgs::operators;
		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<sys_expected<context_ptr<Method>>>
		{
			endpoint_t ep;
			std::error_code error;

			auto addr = asio::ip::make_address(info.url.address().data(), error);
			if( not error )
				ep = {addr, info.url.port()};
			else
			{
				using resolver_t = asio::ip::basic_resolver<protocol_t>;
				resolver_t resolver(m_pool.get_executor());

				auto results = co_await resolver.async_resolve (
					info.url.address(), "", use_awaitable | cancel_slot | error
				);
				if( error )
					co_return sys_unexpected(error);

				else if( results.empty() )
				{
					co_return sys_unexpected (
						make_error_code(std::errc::host_unreachable)
					);
				}
				ep = {results.begin()->endpoint().address(), info.url.port()};
			}
			auto expected = co_await m_pool.get(ep, use_awaitable | cancel_slot);
			if( not expected )
				co_return sys_unexpected(expected.error());

			co_return std::make_shared<context_t<Method>>(request_t<Method>(
				std::move(*expected), std::move(info.url), std::move(info.arg)
			));
		},
		use_awaitable);

		using namespace std::chrono_literals;
		sys_expected<context_ptr<Method>> expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<context_ptr<Method>>> co_make_context(std::error_code &error,
		req_info info, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_make_context<Method>(std::move(info),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	template <protocol::method_enum Method>
	[[nodiscard]] sys_expected<context_ptr<Method>> request(req_info info) noexcept
	{
		bool continue_100 = false;
		context_ptr<Method> context;
		{
			if constexpr( version_v > protocol::version::v10 )
			{
				auto it = info.arg.headers().find(protocol::header::expect);
				continue_100 = it != info.arg.headers().end() and
					strtls::to_lower(*it->second) == "100-continue";
			}
			auto expected = make_context<Method>(std::move(info));
			if( not expected )
				return expected;
			context = std::move(*expected);
		}{
			auto expected = context->request().write();
			if( not expected )
				return sys_unexpected(expected.error());
			else if( not continue_100 )
				return context;
		}
		auto expected = context->wait_reply();
		if( not expected )
			return sys_unexpected(expected.error());
		return context;
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<context_ptr<Method>>> co_request(req_info info,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<sys_expected<context_ptr<Method>>>
		{
			bool continue_100 = false;
			context_ptr<Method> context;
			{
				if constexpr( version_v > protocol::version::v10 )
				{
					auto it = info.arg.headers().find(protocol::header::expect);
					continue_100 = it != info.arg.headers().end() and
						strtls::to_lower(*it->second) == "100-continue";
				}
				auto expected = co_await co_make_context<Method>(
					std::move(info), cancel_slot, 0ns
				);
				if( not expected )
					co_return expected;
				context = std::move(*expected);
			}{
				auto expected = co_await context->request().write(use_awaitable);
				if( not expected )
					co_return sys_unexpected(expected.error());
				else if( not continue_100 )
					co_return context;
			}
			auto expected = context->wait_reply();
			if( not expected )
				co_return sys_unexpected(expected.error());
			co_return context;
		},
		use_awaitable);

		sys_expected<context_ptr<Method>> expected;
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<context_ptr<Method>>> co_request(std::error_code &error,
		req_info info, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_request<Method>(std::move(info),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] sys_expected<context_ptr<protocol::method::put>> upload_file
	(req_info info, auto &&opt, auto &&progress) noexcept
	{
		auto pair = info.arg.set_header(opt);
		if( not pair )
			return sys_unexpected(pair.error());

		return request<protocol::method::put>(std::move(info))
		.and_then([&](const auto &context) -> sys_expected<context_ptr<protocol::method::put>>
		{
			if constexpr( version_v > protocol::version::v10 )
			{
				if( context->responded() and
					context->reply().status() != protocol::status::continue_upload )
					return context;
			}
			else
			{
				if( context->responded() )
					return context;
			}
			return context->request().upload_file (
				std::move(pair->first), std::move(pair->second),
				std::forward<decltype(progress)>(progress)
			)
			.and_then([&]
			{
				context->wait_reply();
				return context;
			});
		});
	}

	[[nodiscard]] awaitable<sys_expected<context_ptr<protocol::method::put>>> co_upload_file(
		req_info info, auto &&opt, auto &&progress, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto pair = info.arg.set_header(opt);
		if( not pair )
			co_return sys_unexpected(pair.error());

		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<sys_expected<context_ptr<protocol::method::put>>>
		{
			context_ptr<protocol::method::put> context;
			{
				auto expected = co_await co_request<protocol::method::put>(
					std::move(info), cancel_slot, 0ns
				);
				if( not expected )
					co_return expected;
				context = std::move(*expected);
			}
			if constexpr( version_v > protocol::version::v10 )
			{
				if( context->responded() and
					context->reply().status() != protocol::status::continue_upload )
					co_return context;
			}
			else
			{
				if( context->responded() )
					co_return context;
			}
			{
				auto expected = co_await context->request().upload_file (
					std::move(pair->first), std::move(pair->second),
					std::forward<decltype(progress)>(progress),
					use_awaitable | cancel_slot
				);
				if( not expected )
					co_return sys_unexpected(expected.error());
			}
			auto expected = co_await context->wait_reply(use_awaitable | cancel_slot);
			if( not expected )
				co_return sys_unexpected(expected.error());
			co_return context;
		},
		use_awaitable);

		sys_expected<context_ptr<protocol::method::put>> expected;
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	[[nodiscard]] awaitable<sys_expected<context_ptr<protocol::method::put>>> co_upload_file(
		std::error_code &error, req_info info, auto &&opt, auto &&progress,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_upload_file(std::move(info),
			std::forward<decltype(opt)>(opt), std::forward<decltype(progress)>(progress),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	connection_pool_t m_pool;
};

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(connection_pool_t &&pool) :
	m_impl(std::make_shared<impl>(std::move(pool)))
{

}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(basic_client &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>&
basic_client<ConnectionPool,Version>::operator=(basic_client &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::~basic_client() = default;

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <protocol::method_enum Method, typename Token>
auto basic_client<ConnectionPool,Version>::request(req_info info, Token &&token)
	noexcept requires request_token_v<Method,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->template request<Method>(std::move(info))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->template request<Method>(std::move(info));

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->template co_request<Method>(ntoken.ec_, std::move(info),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_request<Method>(std::move(info),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_request<Method> (
						ntoken.ec_, std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_request<Method> (
						std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), ntoken, nntoken,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					ntoken.ec_, std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), nntoken,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return request<Method>(std::move(info), token | 0ns);
	}
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename T, typename Token>
auto basic_client<ConnectionPool,Version>::upload_file(req_info info, T &&opt, Token &&token)
	noexcept requires file_opt_token_v<T,Token>
{
	return upload_file(std::move(info),
		std::forward<T>(opt), [](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename T, typename Progress, typename Token>
auto basic_client<ConnectionPool,Version>::upload_file(req_info info, T &&opt, Progress &&progress, Token &&token)
	noexcept requires file_opt_token_v<T,Token> and concepts::progress_callback<Progress,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->upload_file(std::move(info),
			std::forward<T>(opt), std::forward<Progress>(progress)
		)
		.or_else([&token](const error_code &error) {
			token = error;
		});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return m_impl->upload_file(std::move(info),
			std::forward<T>(opt), std::forward<Progress>(progress)
		);
	}
	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_upload_file(ntoken.ec_, std::move(info),
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_upload_file(std::move(info),
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, promise = std::move(promise), info = std::move(info),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_upload_file(ntoken.ec_,
						std::move(info), std::move(opt), std::move(progress),
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					promise = std::move(promise), info = std::move(info),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_upload_file (
						std::move(info), std::move(opt), std::move(progress),
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, info = std::move(info), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file(ntoken.ec_,
					std::move(info), std::move(opt), std::move(progress),
					cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, info = std::move(info), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file (
					std::move(info), std::move(opt), std::move(progress),
					cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return upload_file(std::move(info),
			std::forward<T>(opt), std::forward<Progress>(progress),
			token | 0ns
		);
	}
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_get(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::get,Token>
{
	return request<protocol::method::get>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_put(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::put,Token>
{
	return request<protocol::method::put>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_post(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::post,Token>
{
	return request<protocol::method::post>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_head(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::head,Token>
{
	return request<protocol::method::head>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_patch(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::patch,Token>
{
	return request<protocol::method::patch>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_delete(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::delet,Token>
{
	return request<protocol::method::delet>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_options(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::options,Token>
{
	return request<protocol::method::options>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_trace(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::trace,Token>
{
	return request<protocol::method::trace>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_connect(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::connect,Token>
{
	return request<protocol::method::connect>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <protocol::method_enum Method, typename Token>
auto basic_client<ConnectionPool,Version>::make_context(req_info info, Token &&token)
	noexcept requires request_token_v<Method,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->template make_context<Method>(std::move(info))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return m_impl->template make_context<Method>(
			std::move(info));

	}
	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->template co_make_context<Method>(
					ntoken.ec_, std::move(info),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_make_context<Method>(
					std::move(info),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_make_context<Method> (
						ntoken.ec_, std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_make_context<Method> (
						std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), ntoken, nntoken,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_make_context<Method> (
					ntoken.ec_, std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), nntoken,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_make_context<Method> (
					std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return make_context<Method>(std::move(info), token | 0ns);
	}
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_get(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::get,Token>
{
	return make_context<protocol::method::get>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_put(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::put,Token>
{
	return make_context<protocol::method::put>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_post(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::post,Token>
{
	return make_context<protocol::method::post>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_head(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::head,Token>
{
	return make_context<protocol::method::head>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_patch(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::patch,Token>
{
	return make_context<protocol::method::patch>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_delete(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::delet,Token>
{
	return make_context<protocol::method::delet>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_options(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::options,Token>
{
	return make_context<protocol::method::options>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_trace(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::trace,Token>
{
	return make_context<protocol::method::trace>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_connect(req_info info, Token &&token)
	noexcept requires request_token_v<protocol::method::connect,Token>
{
	return make_context<protocol::method::connect>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
consteval protocol::version_enum basic_client<ConnectionPool,Version>::version() noexcept
{
	return version_v;
}

template <concepts::connection_pool ConnectionPool, protocol::version_enum Version>
basic_client<ConnectionPool,Version>::executor_t
basic_client<ConnectionPool,Version>::get_executor() noexcept
{
	return m_impl->m_pool.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H