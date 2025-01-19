
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_CXX_DETAIL_SOCKET_OPERATION_HELPER_H
#define LIBGS_HTTP_CXX_DETAIL_SOCKET_OPERATION_HELPER_H

#include <libgs/core/coroutine.h>

namespace libgs::http
{

template <concepts::stream Stream>
class socket_operation_helper_base<Stream>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(socket_t &socket) : m_socket(socket) {}
	socket_t &m_socket;
};

template <concepts::stream Stream>
socket_operation_helper_base<Stream>::socket_operation_helper_base(socket_t &socket) :
	m_impl(new impl(socket))
{

}

template <concepts::stream Stream>
socket_operation_helper_base<Stream>::~socket_operation_helper_base()
{
	delete m_impl;
}

template <concepts::stream Stream>
typename socket_operation_helper_base<Stream>::executor_t
socket_operation_helper_base<Stream>::get_executor() noexcept
{
	return m_impl->m_socket.get_executor();
}

template <concepts::stream Stream>
const typename socket_operation_helper_base<Stream>::socket_t&
socket_operation_helper_base<Stream>::socket() const noexcept
{
	return m_impl->m_socket;
}

template <concepts::stream Stream>
typename socket_operation_helper_base<Stream>::socket_t&
socket_operation_helper_base<Stream>::socket() noexcept
{
	return m_impl->m_socket;
}

template <core_concepts::execution Exec>
template <core_concepts::opt_token<error_code> Token>
auto socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
connect(endpoint_t ep, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
		this->socket().connect(std::move(ep), token);

	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = connect(ep, error);
		if( error )
			throw system_error(error, "libgs::http::socket_operation_helper::connect");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		error_code error;
		this->socket().async_connect(ep, token[error]);
		check_error(remove_const(token), error, "libgs::http::socket_operation_helper::connect");
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( core_concepts::dis_func_opt_token<token_t> )
	{
		using namespace libgs::operators;
		return asio::co_spawn(this->get_executor(), [
			&socket = this->socket(), ep = std::move(ep), token
		]() mutable -> awaitable<void>
		{
			error_code error;
			co_await socket.async_connect(ep, use_awaitable | error);
			check_error(remove_const(token), error, "libgs::http::socket_operation_helper::connect");
			co_return ;
		},
		token);
	}
	else
		this->socket().async_connect(std::move(ep), std::forward<Token>(token));
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
get_option(auto &option, error_code &error) noexcept
{
	this->socket().get_option(option, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
get_option(auto &option)
{
	error_code error;
	this->socket().get_option(option, error);
	if( error )
		throw system_error(error, "libgs::http::socket_operation_helper::get_option");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
non_blocking(bool mode, error_code &error) noexcept
{
	this->socket().non_blocking(mode, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
non_blocking(bool mode) noexcept
{
	this->socket().non_blocking(mode);
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::non_blocking() const
{
	return this->socket().non_blocking();
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::cancel() noexcept
{
	if( this->socket().is_open() )
	{
		error_code error;
		this->socket().cancel(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::close() noexcept
{
	if( this->socket().is_open() )
	{
		error_code error;
		this->socket().close(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::remote_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket().remote_endpoint(error);
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::local_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket().local_endpoint(error);
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::is_open() noexcept
{
	return this->socket().is_open();
}

#ifdef LIBGS_ENABLE_OPENSSL

template <core_concepts::execution Exec>
template <core_concepts::opt_token<error_code> Token>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
connect(endpoint_t ep, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
	{
		this->socket().next_layer().connect(ep, token);
		if( not token )
			this->socket().handshake(asio::ssl::stream_base::client, token);
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = connect(ep, error);
		if( error )
			throw system_error(error, "libgs::http::socket_operation_helper::connect");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		error_code error;
		this->socket().next_layer().async_connect(ep, token[error]);
		if( check_error(token, error, "libgs::http::socket_operation_helper::connect") )
		{
			this->socket().async_handshake(std::move(ep), token[error]);
			check_error(remove_const(token), error, "libgs::http::socket_operation_helper::connect");
		}
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( core_concepts::dis_func_opt_token<token_t> )
	{
		using namespace libgs::operators;
		return asio::co_spawn(this->get_executor(), [
			&socket = this->socket(), ep = std::move(ep), token
		]() mutable -> awaitable<void>
		{
			error_code error;
			co_await socket.next_layer().async_connect(ep, use_awaitable | error);
			if( not check_error(token, error, "libgs::http::socket_operation_helper::connect") )
			{
				co_await socket.async_handshake(std::move(ep), use_awaitable | error);
				check_error(remove_const(token), error, "libgs::http::socket_operation_helper::connect");
			}
			co_return ;
		},
		token);
	}
	else
	{
		this->socket().next_layer().async_connect(std::move(ep), [
			&socket = this->socket(), ep = std::move(ep), token = std::forward<Token>(token)
		](const error_code &error)
		{
			if( not error )
				socket.async_handshake(std::move(ep), std::move(token));
		});
	}
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
get_option(auto &option, error_code &error) noexcept
{
	this->socket().next_layer().get_option(option, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
get_option(auto &option)
{
	error_code error;
	this->socket().next_layer().get_option(option, error);
	if( error )
		throw std::system_error(error, "libgs::http::socket_operation_helper::get_option");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
non_blocking(bool mode, error_code &error) noexcept
{
	this->socket().next_layer().non_blocking(mode, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
non_blocking(bool mode) noexcept
{
	this->socket().next_layer().non_blocking(mode);
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::non_blocking() const
{
	return this->socket().next_layer().non_blocking();
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::cancel() noexcept
{
	if( this->socket().next_layer().is_open() )
	{
		error_code error;
		this->socket().next_layer().cancel(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::close() noexcept
{
	if( this->socket().next_layer().is_open() )
	{
		error_code error;
		this->socket().shutdown(error);
		this->socket().next_layer().close(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::remote_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket().next_layer().remote_endpoint(error);
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::local_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket().next_layer().local_endpoint(error);
}

template <core_concepts::execution Exec>
const typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::executor_t&
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::get_executor() noexcept
{
	return this->socket().next_layer().get_executor();
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::is_open() noexcept
{
	return this->socket().next_layer().is_open();
}

#endif //LIBGS_ENABLE_OPENSSL

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_SOCKET_OPERATION_HELPER_H
