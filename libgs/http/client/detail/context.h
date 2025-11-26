
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CONTEXT_H
#define LIBGS_HTTP_CLIENT_DETAIL_CONTEXT_H

namespace libgs::http
{

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_request_context<Method,Connection,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	explicit impl(request_t &&request) :
		m_request(std::move(request)), m_reply(std::move(m_request.connection())) {
		m_request.connection() = std::move(m_reply.connection());
	}
	impl(impl&&) noexcept = default;
	impl& operator=(impl&&) noexcept = default;

public:
	[[nodiscard]] sys_expected<protocol::status_enum> wait_reply() noexcept
	{
		if( m_request.is_finished() )
			return m_reply.parse(std::move(m_request.connection()));

		if constexpr( request_t::version() < protocol::version::v11 )
		{
			if( not m_request.is_finished() )
			{
				return sys_unexpected (
					make_error_code(std::errc::device_or_resource_busy)
				);
			}
			return m_reply.parse(std::move(m_request.connection()));
		}
		else
		{
			auto expected = m_reply.parse(std::move(m_request.connection()));
			if( not expected )
				return expected;

			else if( m_reply.status() == protocol::status::continue_upload )
				m_request.connection() = std::move(m_reply.connection());
			return expected;
		}
	}

	[[nodiscard]] awaitable<sys_expected<protocol::status_enum>> co_wait_reply
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		if( m_request.is_finished() )
		{
			co_return co_await m_reply.parse(std::move(m_request.connection()),
				use_awaitable | cancel_slot | timeout
			);
		}
		if constexpr( request_t::version() < protocol::version::v11 )
		{
			if( not m_request.is_finished() )
			{
				co_return sys_unexpected (
					make_error_code(std::errc::device_or_resource_busy)
				);
			}
			co_return co_await m_reply.parse(std::move(m_request.connection()),
				use_awaitable | cancel_slot | timeout
			);
		}
		else
		{
			auto expected = co_await m_reply.parse(std::move(m_request.connection()),
				use_awaitable | cancel_slot | timeout
			);
			if( not expected or request_t::version() < protocol::version::v11 )
				co_return expected;

			else if( m_reply.status() == protocol::status::continue_upload )
				m_request.connection() = std::move(m_reply.connection());
			co_return expected;
		}
	}

	[[nodiscard]] awaitable<sys_expected<protocol::status_enum>> co_wait_reply(std::error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_wait_reply(
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	request_t m_request {};
	reply_t m_reply {};
};

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::basic_request_context(request_t &&request) :
	m_impl(new impl(std::move(request)))
{

}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::~basic_request_context()
{
	delete m_impl;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::basic_request_context(basic_request_context &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>&
basic_request_context<Method,Connection,Version>::operator=(basic_request_context &&other) noexcept
{
	if( this == &other.m_impl )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
const basic_request_context<Method,Connection,Version>::request_t&
basic_request_context<Method,Connection,Version>::request() const noexcept
{
	return m_impl->m_request;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::request_t&
basic_request_context<Method,Connection,Version>::request() noexcept
{
	return m_impl->m_request;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
const basic_request_context<Method,Connection,Version>::reply_t&
basic_request_context<Method,Connection,Version>::reply() const noexcept
{
	return m_impl->m_reply;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::reply_t&
basic_request_context<Method,Connection,Version>::reply() noexcept
{
	return m_impl->m_reply;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,protocol::status_enum> Token>
auto basic_request_context<Method,Connection,Version>::wait_reply(Token &&token) noexcept
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->wait_reply()
			.or_else([&token](const std::error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->wait_reply();

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
				return m_impl->co_wait_reply(ntoken.ec_,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_wait_reply (
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
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_wait_reply (
						ntoken.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_wait_reply (
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_wait_reply (
					ntoken.ec_, cancel_slot, timeout
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
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_wait_reply (
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
		return wait_reply(token | 0ns);
	}
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
bool basic_request_context<Method,Connection,Version>::responded() const noexcept
{
	return reply().valid();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>::executor_t
basic_request_context<Method,Connection,Version>::get_executor() const noexcept
{
	return m_impl->m_request->get_executor();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request_context<Method,Connection,Version>&
basic_request_context<Method,Connection,Version>::cancel() noexcept
{
	reply().cancel();
	request().cancel();
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CONTEXT_H