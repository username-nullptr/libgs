
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
#define LIBGS_HTTP_SERVER_DETAIL_REQUEST_H

#include <libgs/core/string_list.h>
#include <libgs/core/app_utls.h>
#include <filesystem>
#include <fstream>

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
	socket_ptr m_socket {};
	parser_type &m_parser;
	bool m_bound = false;
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
const typename basic_server_request<CharT,Exec>::parameters_type &basic_server_request<CharT,Exec>::parameters() const noexcept
{
	return m_impl->m_parser.parameters();
}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_request<CharT,Exec>::headers_type &basic_server_request<CharT,Exec>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_request<CharT,Exec>::cookies_type &basic_server_request<CharT,Exec>::cookies() const noexcept
{
	return m_impl->m_parser.cookies();
}

template <concept_char_type CharT, concept_execution Exec>
const basic_value<CharT> &basic_server_request<CharT,Exec>::parameter(str_view_type key) const
{
	auto map = m_impl->m_parser.parameters();
	auto it = map.find(key);
	if( it == map.end() )
		throw runtime_error("libgs::http::request::parameter: key '{}' not exists.", wcstoxx<std::string>(key));
	return it->second;
}

template <concept_char_type CharT, concept_execution Exec>
const basic_value<CharT> &basic_server_request<CharT,Exec>::header(str_view_type key) const
{
	auto map = m_impl->m_parser.headers();
	auto it = map.find(key);
	if( it == map.end() )
		throw runtime_error("libgs::http::request::header: key '{}' not exists.", wcstoxx<std::string>(key));
	return it->second;
}

template <concept_char_type CharT, concept_execution Exec>
const basic_value<CharT> &basic_server_request<CharT,Exec>::cookie(str_view_type key) const
{
	auto map = m_impl->m_parser.cookies();
	auto it = map.find(key);
	if( it == map.end() )
		throw runtime_error("libgs::http::request::cookie: key '{}' not exists.", wcstoxx<std::string>(key));
	return it->second;
}

template <concept_char_type CharT, concept_execution Exec>
basic_value<CharT> basic_server_request<CharT,Exec>::parameter_or(str_view_type key, value_type def_value) const noexcept
{
	auto map = m_impl->m_parser.parameters();
	auto it = map.find(key);
	return it == map.end() ? def_value : it->second;
}

template <concept_char_type CharT, concept_execution Exec>
basic_value<CharT> basic_server_request<CharT,Exec>::header_or(str_view_type key, value_type def_value) const noexcept
{
	auto map = m_impl->m_parser.headers();
	auto it = map.find(key);
	return it == map.end() ? def_value : it->second;
}

template <concept_char_type CharT, concept_execution Exec>
basic_value<CharT> basic_server_request<CharT,Exec>::cookie_or(str_view_type key, value_type def_value) const noexcept
{
	auto map = m_impl->m_parser.cookies();
	auto it = map.find(key);
	return it == map.end() ? def_value : it->second;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::read(buffer<void*> buf, opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return 0;
	}
	if( tk.error )
		*tk.error = std::error_code();

	size_t sum = 0;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		auto dst_buf = reinterpret_cast<char*>(buf.data);
		auto buf_size = m_impl->m_socket->read_buffer_size();
		do {
			auto body = m_impl->m_parser.take_partial_body(buf.size);
			sum += body.size();

			memcpy(dst_buf + sum, body.c_str(), body.size());
			if( sum == buf.size )
				break;

			if( tk.cnl_sig )
				sum += co_await m_impl->m_socket->read({body, buf_size}, {*tk.cnl_sig, error});
			else
				sum += co_await m_impl->m_socket->read({body, buf_size}, error);

			io::detail::check_error(tk.error, error, "libgs::http::request::read");
			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return sum;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::read(buffer<std::string&> buf, opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return 0;
	}
	if( tk.error )
		*tk.error = std::error_code();

	size_t sum = 0;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		do {
			auto body = m_impl->m_parser.take_partial_body(buf.size);
			sum += body.size();

			buf.data += std::string(body.c_str(), body.size());
			if( sum == buf.size )
				break;

			body.clear();
			if( tk.cnl_sig )
				sum += co_await m_impl->m_socket->read(body, {*tk.cnl_sig, error});
			else
				sum += co_await m_impl->m_socket->read(body, error);

			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return sum;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<std::string> basic_server_request<CharT,Exec>::read_all(opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return "";
	}
	if( tk.error )
		*tk.error = std::error_code();

	auto &headers = m_impl->m_parser.headers();
	auto it = headers.find(header_type::content_length);

	size_t buf_size = 0;
	if( it != headers.end() )
		buf_size = it->second.template get<size_t>();
	else
		buf_size = m_impl->m_socket->read_buffer_size();

	std::string buf;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		do {
			auto body = m_impl->m_parser.take_partial_body(buf_size);
			buf += std::string(body.c_str(), body.size());

			body.clear();
			if( tk.cnl_sig )
				co_await m_impl->m_socket->read(body, {*tk.cnl_sig, error});
			else
				co_await m_impl->m_socket->read(body, error);

			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
		co_return 0;
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return buf;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_request<CharT,Exec>::save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk)
{
	if( file_name.empty() )
	{
		io::detail::check_error(tk.error, std::make_error_code(std::errc::invalid_argument), "libgs::http::request::save_file");
		co_return 0;
	}
	auto _file_name = app::absolute_path(file_name);
	using namespace std::chrono_literals;
	namespace fs = std::filesystem;

	if( tk.begin > 0 and not fs::exists(file_name) )
	{
		io::detail::check_error(tk.error, std::make_error_code(std::errc::no_such_file_or_directory), "libgs::http::request::save_file");
		co_return 0;
	}
	std::ofstream file(_file_name, std::ios_base::out | std::ios_base::binary);
	if( not file.is_open() )
	{
		io::detail::check_error(tk.error, std::make_error_code(static_cast<std::errc>(errno)), "libgs::http::request::save_file");
		co_return 0;
	}
	using pos_type = std::ifstream::pos_type;
	file.seekp(static_cast<pos_type>(tk.begin));

	auto tcp_buf_size = m_impl->tcp_ip_buffer_size();
	auto buf = new char[tcp_buf_size] {0};

	std::size_t sum = 0;
	error_code error;
	size_t size;

	while( can_read_body() )
	{
		if( tk.cnl_sig )
			size = read({buf, tcp_buf_size}, {tk.rtime, *tk.cnl_sig, error});
		else
			size = read({buf, tcp_buf_size}, {tk.rtime, error});

		if( error )
		{
			if( tk.error )
				*tk.error = error;
			else if( error != errc::operation_aborted )
			{
				file.close();
				delete[] buf;
				throw system_error(error, "libgs::http::request::save_file");
			}
		}
		sum += size;
		if( tk.total == 0 )
			file.write(buf, static_cast<pos_type>(size));

		else if( size > tk.total )
		{
			file.write(buf, static_cast<pos_type>(tk.total));
			break;
		}
		else
		{
			file.write(buf, static_cast<pos_type>(size));
			tk.total -= size;
		}
		co_await co_sleep_for(512us);
	}
	file.close();
	delete[] buf;
	co_return sum;
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
bool basic_server_request<CharT,Exec>::is_chunked() const
{
	if( stoi32(version()) < 1.1 )
		return false;
	auto it = m_impl->m_headers.find(header_type::transfer_encoding);
	return it != m_impl->m_headers.end() and str_to_lower(it->second) == detail::_key_static_string<CharT>::chunked;
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


#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
