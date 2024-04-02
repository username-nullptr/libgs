
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
	void async_read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void async_read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	size_t read(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});

	void async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept;
	void async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});
	size_t read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

public:
	void async_read_all(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void async_read_all(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> co_read_all(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	size_t read_all(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});

	void async_read_all(std::string &buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept;
	void async_read_all(std::string &buf, opt_token<read_condition,callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<std::string> co_read_all(opt_token<read_condition,error_code&> tk = {});
	std::string read_all(opt_token<read_condition,error_code&> tk = {});

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
	[[nodiscard]] awaitable<size_t> co_read_length_mode(void *buf, size_t size, opt_token<error_code&> tk = {})
	{
		auto &tplen = get<std::size_t>(m_rb_context);
		if( size > tplen )
			size = tplen;

		std::size_t sum = 0;
		if( size > m_datagram.partial_body.size() )
		{
			if( not m_datagram.partial_body.empty() )
			{
				memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());
				tplen -= m_datagram.partial_body.size();
				size -= m_datagram.partial_body.size();

				sum += m_datagram.partial_body.size();
				m_datagram.partial_body.clear();
			}
			char *cbuf = reinterpret_cast<char*>(buf) + sum;
			while( tplen > 0 and size > 0 )
			{
				auto res = co_await m_socket->co_read({cbuf, size}, tk);

				cbuf += res;
				sum += res;

				size -= res;
				tplen -= res;

				if( not tk.error or not *tk.error )
					continue;
				else if( res > 0 )
					*tk.error = std::error_code();
				else
				{
					if( sum > 0 )
						*tk.error = std::error_code();
					break;
				}
			}
		}
		else
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			tplen -= size;
			sum = size;
		}
		if( tplen == 0 )
		{
			m_rb_status = rb_status::finished;
			if( tk.error )
				*tk.error = std::error_code();
		}
		co_return sum;
	}

	[[nodiscard]] size_t read_length_mode(void *buf, size_t size, opt_token<error_code&> tk = {})
	{
		auto &tplen = get<std::size_t>(m_rb_context);
		if( size > tplen )
			size = tplen;

		std::size_t sum = 0;
		if( size > m_datagram.partial_body.size() )
		{
			if( not m_datagram.partial_body.empty() )
			{
				memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());
				tplen -= m_datagram.partial_body.size();
				size -= m_datagram.partial_body.size();

				sum += m_datagram.partial_body.size();
				m_datagram.partial_body.clear();
			}
			char *cbuf = reinterpret_cast<char*>(buf) + sum;
			while( tplen > 0 and size > 0 )
			{
				auto res = m_socket->read({cbuf, size}, tk);

				cbuf += res;
				sum += res;

				size -= res;
				tplen -= res;

				if( not tk.error or not *tk.error )
					continue;
				else if( res > 0 )
					*tk.error = std::error_code();
				else
				{
					if( sum > 0 )
						*tk.error = std::error_code();
					break;
				}
			}
		}
		else
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			tplen -= size;
			sum = size;
		}
		if( tplen == 0 )
		{
			m_rb_status = rb_status::finished;
			if( tk.error )
				*tk.error = std::error_code();
		}
		return sum;
	}

public:
	[[nodiscard]] awaitable<size_t> co_read_chunked_mode(void *buf, size_t size, opt_token<error_code&> tk = {})
	{
		if( size <= m_datagram.partial_body.size() )
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			co_return size;
		}
		std::size_t sum = 0;
		memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());

		size -= m_datagram.partial_body.size();
		sum += m_datagram.partial_body.size();
		m_datagram.partial_body.clear();

		if( m_rb_status == rb_status::chunked_end )
		{
			m_rb_status = rb_status::finished;
			co_return sum;
		}
		auto &abuf = get<std::string>(m_rb_context);
		if( abuf.empty() )
		{
			auto tcp_size = io_buffer_size();
			char *tmp_buf = new char[tcp_size] {0};

			auto res = co_await m_socket->co_read({tmp_buf, tcp_size}, tk);
			abuf.append(tmp_buf, res);
			delete[] tmp_buf;

			if( tk.error and *tk.error )
			{
				if( res > 0 )
					*tk.error = std::error_code();
				else
				{
					if( sum > 0 )
						*tk.error = std::error_code();
					co_return sum;
				}
			}
		}
		std::size_t _size = 0;
		auto lambda_reset = [&]
		{
			m_rb_status = rb_status::finished;
			auto ec = std::make_error_code(std::errc::wrong_protocol_type);
			if( tk.error )
				*tk.error = ec;
			else
				throw system_error(ec, "libgs::http::request: read_chunked_mode");
		};
		while( not abuf.empty() )
		{
			auto pos = abuf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( abuf.size() >= 1024 )
				{
					lambda_reset();
					co_return 0;
				}
				break;
			}
			auto line_buf = abuf.substr(0, pos + 2);
			abuf.erase(0, pos + 2);

			if( m_rb_status == rb_status::wait_size )
			{
				line_buf.erase(pos);
				pos = line_buf.find(';');
				if( pos != std::string::npos )
					line_buf.erase(pos);

				if( line_buf.size() > 16 )
				{
					lambda_reset();
					co_return 0;
				}
				try {
					_size = static_cast<size_t>(std::stoull(line_buf, nullptr, 16));
				}
				catch(...)
				{
					lambda_reset();
					co_return 0;
				}
				m_rb_status = _size == 0 ? rb_status::wait_headers : rb_status::wait_content;
			}
			else if( m_rb_status == rb_status::wait_content )
			{
				line_buf.erase(pos);
				if( _size < line_buf.size() )
					_size = line_buf.size();
				else
					m_rb_status = rb_status::wait_size;
				m_datagram.partial_body += line_buf;
			}
			else if( m_rb_status == rb_status::wait_headers )
			{
				if( line_buf == "\r\n" )
				{
					m_rb_status = rb_status::chunked_end;
					break;
				}
				auto colon_index = line_buf.find(':');
				if( colon_index == std::string::npos )
				{
					lambda_reset();
					co_return 0;
				}
				auto header_key = str_to_lower(str_trimmed(line_buf.substr(0, colon_index)));
				auto header_value = from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1)));

				if( header_key != "cookie" )
				{
					m_datagram.headers[header_key] = header_value;
					continue;
				}
				auto list = string_list::from_string(header_value, ";");
				for(auto &statement : list)
				{
					statement = str_trimmed(statement);
					pos = statement.find('=');

					if( pos == std::string::npos )
					{
						lambda_reset();
						co_return 0;
					}
					header_key = str_trimmed(statement.substr(0,pos));
					header_value = str_trimmed(statement.substr(pos+1));
					m_datagram.cookies[header_key] = header_value;
				}
			}
		}
		if( size <= m_datagram.partial_body.size() )
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			co_return sum + size;
		}
		memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());
		sum += m_datagram.partial_body.size();
		m_datagram.partial_body.clear();

		if( m_rb_status == rb_status::chunked_end )
			m_rb_status = rb_status::finished;
		co_return sum;
	}

	[[nodiscard]] size_t read_chunked_mode(void *buf, size_t size, opt_token<error_code&> tk = {})
	{
		if( size <= m_datagram.partial_body.size() )
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			return size;
		}
		std::size_t sum = 0;
		memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());

		size -= m_datagram.partial_body.size();
		sum += m_datagram.partial_body.size();
		m_datagram.partial_body.clear();

		if( m_rb_status == rb_status::chunked_end )
		{
			m_rb_status = rb_status::finished;
			return sum;
		}
		auto &abuf = get<std::string>(m_rb_context);
		if( abuf.empty() )
		{
			auto tcp_size = io_buffer_size();
			char *tmp_buf = new char[tcp_size] {0};

			auto res = m_socket->read({tmp_buf, tcp_size}, tk);
			abuf.append(tmp_buf, res);
			delete[] tmp_buf;

			if( tk.error and *tk.error )
			{
				if( res > 0 )
					*tk.error = std::error_code();
				else
				{
					if( sum > 0 )
						*tk.error = std::error_code();
					return sum;
				}
			}
		}
		std::size_t _size = 0;
		auto lambda_reset = [&]
		{
			m_rb_status = rb_status::finished;
			auto ec = std::make_error_code(std::errc::wrong_protocol_type);
			if( tk.error )
				*tk.error = ec;
			else
				throw system_error(ec, "libgs::http::request: read_chunked_mode");
		};
		while( not abuf.empty() )
		{
			auto pos = abuf.find("\r\n");
			if( pos == std::string::npos )
			{
				if( abuf.size() >= 1024 )
				{
					lambda_reset();
					return 0;
				}
				break;
			}
			auto line_buf = abuf.substr(0, pos + 2);
			abuf.erase(0, pos + 2);

			if( m_rb_status == rb_status::wait_size )
			{
				line_buf.erase(pos);
				pos = line_buf.find(';');
				if( pos != std::string::npos )
					line_buf.erase(pos);

				if( line_buf.size() > 16 )
				{
					lambda_reset();
					return 0;
				}
				try {
					_size = static_cast<size_t>(std::stoull(line_buf, nullptr, 16));
				}
				catch(...)
				{
					lambda_reset();
					return 0;
				}
				m_rb_status = _size == 0 ? rb_status::wait_headers : rb_status::wait_content;
			}
			else if( m_rb_status == rb_status::wait_content )
			{
				line_buf.erase(pos);
				if( _size < line_buf.size() )
					_size = line_buf.size();
				else
					m_rb_status = rb_status::wait_size;
				m_datagram.partial_body += line_buf;
			}
			else if( m_rb_status == rb_status::wait_headers )
			{
				if( line_buf == "\r\n" )
				{
					m_rb_status = rb_status::chunked_end;
					break;
				}
				auto colon_index = line_buf.find(':');
				if( colon_index == std::string::npos )
				{
					lambda_reset();
					return 0;
				}
				auto header_key = str_to_lower(str_trimmed(line_buf.substr(0, colon_index)));
				auto header_value = from_percent_encoding(str_trimmed(line_buf.substr(colon_index + 1)));

				if( header_key != "cookie" )
				{
					m_datagram.headers[header_key] = header_value;
					continue;
				}
				auto list = string_list::from_string(header_value, ";");
				for(auto &statement : list)
				{
					statement = str_trimmed(statement);
					pos = statement.find('=');

					if( pos == std::string::npos )
					{
						lambda_reset();
						return 0;
					}
					header_key = str_trimmed(statement.substr(0,pos));
					header_value = str_trimmed(statement.substr(pos+1));
					m_datagram.cookies[header_key] = header_value;
				}
			}
		}
		if( size <= m_datagram.partial_body.size() )
		{
			memcpy(buf, m_datagram.partial_body.c_str(), size);
			m_datagram.partial_body.erase(0,size);
			return sum + size;
		}
		memcpy(buf, m_datagram.partial_body.c_str(), m_datagram.partial_body.size());
		sum += m_datagram.partial_body.size();
		m_datagram.partial_body.clear();

		if( m_rb_status == rb_status::chunked_end )
			m_rb_status = rb_status::finished;
		return sum;
	}

public:
	[[nodiscard]] size_t io_buffer_size() const
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
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{
	using rb_stat = impl::rb_status;
	if( buf.size == 0 or m_impl->m_rb_status == rb_stat::finished )
		co_return 0;

	else if( m_impl->m_rb_status == rb_stat::length )
		co_return co_await m_impl->co_read_length_mode(buf.data, buf.size, tk);

	co_return co_await m_impl->co_read_chunked_mode(buf.data, buf.size, tk);
}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{
	using rb_stat = impl::rb_status;
	if( buf.size == 0 or m_impl->m_rb_status == rb_stat::finished )
		return 0;

	else if( m_impl->m_rb_status == rb_stat::length )
		return m_impl->read_length_mode(buf.data, buf.size, tk);

	return m_impl->read_chunked_mode(buf.data, buf.size, tk);
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::co_read_all(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
size_t basic_server_request<CharT,Exec>::read_all(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_request<CharT,Exec>::async_read_all(std::string &buf, opt_token<read_condition,callback_t<>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<std::string> basic_server_request<CharT,Exec>::co_read_all(opt_token<read_condition,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
std::string basic_server_request<CharT,Exec>::read_all(opt_token<read_condition,error_code&> tk)
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
