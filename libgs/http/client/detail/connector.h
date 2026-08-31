
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CONNECTOR_H
#define LIBGS_HTTP_CLIENT_DETAIL_CONNECTOR_H

#include <libgs/http/utils/tcp_connection.h>
#include <libgs/http/utils/tls_connection.h>

namespace libgs::http
{

#if LIBGS_OPENSSL_SUPPORT
namespace detail {
[[nodiscard]] LIBGS_HTTP_API asio::ssl::context &default_ssl_context() noexcept;
} //namespace detail
#endif //LIBGS_OPENSSL_SUPPORT

template <core_concepts::exec Exec>
class LIBGS_HTTP_TAPI basic_connector<Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(executor_t exec) :
		m_exec(std::move(exec)) {}

#if LIBGS_OPENSSL_SUPPORT
	impl(executor_t exec, asio::ssl::context &tls_context) :
		m_exec(std::move(exec)), m_tls_context(&tls_context) {}
#endif //LIBGS_OPENSSL_SUPPORT

	executor_t m_exec {};

#if LIBGS_OPENSSL_SUPPORT
	asio::ssl::context *m_tls_context =
		&detail::default_ssl_context();
#endif //LIBGS_OPENSSL_SUPPORT
};

template <core_concepts::exec Exec>
basic_connector<Exec>::basic_connector
(core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(new impl(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

#if LIBGS_OPENSSL_SUPPORT
template <core_concepts::exec Exec>
basic_connector<Exec>::basic_connector
(core_concepts::match_sched<executor_t> auto &&exec, asio::ssl::context &tls_context) :
	m_impl(new impl(get_executor_helper(std::forward<decltype(exec)>(exec)), tls_context))
{

}
#endif //LIBGS_OPENSSL_SUPPORT

template <core_concepts::exec Exec>
basic_connector<Exec>::~basic_connector()
{
	delete m_impl;
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_connector<Exec>::connect(const connect_target &target, Token &&token) noexcept
	requires concepts::dis_detach_opt_token<Token,error_code,connection_ptr>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		auto result = do_connect(target);
		token = result ? error_code{} : result.error();
		return result ? std::move(*result) : connection_ptr{};
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return do_connect(target);
	else
	{
		return initiate_expected<connection_ptr>(get_executor(),
		[this, target]() mutable -> awaitable<sys_expected<connection_ptr>> {
			co_return co_await co_do_connect(target);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
basic_connector<Exec>::executor_t basic_connector<Exec>::get_executor() const noexcept
{
	return m_impl->m_exec;
}

template <core_concepts::exec Exec>
sys_expected<typename basic_connector<Exec>::connection_ptr>
basic_connector<Exec>::do_connect(const connect_target &target) noexcept
{
	try {
		if( target.host.empty() or target.port == 0 )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		using resolver_t = asio::ip::basic_resolver<asio::ip::tcp,executor_t>;
		resolver_t resolver(m_impl->m_exec);

		error_code error {};
		auto results = resolver.resolve(target.host,
			std::to_string(target.port), error
		);
		if( error )
			return sys_unexpected(error);

		if( results.empty() )
			return sys_unexpected(make_error_code(errc::host_not_found));

		if( target.security == security_mode::plain )
		{
			typename basic_tcp_connection<executor_t>::socket_t socket(m_impl->m_exec);
			asio::connect(socket, results, error);

			if( error )
				return sys_unexpected(error);

			return connection_ptr(
				std::make_shared<basic_tcp_connection<executor_t>>(std::move(socket))
			);
		}
#if LIBGS_OPENSSL_SUPPORT
		if( target.security == security_mode::tls )
		{
			typename basic_tls_connection<executor_t>::socket_t socket (
				m_impl->m_exec, *m_impl->m_tls_context
			);
			error_code address_error {};
			ignore_unused(asio::ip::make_address(target.host, address_error));

			if( address_error and
				not ::SSL_set_tlsext_host_name(socket.native_handle(), target.host.c_str()) )
				return sys_unexpected(make_error_code(std::errc::protocol_error));

			socket.set_verify_callback (
				asio::ssl::host_name_verification(target.host), error
			);
			if( error )
				return sys_unexpected(error);

			asio::connect(socket.next_layer(), results, error);
			if( error )
				return sys_unexpected(error);

			socket.handshake(asio::ssl::stream_base::client, error);
			if( error )
				return sys_unexpected(error);

			return connection_ptr (
				std::make_shared<basic_tls_connection<executor_t>>(std::move(socket))
			);
		}
#endif //LIBGS_OPENSSL_SUPPORT
		return sys_unexpected(make_error_code(std::errc::operation_not_supported));
	}
	catch(const std::system_error &ex) {
		return sys_unexpected(ex.code());
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

template <core_concepts::exec Exec>
awaitable<sys_expected<typename basic_connector<Exec>::connection_ptr>>
basic_connector<Exec>::co_do_connect(const connect_target &target) noexcept
{
	try {
		if( target.host.empty() or target.port == 0 )
			co_return sys_unexpected(make_error_code(std::errc::invalid_argument));

		using resolver_t = asio::ip::basic_resolver<asio::ip::tcp,executor_t>;
		resolver_t resolver(m_impl->m_exec);

		error_code error {};
		auto results = co_await resolver.async_resolve(target.host,
			std::to_string(target.port),
			asio::redirect_error(use_awaitable, error)
		);
		if( error )
			co_return sys_unexpected(error);

		if( results.empty() )
			co_return sys_unexpected(make_error_code(errc::host_not_found));

		if( target.security == security_mode::plain )
		{
			typename basic_tcp_connection<executor_t>::socket_t socket(m_impl->m_exec);
			co_await asio::async_connect(socket, results,
				asio::redirect_error(use_awaitable, error)
			);
			if( error )
				co_return sys_unexpected(error);

			co_return connection_ptr(
				std::make_shared<basic_tcp_connection<executor_t>>(std::move(socket))
			);
		}
#if LIBGS_OPENSSL_SUPPORT
		if( target.security == security_mode::tls )
		{
			typename basic_tls_connection<executor_t>::socket_t socket (
				m_impl->m_exec, *m_impl->m_tls_context
			);
			error_code address_error {};
			ignore_unused(asio::ip::make_address(target.host, address_error));

			if( address_error and
				not ::SSL_set_tlsext_host_name(socket.native_handle(), target.host.c_str()) )
				co_return sys_unexpected(make_error_code(std::errc::protocol_error));

			socket.set_verify_callback(
				asio::ssl::host_name_verification(target.host), error
			);
			if( error )
				co_return sys_unexpected(error);

			co_await asio::async_connect(socket.next_layer(), results,
				asio::redirect_error(use_awaitable, error)
			);
			if( error )
				co_return sys_unexpected(error);

			co_await socket.async_handshake(asio::ssl::stream_base::client,
				asio::redirect_error(use_awaitable, error)
			);
			if( error )
				co_return sys_unexpected(error);

			co_return connection_ptr(
				std::make_shared<basic_tls_connection<executor_t>>(std::move(socket))
			);
		}
#endif //LIBGS_OPENSSL_SUPPORT
		co_return sys_unexpected(make_error_code(std::errc::operation_not_supported));
	}
	catch(const std::system_error &ex) {
		co_return sys_unexpected(ex.code());
	}
	catch(const std::bad_alloc&) {
		co_return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	co_return sys_unexpected(make_error_code(std::errc::io_error));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CONNECTOR_H
