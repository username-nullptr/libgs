
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

#include <libgs/http/server/request_parser.h>
#include <libgs/http/basic/opt_token.h>
#include <libgs/io/socket.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server_request : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_server_request)
	using base_type = io::device_base<Exec>;

public:
	using executor_type = typename base_type::executor_type;
	using socket_ptr = io::basic_socket_ptr<Exec>;
	using parser_type = basic_request_parser<CharT>;

	using str_type = typename parser_type::str_type;
	using str_view_type = std::basic_string_view<CharT>;

	using headers_view_type = typename parser_type::headers_view_type;
	using cookies_view_type = typename parser_type::cookies_view_type;
	using parameters_view_type = typename parser_type::parameters_view_type;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	basic_server_request(socket_ptr socket, parser_type &parser);
	~basic_server_request() override;

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_view_type version() const noexcept;
	[[nodiscard]] str_view_type path() const noexcept;

public:
	[[nodiscard]] parameters_view_type parameters() const noexcept;
	[[nodiscard]] headers_view_type headers() const noexcept;
	[[nodiscard]] cookies_view_type cookies() const noexcept;

public:
	void async_read(buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void async_read(buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<void*> buf, opt_token<error_code&> tk = {});
	size_t read(buffer<void*> buf, opt_token<error_code&> tk = {});

	void async_read(buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<callback_t<size_t>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<callback_t<error_code>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<std::string&> buf, opt_token<error_code&> tk = {});
	size_t read(buffer<std::string&> buf, opt_token<error_code&> tk = {});

public:
	void async_read_all(buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void async_read_all(buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read_all(buffer<void*> buf, opt_token<error_code&> tk = {});
	size_t read_all(buffer<void*> buf, opt_token<error_code&> tk = {});

	void async_read_all(std::string &buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<callback_t<size_t>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<callback_t<error_code>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<std::string> co_read_all(opt_token<error_code&> tk = {});
	std::string read_all(opt_token<error_code&> tk = {});

public:
	void async_save_file(const std::string &file_name, opt_token<begin_t,total_t,callback_t<size_t,error_code>> tk);
	void async_save_file(const std::string &file_name, opt_token<begin_t,total_t,callback_t<size_t>> tk);

	[[nodiscard]] awaitable<size_t> co_save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk = {});
	size_t save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk = {});

public:
	[[nodiscard]] bool is_websocket_handshake() const;
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool can_read_body() const;

public:
	[[nodiscard]] io::ip_endpoint remote_endpoint() const;
	[[nodiscard]] io::ip_endpoint local_endpoint() const;
	void cancel() noexcept override;

private:
	class impl;
	impl *m_impl;
};

using server_request = basic_server_request<char>;
using server_wrequest = basic_server_request<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
using basic_server_request_ptr = std::shared_ptr<basic_server_request<CharT,Exec>>;

using server_request_ptr = std::shared_ptr<basic_server_request<char>>;
using server_wrequest_ptr = std::shared_ptr<basic_server_request<char>>;

} //namespace libgs::http

#include <libgs/core/string_list.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server_request<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Socket>
	impl(Socket &&socket, parser_type &parser) :
		m_socket(std::forward<Socket>(socket)), m_parser(parser) {}

public:


public:
	[[nodiscard]] size_t io_buffer_size() const
	{
		asio::ip::tcp::socket::send_buffer_size attr;
		m_socket->get_option(attr);
		return attr.value();
	}

public:
	socket_ptr m_socket;
	parser_type &m_parser;
};

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec>::basic_server_request(io::basic_socket_ptr<Exec> socket, parser_type &parser) :
	base_type(socket->executor()), m_impl(new impl(std::move(socket), parser))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec>::~basic_server_request()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
http::method basic_server_request<CharT,Exec>::method() const noexcept
{
	return m_impl->m_parser.method();
}

template <concept_char_type CharT, concept_execution Exec>
std::basic_string_view<CharT> basic_server_request<CharT,Exec>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <concept_char_type CharT, concept_execution Exec>
std::basic_string_view<CharT> basic_server_request<CharT,Exec>::path() const noexcept
{
	return m_impl->m_parser.path();
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::parameters_view_type basic_server_request<CharT,Exec>::parameters() const noexcept
{
	return m_impl->m_parser.parameters();
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::headers_view_type basic_server_request<CharT,Exec>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_request<CharT,Exec>::cookies_view_type basic_server_request<CharT,Exec>::cookies() const noexcept
{
	return m_impl->m_parser.cookies();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<void*> buf, opt_token<error_code&> tk)
{
	if( tk.error )
		*tk.error = std::error_code();

	auto dst_buf = reinterpret_cast<char*>(buf.data);
	size_t sum = 0;
	do {
		auto body = m_impl->m_parser.take_partial_body(buf.size);
		sum += body.size();

		memcpy(dst_buf + sum, body.c_str(), body.size());
		if( sum == buf.size )
			break;

		sum += co_await m_impl->m_socket->co_read(body, tk);
		if( body.empty() or not m_impl->m_parser.append(body) )
			break;
	}
	while(true);
	co_return sum;
}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<void*> buf, opt_token<error_code&> tk)
{
	if( tk.error )
		*tk.error = std::error_code();

	auto dst_buf = reinterpret_cast<char*>(buf.data);
	size_t sum = 0;
	do {
		auto body = m_impl->m_parser.take_partial_body(buf.size);
		sum += body.size();

		memcpy(dst_buf + sum, body.c_str(), body.size());
		if( sum == buf.size )
			break;

		sum += m_impl->m_socket->read(body, tk);
		if( body.empty() or not m_impl->m_parser.append(body) )
			break;
	}
	while(true);
	return sum;
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<callback_t<error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<callback_t<>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<std::string&> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<std::string&> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, opt_token<callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read_all(buffer<void*> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read_all(buffer<void*> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<callback_t<error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<callback_t<>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<std::string> basic_server_request<CharT,Exec>::co_read_all(opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
std::string basic_server_request<CharT,Exec>::read_all(opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_save_file(const std::string &file_name, opt_token<begin_t,total_t,callback_t<size_t,error_code>> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_save_file(const std::string &file_name, opt_token<begin_t,total_t,callback_t<size_t>> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::is_websocket_handshake() const
{
	return m_impl->m_parser.is_websocket_handshake();
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::keep_alive() const
{
	return m_impl->m_parser.keep_alive();
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::support_gzip() const
{
	return m_impl->m_parser.support_gzip();
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::can_read_body() const
{
	return m_impl->m_parser.can_read_body();
}

template <concept_char_type CharT, concept_execution Exec>
io::ip_endpoint basic_server_request<CharT,Exec>::remote_endpoint() const
{
	return m_impl->m_socket->remote_endpoint();
}

template <concept_char_type CharT, concept_execution Exec>
io::ip_endpoint basic_server_request<CharT,Exec>::local_endpoint() const
{
	return m_impl->m_socket->local_endpoint();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::cancel() noexcept
{
	m_impl->m_socket->cancel();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_REQUEST_H
