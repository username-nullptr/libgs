
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_H
#define LIBGS_HTTP_SERVER_REQUEST_H

#include <libgs/http/server/request_datagram.h>
#include <libgs/http/basic/opt_token.h>
#include <libgs/io/socket.h>
#include <ranges>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server_request : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY(basic_server_request)
	using base_type = io::device_base<Exec>;

public:
	using executor_type = Exec;
	using datagram_type = basic_request_datagram<CharT>;

	using str_type = typename datagram_type::str_type;
	using str_view_type = std::basic_string_view<CharT>;

	using headers_type = typename datagram_type::headers_type;
	using cookies_type = typename datagram_type::cookies_type;
	using parameters_type = typename datagram_type::parameters_type;

	using headers_view_type = typename datagram_type::headers_view_type;
	using cookies_view_type = typename datagram_type::cookies_view_type;
	using parameters_view_type = typename datagram_type::parameters_view_type;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	basic_server_request(io::basic_socket_ptr<Exec> socket, const datagram_type &datagram);
	basic_server_request(io::basic_socket_ptr<Exec> socket, datagram_type &&datagram);

	basic_server_request(basic_server_request &&other) noexcept;
	basic_server_request &operator=(basic_server_request &&other) noexcept;

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_view_type version() const noexcept;
	[[nodiscard]] str_view_type path() const noexcept;

public:
	[[nodiscard]] parameters_view_type parameters() const noexcept;
	[[nodiscard]] headers_view_type headers() const noexcept;
	[[nodiscard]] cookies_view_type cookies() const noexcept;

public:
	void async_read(buffer<void*> buf, read_cb_token<size_t,error_code> tk) noexcept;
	void async_read(buffer<void*> buf, read_cb_token<size_t> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<void*> buf, read_token<error_code&> tk = {});
	virtual size_t read(buffer<void*> buf, read_token<error_code&> tk) = 0;
	size_t read(buffer<void*> buf);

	void async_read(buffer<std::string&> buf, read_cb_token<size_t,error_code> tk) noexcept;
	void async_read(buffer<std::string&> buf, read_cb_token<size_t> tk) noexcept;
	void async_read(buffer<std::string&> buf, read_cb_token<error_code> tk) noexcept;
	void async_read(buffer<std::string&> buf, read_cb_token<> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<std::string&> buf, read_token<error_code&> tk = {});
	size_t read(buffer<std::string&> buf, read_token<error_code&> tk = {});

public:
	void async_read_all(buffer<void*> buf, read_cb_token<size_t,error_code> tk) noexcept;
	void async_read_all(buffer<void*> buf, read_cb_token<size_t> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read_all(buffer<void*> buf, read_token<error_code&> tk = {});
	size_t read_all(buffer<void*> buf, read_token<error_code&> tk = {});

	void async_read_all(std::string &buf, read_cb_token<size_t,error_code> tk) noexcept;
	void async_read_all(std::string &buf, read_cb_token<size_t> tk) noexcept;
	void async_read_all(std::string &buf, read_cb_token<error_code> tk) noexcept;
	void async_read_all(std::string &buf, read_cb_token<> tk) noexcept;

	[[nodiscard]] awaitable<std::string> co_read_all(read_token<error_code&> tk = {});
	std::string read_all(read_token<error_code&> tk = {});

public:
	void async_save_file(const std::string &file_name, save_file_cb_token<size_t,error_code> tk);
	void async_save_file(const std::string &file_name, save_file_cb_token<size_t> tk);

	[[nodiscard]] awaitable<size_t> co_save_file(const std::string &file_name, save_file_token<error_code&> tk = {});
	size_t save_file(const std::string &file_name, save_file_token<error_code&> tk = {});

public:
	[[nodiscard]] bool is_websocket_handshake() const;
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool can_read_body() const;
	[[nodiscard]] bool is_valid() const;

public:
	[[nodiscard]] io::ip_endpoint remote_endpoint() const;
	[[nodiscard]] io::ip_endpoint local_endpoint() const;

private:
	class impl;
	impl *m_impl;
};

using server_request = basic_server_request<char>;
using server_wrequest = basic_server_request<wchar_t>;

} //namespace libgs::http

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server_request<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Socket, typename Datagram>
	impl(Socket &&socket, Datagram &&datagram) :
		m_socket(std::forward<Socket>(socket)), m_datagram(std::forward<Datagram>(datagram)) {}
	impl() = default;

public:
	io::basic_socket_ptr<Exec> m_socket;
	datagram_type m_datagram;
};

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec>::basic_server_request(io::basic_socket_ptr<Exec> socket, const datagram_type &datagram) :
	base_type(socket->executor()), m_impl(new impl(std::move(socket), datagram))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec>::basic_server_request(io::basic_socket_ptr<Exec> socket, datagram_type &&datagram) :
	base_type(socket->executor()), m_impl(new impl(std::move(socket), std::move(datagram)))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec>::basic_server_request(basic_server_request &&other) noexcept :
	base_type(std::move(other)), m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec> &basic_server_request<CharT,Exec>::operator=(basic_server_request &&other) noexcept
{
	base_type::operator=(std::move(other));
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
http::method basic_server_request<CharT,Exec>::method() const noexcept
{
	return m_impl->m_datagram.method;
}

template <concept_char_type CharT, concept_execution Exec>
std::basic_string_view<CharT> basic_server_request<CharT,Exec>::version() const noexcept
{
	return m_impl->m_datagram.version;
}

template <concept_char_type CharT, concept_execution Exec>
std::basic_string_view<CharT> basic_server_request<CharT,Exec>::path() const noexcept
{
	return m_impl->m_datagram.path;
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::parameters_view_type basic_server_request<CharT,Exec>::parameters() const noexcept
{
	return m_impl->m_datagram.parameters | std::views::take(m_impl->m_datagram.parameters.size());
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::headers_view_type basic_server_request<CharT,Exec>::headers() const noexcept
{
	return m_impl->m_datagram.headers | std::views::take(m_impl->m_datagram.headers.size());
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::cookies_view_type basic_server_request<CharT,Exec>::cookies() const noexcept
{
	return m_impl->m_datagram.cookies | std::views::take(m_impl->m_datagram.cookies.size());
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_REQUEST_H
