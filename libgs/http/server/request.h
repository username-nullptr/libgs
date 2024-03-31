
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
	size_t read(buffer<void*> buf, read_token<error_code&> tk);
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

#include <libgs/core/string_list.h>

namespace libgs::http
{

namespace detail
{

template <typename T>
struct _static_string;

#define LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(_type, ...) \
	static constexpr const _type *v_1_0      = __VA_ARGS__##"1.0"       ; \
	static constexpr const _type *v_1_1      = __VA_ARGS__##"1.1"       ; \
	static constexpr const _type *close      = __VA_ARGS__##"close"     ; \
	static constexpr const _type *comma      = __VA_ARGS__##","         ; \
	static constexpr const _type *gzip       = __VA_ARGS__##"gzip"      ; \
	static constexpr const _type *chunked    = __VA_ARGS__##"chunked"   ; \
	static constexpr const _type *connection = __VA_ARGS__##"connection"; \
	static constexpr const _type *upgrade    = __VA_ARGS__##"upgrade"   ; \
	static constexpr const _type *websocket  = __VA_ARGS__##"websocket" ;

template <>
struct _static_string<char> {
	LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(char);
};

template <>
struct _static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STATIC_STRING_KEY(wchar_t,L);
};

} //namespace detail

template <concept_char_type CharT, concept_execution Exec>
class basic_server_request<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using static_string = detail::_static_string<CharT>;

public:
	template <typename Socket, typename Datagram>
	impl(Socket &&socket, Datagram &&datagram) :
		m_socket(std::forward<Socket>(socket)), m_datagram(std::forward<Datagram>(datagram)) {}

	impl()
	{
		auto it = m_datagram.headers.find(header::connection);
		if( it == m_datagram.headers.end() )
			m_keep_alive = m_datagram.version != static_string::v_1_0;
		else
			m_keep_alive = str_to_lower(it->second) != static_string::close;

		it = m_datagram.headers.find(header::accept_encoding);
		if( it == m_datagram.headers.end() )
			m_support_gzip = false;
		else
		{
			for(auto &str : string_list::from_string(it->second, static_string::comma))
			{
				if( str_to_lower(str_trimmed(str)) == static_string::gzip )
				{
					m_support_gzip = true;
					break;
				}
			}
		}
		it = m_datagram.headers.find(header::content_length);
		if( it != m_datagram.headers.end() )
		{
			auto tplen = it->second.template get<std::size_t>();
			if( m_datagram.partial_body.size() > tplen )
				m_datagram.partial_body = m_datagram.partial_body.substr(0, tplen);

			m_rb_status = tplen > 0 ? rb_status::length : rb_status::finished;
			m_rb_context = tplen;
		}
		else if( m_datagram.version == static_string::v_1_1 )
		{
			it = m_datagram.headers.find(header::transfer_encoding);
			if( it == m_datagram.headers.end() or it->second != static_string::chunked )
				m_rb_status = rb_status::finished;
			else
			{
				m_rb_status = rb_status::wait_size;
				m_rb_context = m_datagram.partial_body;
				m_datagram.partial_body.clear();
			}
		}
		else
		{
			m_rb_status = rb_status::finished;
			m_rb_context = std::size_t(0);
		}
		if( m_datagram.version == static_string::v_1_1 )
		{
			it = m_datagram.headers.find(static_string::connection);
			if( it != m_datagram.headers.end() and str_to_lower(it->second) == static_string::upgrade )
			{
				it = m_datagram.headers.find(static_string::upgrade);
				if( it != m_datagram.headers.end() and str_to_lower(it->second) == static_string::websocket )
					m_websocket = true;
			}
		}
	}

public:
	[[nodiscard]] size_t read_length_mode(error_code &error, void *buf, size_t size)
	{

	}

	[[nodiscard]] size_t read_chunked_mode(error_code &error, void *buf, size_t size)
	{

	}

	[[nodiscard]] size_t device_buffer_size() const
	{
		asio::ip::tcp::socket::send_buffer_size attr;
		m_socket->get_option(attr);
		return attr.value();
	}

public:
	io::basic_socket_ptr<Exec> m_socket;
	datagram_type m_datagram;

	bool m_keep_alive = true;
	bool m_support_gzip = false;
	bool m_websocket = false;

	enum class rb_status
	{
		length,
		wait_size,
		wait_content,
		wait_headers,
		chunked_end,
		finished
	}
	m_rb_status = rb_status::length;
	std::variant<size_t,std::string> m_rb_context;
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

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, read_cb_token<size_t,error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, read_cb_token<size_t> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<void*> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<void*> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<void*> buf)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, read_cb_token<size_t,error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, read_cb_token<size_t> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, read_cb_token<error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, read_cb_token<> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<std::string&> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<std::string&> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, read_cb_token<size_t,error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, read_cb_token<size_t> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read_all(buffer<void*> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read_all(buffer<void*> buf, read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, read_cb_token<size_t,error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, read_cb_token<size_t> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, read_cb_token<error_code> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, read_cb_token<> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<std::string> basic_server_request<CharT,Exec>::co_read_all(read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
std::string basic_server_request<CharT,Exec>::read_all(read_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_save_file(const std::string &file_name, save_file_cb_token<size_t,error_code> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_save_file(const std::string &file_name, save_file_cb_token<size_t> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_save_file(const std::string &file_name, save_file_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::save_file(const std::string &file_name, save_file_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::is_websocket_handshake() const
{
	return m_impl->m_websocket;
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::keep_alive() const
{
	return m_impl->m_keep_alive;
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::support_gzip() const
{
	return m_impl->m_support_gzip;
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_request<CharT,Exec>::can_read_body() const
{
	return m_impl->m_rb_status != impl::rb_status::finished;
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

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_REQUEST_H
