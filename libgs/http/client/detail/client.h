
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

namespace libgs::http
{

template <concepts::session_pool SessionPool, protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_client<SessionPool,Version>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_pool(io_context()) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec) :
		m_pool(exec) {}

	explicit impl(session_pool_t &&pool) :
		m_pool(std::move(pool)) {}

public:
	template <protocol::method_enum Method>
	[[nodiscard]] sys_expected<request_ptr<Method>> request(url_t url, request_arg_t arg) noexcept
	{
		if( strtls::to_lower(url.protocol()) != "http" )
		{
			return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = session_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		endpoint_t ep;
		std::error_code error;

		auto addr = asio::ip::make_address(url.address().data(), error);
		if( not error )
			ep = {addr, url.port()};
		else
		{
			using resolver_t = asio::ip::basic_resolver<protocol_t>;
			resolver_t resolver(m_pool.get_executor());

			auto results = resolver.resolve(url.address(), "", error);
			if( error )
				return sys_unexpected(error);

			else if( results.empty() )
			{
				return sys_unexpected (
					make_error_code(std::errc::host_unreachable)
				);
			}
			ep = {results.begin()->endpoint().address(), url.port()};
		}
		auto expected = m_pool.get(ep);
		if( not expected )
			return sys_unexpected(expected.error());

		return std::make_shared<request_t<Method>>(
			std::move(*expected), std::move(url), std::move(arg)
		);
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<request_ptr<Method>>> co_request(url_t url, request_arg_t arg,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( strtls::to_lower(url.protocol()) != "http" )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = session_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		using namespace libgs::operators;
		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<sys_expected<request_ptr<Method>>>
		{
			endpoint_t ep;
			std::error_code error;

			auto addr = asio::ip::make_address(url.address().data(), error);
			if( not error )
				ep = {addr, url.port()};
			else
			{
				using resolver_t = asio::ip::basic_resolver<protocol_t>;
				resolver_t resolver(m_pool.get_executor());

				auto results = co_await resolver.async_resolve (
					url.address(), "", use_awaitable | cancel_slot | error
				);
				if( error )
					co_return sys_unexpected(error);

				else if( results.empty() )
				{
					co_return sys_unexpected (
						make_error_code(std::errc::host_unreachable)
					);
				}
				ep = {results.begin()->endpoint().address(), url.port()};
			}
			auto expected = co_await m_pool.get(ep, use_awaitable | cancel_slot);
			if( not expected )
				co_return sys_unexpected(expected.error());

			co_return std::make_shared<request_t<Method>>(
				std::move(*expected), std::move(url), std::move(arg)
			);
		},
		use_awaitable);

		using namespace std::chrono_literals;
		sys_expected<request_ptr<Method>> expected;

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
	[[nodiscard]] awaitable<sys_expected<request_ptr<Method>>> co_request(
		std::error_code &error, url_t url, request_arg_t arg,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_request(std::move(url), std::move(arg),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	template <protocol::method_enum Method>
	[[nodiscard]] sys_expected<reply_ptr> reply(const request_ptr<Method> &request) noexcept
	{
		auto expected = reply_t::make(std::move(request->session()));
		if( not expected or request->version() < protocol::version::v11 )
			return expected;

		else if( auto &reply = *expected; reply->status() == protocol::status::continue_upload )
			request->session() = std::move(reply->session());
		return expected;
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<reply_ptr>> co_reply(const request_ptr<Method> &request,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		auto expected = co_await reply_t::make(std::move(request->session()),
			use_awaitable | cancel_slot | timeout
		);
		if( not expected or request->version() < protocol::version::v11 )
			co_return expected;

		else if( auto &reply = *expected; reply->status() == protocol::status::continue_upload )
			request->session() = std::move(reply->session());
		co_return expected;
	}

	template <protocol::method_enum Method>
	[[nodiscard]] awaitable<sys_expected<reply_ptr>> co_reply(
		std::error_code &error, const request_ptr<Method> &request,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_reply(request,
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	session_pool_t m_pool;
};

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::basic_client(core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::basic_client(session_pool_t &&pool) :
	m_impl(std::make_shared<impl>(std::move(pool)))
{

}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::basic_client(basic_client &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>&
basic_client<SessionPool,Version>::operator=(basic_client &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::~basic_client() = default;

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <protocol::method_enum Method, typename Token>
auto basic_client<SessionPool,Version>::request(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->template request<Method>(std::move(url), std::move(arg))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return m_impl->template request<Method>(
			std::move(url), std::move(arg));

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
				return m_impl->template co_request<Method>(
					ntoken.ec_, std::move(url), std::move(arg),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_request<Method>(
					std::move(url), std::move(arg),
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
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, url = std::move(url), arg = std::move(arg), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_request<Method> (
						ntoken.ec_, std::move(url), std::move(arg), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
					url = std::move(url), arg = std::move(arg), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_request<Method> (
						std::move(url), std::move(arg), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken,
				url = std::move(url), arg = std::move(arg), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					ntoken.ec_, std::move(url), std::move(arg), cancel_slot, timeout
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
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken,
				url = std::move(url), arg = std::move(arg), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_request<Method> (
					std::move(url), std::move(arg), cancel_slot, timeout
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
		return request<Method>(std::move(url), token | 0ns);
	}
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <protocol::method_enum Method, typename Token>
auto basic_client<SessionPool,Version>::request(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<Method>(std::move(url), {}, std::forward<Token>(token));
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <protocol::method_enum Method, typename Token>
auto basic_client<SessionPool,Version>::reply(const request_ptr<Method> &request, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->reply(request)
			.or_else([&token](const std::error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->reply(request);

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
				return m_impl->template co_reply<Method>(ntoken.ec_, request,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->template co_reply<Method>(request,
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
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, request, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_reply<Method> (
						ntoken.ec_, request, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), request, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->template co_reply<Method> (
						request, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken,
				request, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_reply<Method> (
					ntoken.ec_, request, cancel_slot, timeout
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
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken,
				request, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->template co_reply<Method> (
					request, cancel_slot, timeout
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
		return reply<Method>(request, token | 0ns);
	}
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_get(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::get>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_put(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::put>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_post(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::post>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_head(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::head>(
		std::move(url), std::move(arg), std::forward<Token>(token)
		);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_patch(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::patch>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_delete(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::delet>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_options(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::options>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_trace(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::trace>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_connect(url_t url, request_arg_t arg, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::connect>(
		std::move(url), std::move(arg), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_get(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::get>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_put(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::put>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_post(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::post>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_head(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::head>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_patch(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::patch>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_delete(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::delet>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_options(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::options>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_trace(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::trace>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
template <typename Token>
auto basic_client<SessionPool,Version>::req_connect(url_t url, Token &&token)
	noexcept requires request_token_v<Token>
{
	return request<protocol::method_enum::connect>(
		std::move(url), std::forward<Token>(token)
	);
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
consteval protocol::version_enum basic_client<SessionPool,Version>::version() noexcept
{
	return version_v;
}

template <concepts::session_pool SessionPool, protocol::version_enum Version>
basic_client<SessionPool,Version>::executor_t
basic_client<SessionPool,Version>::get_executor() noexcept
{
	return m_impl->m_pool.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
