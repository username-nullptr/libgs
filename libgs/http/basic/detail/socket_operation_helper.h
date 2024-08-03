
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_SOCKET_OPERATION_HELPER_H
#define LIBGS_HTTP_BASIC_DETAIL_SOCKET_OPERATION_HELPER_H

namespace libgs::http
{

template <concept_execution Exec>
inline void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::get_option
(socket_t &socket, auto &option, error_code &error)
{
	socket.get_option(option, error);
}

template <concept_execution Exec>
void socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::close(socket_t &socket)
{
	if( socket.is_open() )
	{
		error_code error;
		socket.close(error);
	}
}

template <concept_execution Exec>
inline typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::remote_endpoint(socket_t &socket)
{
	return socket.remote_endpoint();
}

template <concept_execution Exec>
inline typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::endpoint_t
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::local_endpoint(socket_t &socket)
{
	return socket.local_endpoint();
}

template <concept_execution Exec>
inline const typename socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::executor_t&
socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::get_executor(socket_t &socket) noexcept
{
	return socket.get_executor();
}

template <concept_execution Exec>
inline bool socket_operation_helper<asio::basic_stream_socket<asio::ip::tcp,Exec>>::is_open(socket_t &socket) noexcept
{
	return socket.is_open();
}

template <concept_execution Exec>
inline void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::get_option
(socket_t &socket, auto &option, error_code &error)
{
	socket.next_layer().get_option(option, error);
}

template <concept_execution Exec>
void socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::close(socket_t &socket)
{
	if( socket.next_layer().is_open() )
	{
		error_code error;
		socket.shutdown(error);
		socket.next_layer().close(error);
	}
}

template <concept_execution Exec>
inline typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::remote_endpoint(socket_t &socket)
{
	return socket.next_layer().remote_endpoint();
}

template <concept_execution Exec>
inline typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::endpoint_t
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::local_endpoint(socket_t &socket)
{
	return socket.next_layer().local_endpoint();
}

template <concept_execution Exec>
inline const typename socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::executor_t&
socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::get_executor(socket_t &socket) noexcept
{
	return socket.next_layer().get_executor();
}

template <concept_execution Exec>
inline bool socket_operation_helper<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::is_open
(socket_t &socket) noexcept
{
	return socket.next_layer().is_open();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_DETAIL_SOCKET_OPERATION_HELPER_H
