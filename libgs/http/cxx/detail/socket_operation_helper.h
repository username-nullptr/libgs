
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

template <concepts::stream_requires Stream>
socket_operation_helper_base<Stream>::socket_operation_helper_base(socket_t &socket) :
	socket(socket)
{

}

template <core_concepts::execution Exec>
template <asio::completion_token_for<void(error_code)> Token>
auto socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
async_connect(endpoint_t ep, Token &&token)
{
	if constexpr( is_function_v<Token> )
		this->socket.async_connect(std::move(ep), std::forward<Token>(token));
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<Token> )
	{
		error_code error;
		this->socket.async_connect(ep, token[error]);
		check_error(remove_const(token), error, "libgs::http::socket_operation_helper::async_connect");
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		return asio::co_spawn(this->socket.get_executor(),
							  [&socket = this->socket, ep = std::move(ep), token = std::forward<Token>(token)]()
							  mutable -> awaitable<void>
		{
			error_code error;
			co_await socket.async_connect(ep, use_awaitable|error);
			check_error(remove_const(token), error, "libgs::http::socket_operation_helper::async_connect");
			co_return ;
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
connect(endpoint_t ep, error_code &error) noexcept
{
	this->socket.connect(std::move(ep), error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
connect(endpoint_t ep)
{
	error_code error;
	this->socket.connect(std::move(ep), error);
	if( error )
		throw system_error(error, "libgs::http::socket_operation_helper::connect");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
get_option(auto &option, error_code &error) noexcept
{
	this->socket.get_option(option, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::
get_option(auto &option)
{
	error_code error;
	this->socket.get_option(option, error);
	if( error )
		throw system_error(error, "libgs::http::socket_operation_helper::get_option");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::close() noexcept
{
	if( this->socket.is_open() )
	{
		error_code error;
		this->socket.close(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::remote_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket.remote_endpoint(error);
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::local_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket.local_endpoint(error);
}

template <core_concepts::execution Exec>
const typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::executor_t&
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp, Exec>>::get_executor() noexcept
{
	return this->socket.get_executor();
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::is_open() noexcept
{
	return this->socket.is_open();
}

#ifdef LIBGS_ENABLE_OPENSSL

template <core_concepts::execution Exec>
template <asio::completion_token_for<void(error_code)> Token>
auto socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
async_connect(endpoint_t ep, Token &&token)
{
	if constexpr( is_function_v<Token> )
	{
		auto _ep = ep;
		this->socket.next_layer().async_connect(std::move(ep),
			[socket = &this->socket, ep = std::move(_ep), token = std::forward<Token>(token)](error_code error)
		{
			if( not error )
				socket->async_handshake(std::move(ep), std::forward<Token>(token));
		});
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<Token> )
	{
		error_code error;
		this->socket.next_layer().async_connect(ep, token[error]);
		if( check_error(token, error, "libgs::http::socket_operation_helper::async_connect") )
		{
			this->socket.async_handshake(std::move(ep), token[error]);
			check_error(remove_const(token), error, "libgs::http::socket_operation_helper::async_connect");
		}
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		return asio::co_spawn(get_executor(),
			[socket = &this->socket, ep = std::move(ep), token = std::forward<Token>(token)]()
			mutable -> awaitable<void>
		{
			error_code error;
			co_await socket->next_layer().async_connect(ep, use_awaitable|error);
			if( not check_error(token, error, "libgs::http::socket_operation_helper::async_connect") )
			{
				socket->async_handshake(std::move(ep), use_awaitable|error);
				check_error(remove_const(token), error, "libgs::http::socket_operation_helper::async_connect");
			}
			co_return ;
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
connect(endpoint_t ep, error_code &error) noexcept
{
	this->socket.next_layer().connect(ep, error);
	if( not error )
		this->socket.handshake(asio::ssl::stream_base::client, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
connect(endpoint_t ep)
{
	error_code error;
	this->socket.next_layer().connect(ep, error);
	if( error )
		throw std::system_error(error, "libgs::http::socket_operation_helper::connect");
	this->socket.handshake(asio::ssl::stream_base::client, error);
	if( error )
		throw std::system_error(error, "libgs::http::socket_operation_helper::connect");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
get_option(auto &option, error_code &error) noexcept
{
	this->socket.next_layer().get_option(option, error);
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::
get_option(auto &option)
{
	error_code error;
	this->socket.next_layer().get_option(option, error);
	if( error )
		throw std::system_error(error, "libgs::http::socket_operation_helper::get_option");
}

template <core_concepts::execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::close() noexcept
{
	if( this->socket.next_layer().is_open() )
	{
		error_code error;
		this->socket.shutdown(error);
		this->socket.next_layer().close(error);
		ignore_unused(error);
	}
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::remote_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket.next_layer().remote_endpoint(error);
}

template <core_concepts::execution Exec>
typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::local_endpoint() noexcept
{
	error_code error; ignore_unused(error);
	return this->socket.next_layer().local_endpoint(error);
}

template <core_concepts::execution Exec>
const typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::executor_t&
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::get_executor() noexcept
{
	return this->socket.next_layer().get_executor();
}

template <core_concepts::execution Exec>
const typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::executor_t&
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::get_executor() noexcept
{
	return this->socket.next_layer().get_executor();
}

template <core_concepts::execution Exec>
bool socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::is_open() noexcept
{
	return this->socket.next_layer().is_open();
}

#endif //LIBGS_ENABLE_OPENSSL

} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_SOCKET_OPERATION_HELPER_H
