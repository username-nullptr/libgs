
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H

namespace libgs::http_nt { namespace detail
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

template <concepts::connection_pool ConnectionPool, version_enum Version>
class LIBGS_HTTP_NT_TAPI basic_client<ConnectionPool,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using socket_t = connection_t::socket_t;

	template <method_enum Method>
	using ctx_expected_t = sys_expected<context_ptr<Method>>;

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_pool(io_context()) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec) :
		m_pool(exec) {}

	explicit impl(connection_pool_t &&pool) :
		m_pool(std::move(pool)) {}

public:
	template <method_enum Method>
	[[nodiscard]] ctx_expected_t<Method> request(req_info info) noexcept
	{
		bool continue_100 = false;
		if constexpr( version_v > version::v10 )
		{
			auto it = info.arg.headers().find(header::expect);
			continue_100 = it != info.arg.headers().end() and
				strtls::to_lower(*it->second) == "100-continue";
		}
		ctx_expected_t<Method> ctx_expected {
			sys_unexpected(make_error_code(std::errc::connection_aborted))
		};
		for(size_t i=0; i<10; i++)
		{
			ctx_expected = make_context<Method>(std::move(info));
			if( not ctx_expected )
				return ctx_expected;

			if( (*ctx_expected)->connection().peek() )
				break;
		}
		if( not (*ctx_expected)->connection().peek() )
			return sys_unexpected((*ctx_expected)->reply()->first_error());
		{
			auto expected = (*ctx_expected)->write();
			if( not expected )
				return sys_unexpected(expected.error());

			else if( not continue_100 )
				return ctx_expected;
		}
		auto expected = (*ctx_expected)->wait_reply();
		if( not expected )
			ctx_expected.despair(expected.error());
		return ctx_expected;
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_request(req_info info,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<ctx_expected_t<Method>>
		{
			bool continue_100 = false;
			if constexpr( version_v > version::v10 )
			{
				auto it = info.arg.headers().find(header::expect);
				continue_100 = it != info.arg.headers().end() and
					strtls::to_lower(*it->second) == "100-continue";
			}
			ctx_expected_t<Method> ctx_expected {
				sys_unexpected(make_error_code(std::errc::connection_aborted))
			};
			for(size_t i=0; i<10; i++)
			{
				ctx_expected = co_await co_make_context<Method>(
					std::move(info), cancel_slot, 0ns
				);
				if( not ctx_expected )
					co_return ctx_expected;

				else if( (*ctx_expected)->connection().peek() )
					break;
			}
			if( not (*ctx_expected)->connection().peek() )
				co_return sys_unexpected((*ctx_expected)->reply()->first_error());
			{
				auto expected = (*ctx_expected)->write();
				if( not expected )
					co_return sys_unexpected(expected.error());

				else if( not continue_100 )
					co_return ctx_expected;
			}
			auto expected = (*ctx_expected)->wait_reply();
			if( not expected )
				ctx_expected.despair(expected.error());
			co_return ctx_expected;
		},
		use_awaitable);

		ctx_expected_t<Method> expected {
			sys_unexpected(make_error_code(std::errc::connection_aborted))
		};
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::move(std::get<0>(var));
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_request(std::error_code &error,
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
	[[nodiscard]] sys_expected<context_t<method::put>> upload_file
	(req_info info, auto &&opt, auto &&progress) noexcept
	{
		auto pair = info.arg.set_header(opt);
		if( not pair )
			return sys_unexpected(pair.error());

		return request<method::put>(std::move(info))
		.and_then([&](const auto &context) -> sys_expected<context_t<method::put>>
		{
			if constexpr( version_v > version::v10 )
			{
				if( context->responded() and
					context->reply()->status() != status::continue_upload )
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

	[[nodiscard]] awaitable<sys_expected<context_t<method::put>>> co_upload_file(
		req_info info, auto &&opt, auto &&progress, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto pair = info.arg.set_header(opt);
		if( not pair )
			co_return sys_unexpected(pair.error());

		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<sys_expected<context_t<method::put>>>
		{
			context_t<method::put> context;
			{
				auto expected = co_await co_request<method::put>(
					std::move(info), cancel_slot, 0ns
				);
				if( not expected )
					co_return expected;
				context = std::move(*expected);
			}
			if constexpr( version_v > version::v10 )
			{
				if( context->responded() and
					context->reply()->status() != status::continue_upload )
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

		sys_expected<context_t<method::put>> expected;
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

	[[nodiscard]] awaitable<sys_expected<context_t<method::put>>> co_upload_file(
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
	template <method_enum Method>
	[[nodiscard]] ctx_expected_t<Method> make_context(req_info info) noexcept
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

		return std::make_shared<context_t<Method>>(
			std::move(*expected), std::move(info.url), std::move(info.arg)
		);
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_make_context(req_info info,
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
		[&]() mutable -> awaitable<ctx_expected_t<Method>>
		{
			std::error_code error;
			auto make_task = [&](const endpoint_t &ep)
			{
				return libgs::dispatch(m_pool.get_executor(),
				[&, ep]() -> awaitable<std::shared_ptr<connection_t>>
				{
					auto expected = co_await m_pool.get(ep, use_awaitable | cancel_slot);
					if( expected )
						co_return std::make_shared<connection_t>(std::move(*expected));

					else if( expected.error() == errc::operation_aborted )
						error = expected.error();

					system_error::loc_throw(error);
					co_return nullptr;
				},
				deferred);
			};
			using task_t = decltype(make_task({}));
			std::vector<task_t> tasks {};

			auto addr = asio::ip::make_address(info.url.address().data(), error);
			if( not error )
			{
				tasks.emplace_back (
					make_task({addr, info.url.port()})
				);
			}
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
				for(auto &res : results)
				{
					tasks.emplace_back (
						make_task({res.endpoint().address(), info.url.port()})
					);
				}
			}
			error = make_error_code(errc::connection_refused);
			auto [order, exs, values] = co_await asio::experimental::make_parallel_group(std::move(tasks))
				.async_wait(asio::experimental::wait_for_one_success(), deferred);

			for(size_t i=0; i<exs.size(); i++)
			{
				if( exs[i] )
					continue;

				co_return std::make_shared<context_t<Method>>(
					std::move(*values[i]), std::move(info.url), std::move(info.arg)
				);
			}
			co_return sys_unexpected(error);
		},
		use_awaitable);

		using namespace std::chrono_literals;
		ctx_expected_t<Method> expected {
			sys_unexpected(make_error_code(std::errc::connection_refused))
		};
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::move(std::get<0>(var));
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_make_context(std::error_code &error,
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
	connection_pool_t m_pool;
};

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(connection_pool_t &&pool) :
	m_impl(std::make_shared<impl>(std::move(pool)))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(basic_client &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>&
basic_client<ConnectionPool,Version>::operator=(basic_client &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::~basic_client() = default;

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <method_enum Method, typename Token>
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
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->template co_request<Method>(no_time_token.ec_, std::move(info),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_request<Method>(std::move(info),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(),
					m_impl->template co_request<Method>(no_time_token.ec_, std::move(info),
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
			else
			{
				return libgs::dispatch(get_executor(),
					m_impl->template co_request<Method>(std::move(info),
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					no_time_token, info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_request<Method> (
						no_time_token.ec_, std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
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
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), no_time_token, original_token,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					no_time_token.ec_, std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), original_token,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
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

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename T, typename Token>
auto basic_client<ConnectionPool,Version>::upload_file(req_info info, T &&opt, Token &&token)
	noexcept requires file_opt_token_v<T,Token>
{
	return upload_file(std::move(info),
		std::forward<T>(opt), [](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
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
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_upload_file(no_time_token.ec_, std::move(info),
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_upload_file(std::move(info),
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(),
					m_impl->co_upload_file(no_time_token.ec_, std::move(info),
						std::forward<T>(opt), std::forward<Progress>(progress),
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
			else
			{
				return libgs::dispatch(get_executor(),
					m_impl->co_upload_file(std::move(info),
						std::forward<T>(opt), std::forward<Progress>(progress),
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					no_time_token, promise = std::move(promise), info = std::move(info),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_upload_file(no_time_token.ec_,
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
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
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
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				no_time_token, original_token, info = std::move(info), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file(no_time_token.ec_,
					std::move(info), std::move(opt), std::move(progress),
					cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				original_token, info = std::move(info), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file (
					std::move(info), std::move(opt), std::move(progress),
					cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
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

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_get(req_info info, Token &&token)
	noexcept requires request_token_v<method::get,Token>
{
	return request<method::get>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_put(req_info info, Token &&token)
	noexcept requires request_token_v<method::put,Token>
{
	return request<method::put>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_post(req_info info, Token &&token)
	noexcept requires request_token_v<method::post,Token>
{
	return request<method::post>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_head(req_info info, Token &&token)
	noexcept requires request_token_v<method::head,Token>
{
	return request<method::head>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_patch(req_info info, Token &&token)
	noexcept requires request_token_v<method::patch,Token>
{
	return request<method::patch>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_delete(req_info info, Token &&token)
	noexcept requires request_token_v<method::delet,Token>
{
	return request<method::delet>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_options(req_info info, Token &&token)
	noexcept requires request_token_v<method::options,Token>
{
	return request<method::options>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_trace(req_info info, Token &&token)
	noexcept requires request_token_v<method::trace,Token>
{
	return request<method::trace>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::request_connect(req_info info, Token &&token)
	noexcept requires request_token_v<method::connect,Token>
{
	return request<method::connect>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <method_enum Method, typename Token>
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
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->template co_make_context<Method>(
					no_time_token.ec_, std::move(info), asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_make_context<Method>(
					std::move(info), asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->template co_make_context<Method>(
					no_time_token.ec_, std::move(info), asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->template co_make_context<Method>(
					std::move(info), asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					no_time_token, info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_make_context<Method> (
						no_time_token.ec_, std::move(info), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					info = std::move(info), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
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
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), no_time_token, original_token,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_make_context<Method> (
					no_time_token.ec_, std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(), original_token,
				info = std::move(info), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_make_context<Method> (
					std::move(info), cancel_slot, timeout
				);
				expected
				.transform([&callback = original_token](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = original_token](const error_code &error) {
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

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_get(req_info info, Token &&token)
	noexcept requires request_token_v<method::get,Token>
{
	return make_context<method::get>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_put(req_info info, Token &&token)
	noexcept requires request_token_v<method::put,Token>
{
	return make_context<method::put>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_post(req_info info, Token &&token)
	noexcept requires request_token_v<method::post,Token>
{
	return make_context<method::post>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_head(req_info info, Token &&token)
	noexcept requires request_token_v<method::head,Token>
{
	return make_context<method::head>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_patch(req_info info, Token &&token)
	noexcept requires request_token_v<method::patch,Token>
{
	return make_context<method::patch>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_delete(req_info info, Token &&token)
	noexcept requires request_token_v<method::delet,Token>
{
	return make_context<method::delet>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_options(req_info info, Token &&token)
	noexcept requires request_token_v<method::options,Token>
{
	return make_context<method::options>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_trace(req_info info, Token &&token)
	noexcept requires request_token_v<method::trace,Token>
{
	return make_context<method::trace>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <typename Token>
auto basic_client<ConnectionPool,Version>::make_connect(req_info info, Token &&token)
	noexcept requires request_token_v<method::connect,Token>
{
	return make_context<method::connect>(
		std::move(info), std::forward<Token>(token)
	);
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
consteval version_enum basic_client<ConnectionPool,Version>::version() noexcept
{
	return version_v;
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::executor_t
basic_client<ConnectionPool,Version>::get_executor() noexcept
{
	return m_impl->m_pool.get_executor();
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H