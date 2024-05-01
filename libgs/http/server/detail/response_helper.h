
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H

#include <libgs/core/algorithm/mime_type.h>
#include <libgs/core/log.h>
#include <filesystem>
#include <list>

namespace libgs::http
{

namespace detail
{

template <typename T>
struct _response_helper_static_string;

#define LIBGS_HTTP_DETAIL_STATIC_STRING(_type, ...) \
	static constexpr const _type *colon                 = __VA_ARGS__##": "                             ; \
	static constexpr const _type *line_break            = __VA_ARGS__##"\r\n"                           ; \
	static constexpr const _type *content_type          = __VA_ARGS__##"text/plain; charset=utf-8"      ; \
	static constexpr const _type *set_cookie            = __VA_ARGS__##"set-cookie"                     ; \
	static constexpr const _type *bytes                 = __VA_ARGS__##"bytes"                          ; \
	static constexpr const _type *bytes_start           = __VA_ARGS__##"bytes="                         ; \
	static constexpr const _type *range_format          = __VA_ARGS__##"{}-{},"                         ; \
	static constexpr const _type *content_range_format  = __VA_ARGS__##"{}-{}/{}"                       ; \
	static constexpr const _type *content_type_boundary = __VA_ARGS__##"multipart/byteranges; boundary="; \

template <>
struct _response_helper_static_string<char> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(char);
};

template <>
struct _response_helper_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STATIC_STRING(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STATIC_STRING

} //namespace detail

template <concept_char_type CharT>
class basic_response_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using fpos_t = std::ifstream::pos_type;

	using helper_type = basic_response_helper<CharT>;
	using str_list_type = basic_string_list<CharT>;

	using key_static_string = detail::_key_static_string<CharT>;
	using static_string = detail::_response_helper_static_string<CharT>;

public:
	explicit impl(helper_type *q_ptr) : q_ptr(q_ptr) {}
	impl(helper_type *q_ptr, str_view_type version, const headers_type &request_headers) :
		q_ptr(q_ptr), m_version(version), m_request_headers(request_headers) {}

public:
	[[nodiscard]] awaitable<size_t> write_header(size_t body_size, error_code &error)
	{
		m_headers_writed = true;
		auto it = m_response_headers.find(header_type::content_length);

		if( it == m_response_headers.end() )
		{
			if( stoi32(m_version) > 1.0 )
			{
				it = m_response_headers.find(header_type::transfer_encoding);
				if( it == m_response_headers.end() or str_to_lower(it->second.to_string()) != key_static_string::chunked )
					q_ptr->set_header(header_type::content_length, body_size);
			}
			else
				q_ptr->set_header(header_type::content_length, body_size);
		}
		std::string buf;
		buf.reserve(4096);

		buf = std::format("HTTP/{} {} {}\r\n", xxtombs<CharT>(m_version), m_status, to_status_description(m_status));
		m_response_headers.erase(static_string::set_cookie);

		for(auto &[key,value] : m_response_headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value.to_string()) + "\r\n";

		for(auto &[ckey,cookie] : m_cookies)
		{
			buf += "set-cookie: " + xxtombs<CharT>(ckey) + "=" + xxtombs<CharT>(cookie.value().to_string()) + ";";
			for(auto &[akey,attr] : cookie.attributes())
				buf += xxtombs<CharT>(akey) + "=" + xxtombs<CharT>(attr.to_string()) + ";";

			buf.pop_back();
			buf += "\r\n";
		}
		co_return co_await m_writer(buf + "\r\n", error);
	}

	[[nodiscard]] awaitable<size_t> write_body(const void *buf, size_t size, error_code &error)
	{
		if( not is_chunked() )
			co_return co_await m_writer({static_cast<const char*>(buf), size}, error);

		else if( m_chunk_attributes.empty() )
		{
			size = co_await m_writer(std::format("{:X}\r\n", size), error);
			if( error )
				co_return size;
		}
		else
		{
			std::string attributes;
			for(auto &attr : m_chunk_attributes)
				attributes += xxtombs<CharT>(attr.to_string()) + ";";

			m_chunk_attributes.clear();
			attributes.pop_back();

			size = co_await m_writer(std::format("{:X}; {}\r\n", size, attributes), error);
			if( error )
				co_return size;
		}
		co_return size + co_await m_writer(std::string(static_cast<const char*>(buf), size) + "\r\n", error);
	}

	[[nodiscard]] awaitable<size_t> send_file(std::string_view file_name, const http::ranges &ranges,  error_code &error)
	{
		namespace fs = std::filesystem;
		if( not fs::exists(file_name) )
		{
			throw system_error(std::make_error_code(static_cast<std::errc>(errc::no_such_device)),
							   "libgs::http::response_helper::write('{}')", file_name);
		}
		std::ifstream file(file_name.data(), std::ios_base::in | std::ios_base::binary);
		if( not file.is_open() )
		{
			throw system_error(std::make_error_code(static_cast<std::errc>(errno)),
							   "libgs::http::response_helper::write('{}')", file_name);
		}
		size_t res = 0;
		std::exception *exp = nullptr;
		try {
			if( ranges.empty() )
			{
				auto it = m_request_headers.find(header_type::range);
				if( it != m_request_headers.end() )
					res = co_await range_transfer(file_name, file, it->second, error);
				else
					res = co_await default_transfer(file_name, file, error);
			}
			else
			{
				str_type range_text = static_string::bytes_start;
				for(auto &range : ranges)
				{
					if( range.begin >= range.end )
						range_text += std::format(static_string::range_format, range.begin, range.end);
					else
					{
						throw system_error(std::make_error_code(static_cast<std::errc>(errc::invalid_argument)),
										   "gts::response::write_file: range error: begin > end");
					}
				}
				range_text.pop_back();
				res = co_await range_transfer(file_name, file, range_text, error);
			}
		}
		catch(std::exception &ex) {
			exp = &ex;
		}
		file.close();
		if( exp )
			throw *exp;
		co_return res;
	}

private:
	[[nodiscard]] bool is_chunked() const
	{
		if( stoi32(m_version) < 1.1 )
			return false;

		auto it = m_request_headers.find(header_type::transfer_encoding);
		return it != m_request_headers.end() and str_to_lower(it->second.to_string()) == key_static_string::chunked;
	}

private:
	awaitable<size_t> default_transfer(std::string_view file_name, std::ifstream &file, error_code &error)
	{
		using namespace std::chrono_literals;
		namespace fs = std::filesystem;

		auto mime_type = get_mime_type(file_name);
		libgs_log_debug("resource mime-type: {}.", mime_type);

		auto file_size = fs::file_size(file_name);
		if( file_size == 0 )
			co_return 0;

		q_ptr->set_header(header_type::content_type, mbstoxx<CharT>(mime_type));
		auto sum = co_await write_header(file_size, error);
		if( error )
			co_return sum;

		fpos_t buf_size = m_get_write_buffer_size ? m_get_write_buffer_size() : 65536;
		char *fr_buf = new char[buf_size] {0};

		auto is_chunked = this->is_chunked();
		while( not file.eof() )
		{
			file.read(fr_buf, buf_size);
			auto size = file.gcount();

			if( size == 0 or not write_body(fr_buf, size, is_chunked) )
				break;
			co_await co_sleep_for(512us);
		}
		delete[] fr_buf;
	}

private:
	struct range_value : range
	{
		std::string cr_line;
		std::size_t size = 0;
	};

	bool range_parsing(std::string_view file_name, str_view_type range_str, range_value &rv)
	{
		rv.size = 0;
		auto list = str_list_type::from_string(range_str, '-', false);

		if( list.size() > 2 )
		{
			libgs_log_debug("Range format error: {}.", range_str);
			return false;
		}
		namespace fs = std::filesystem;
		auto file_size = fs::file_size(file_name);

		if( list[0].empty() )
		{
			if( list[1].empty() )
			{
				libgs_log_debug("Range format error.");
				return false;
			}
			rv.size = ston_or<size_t>(list[1]);

			if( rv.size == 0 or rv.size > file_size )
			{
				libgs_log_debug("Range size is invalid.");
				return false;
			}
			rv.end = file_size - 1;
			rv.begin = file_size - rv.size;
		}
		else if( list[1].empty() )
		{
			if( list[0].empty() )
			{
				libgs_log_debug("Range format error.");
				return false;
			}
			rv.begin = ston_or<size_t>(list[0]);
			rv.end = file_size - 1;

			if( rv.begin > rv.end )
			{
				libgs_log_debug("Range is invalid.");
				return false;
			}
			rv.size = file_size - rv.begin;
		}
		else
		{
			rv.begin = ston_or<size_t>(list[0]);
			rv.end = ston_or<size_t>(list[1]);

			if( rv.begin > rv.end or rv.end >= file_size )
			{
				libgs_log_debug("Range is invalid.");
				return false;
			}
			rv.size = rv.end - rv.begin + 1;
		}
		return true;
	}

	awaitable<size_t> send_range(std::ifstream &file,
								 std::string_view boundary,
								 std::string_view ct_line,
								 std::list<range_value> &range_value_queue,
								 error_code &error)
	{
		using namespace std::chrono_literals;

		assert(not range_value_queue.empty());
		auto sum = co_await write_header();

		fpos_t buf_size = m_get_write_buffer_size ? m_get_write_buffer_size() : 65536;
		if( range_value_queue.size() == 1 )
		{
			auto &value = range_value_queue.back();
			file.seekg(value.begin, std::ios_base::beg);

			auto buf = new char[buf_size] {0};
			while( not file.eof() )
			{
				if( value.size <= buf_size )
				{
					file.read(buf, value.size);
					auto size = file.gcount();

					sum += co_await write_body(buf, size, error);
					if( error )
						break;

					co_await co_sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += write_body(buf, size, error);
				if( error )
					break;

				value.size -= buf_size;
				co_await co_sleep_for(512us);
			}
			delete[] buf;
			co_return sum;
		}
		else if( range_value_queue.empty() )
			co_return 0;

		auto buf = new char[buf_size] {0};
		auto sp_buf = std::shared_ptr<char>(buf); LIBGS_UNUSED(sp_buf);

		for(auto &value : range_value_queue)
		{
			std::string body;
			body.reserve(2 + boundary.size() + 2 +
						 ct_line.size() + 2 +
						 value.cr_line.size() + 2 +
						 2);

			body.append("--").append(boundary).append("\r\n")
				.append(ct_line).append("\r\n")
				.append(value.cr_line).append("\r\n"
				                              "\r\n");

			sum += co_await write_body(body, error);
			if( error )
				co_return sum;

			file.seekg(value.begin, std::ios_base::beg);
			while( not file.eof() )
			{
				if( value.size <= buf_size )
				{
					file.read(buf, value.size);
					auto size = file.gcount();

					if( size == 0 )
						break;

					buf[size + 0] = '\r';
					buf[size + 1] = '\n';

					sum += co_await write_body(buf, size + 2, error);
					if( error )
						co_return sum;

					co_await co_sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += co_await write_body(buf, size, error);
				if( error )
					co_return sum;

				value.size -= buf_size;
				co_await co_sleep_for(512us);
			}
		}
		sum += co_await write_body("--" + std::string(boundary.data(), boundary.size()) + "--\r\n", error);
		co_return sum;
	}

	awaitable<size_t> range_transfer(std::string_view file_name, std::ifstream &file, str_view_type _range_str, error_code &error)
	{
		str_type range_str(_range_str.data(), _range_str.size());
		for(auto i=range_str.size(); i>0; i--)
		{
			if( range_str[i] == 0x20/*SPACE*/ )
				range_str.erase(i,1);
		}
		if( range_str.empty() )
		{
			q_ptr->set_status(status::bad_request);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

		// bytes=x-y, m-n, i-j ...
		else if( range_str.substr(0,6) != static_string::bytes_start )
		{
			q_ptr->set_status(status::range_not_satisfiable);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

		// x-y, m-n, i-j ...
		auto range_list_str = range_str.substr(6);
		if( range_list_str.empty() )
		{
			q_ptr->set_status(status::range_not_satisfiable);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

		// (x-y) ( m-n) ( i-j) ...
		auto range_list = str_list_type::from_string(range_list_str, 0x2C/*,*/);
		auto mime_type = get_mime_type(file_name);

		q_ptr->set_status(status::partial_content);
		std::list<range_value> range_value_list;

		if( range_list.size() == 1 )
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(range_list[0], range_value) )
			{
				q_ptr->set_status(status::range_not_satisfiable);
				co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
			}
			q_ptr->set_header(header_type::accept_ranges , static_string::bytes)
					.set_header(header_type::content_type  , mime_type)
					.set_header(header_type::content_length, range_value.size)
					.set_header(header_type::content_range , {static_string::content_range_format, range_value.begin, range_value.end, range_value.size});

			co_return co_await send_range(file, "", "", range_value_list, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}", __func__, duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

		q_ptr->set_header(header_type::content_type, static_string::content_type_boundary + boundary);

		auto ct_line = std::format("{}: {}", header::content_type, mime_type);
		std::size_t content_length = 0;

		for(auto &str : range_list)
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(str_trimmed(str), range_value) )
			{
				q_ptr->set_status(status::range_not_satisfiable);
				co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
			}
			namespace fs = std::filesystem;
			range_value.cr_line = std::format("{}: bytes {}-{}/{}", header::content_range,
											  range_value.begin, range_value.end, fs::file_size(file_name));
			/*
				--boundary<CR><LF>
				Content-Type: xxx<CR><LF>
				Content-Range: bytes 3-11/96<CR><LF>
				<CR><LF>
				012345678<CR><LF>
				--boundary<CR><LF>
				Content-Type: xxx<CR><LF>
				Content-Range: bytes 0-7/96<CR><LF>
				<CR><LF>
				01235467<CR><LF>
				--boundary--<CR><LF>
			*/
			content_length += 2 + boundary.size() + 2 +        // --boundary<CR><LF>
							  ct_line.size() + 2 +             // Content-Type: xxx<CR><LF>
							  range_value.cr_line.size() + 2 + // Content-Range: bytes 3-11/96<CR><LF>
							  2 +                              // <CR><LF>
							  range_value.size + 2;            // 012345678<CR><LF>
		}
		content_length += 2 + boundary.size() + 2 + 2;         // --boundary--<CR><LF>

		q_ptr->set_header(header_type::content_length, content_length)
				.set_header(header_type::accept_ranges , static_string::bytes);

		co_return co_await send_range(file, boundary, ct_line, range_value_list, error);
	}

public:
	helper_type *q_ptr;
	str_view_type m_version = key_static_string::v_1_1;
	const headers_type &m_request_headers;

	http::status m_status = status::ok;
	headers_type m_response_headers {
		{ header_type::content_type, static_string::content_type }
	};
	cookies_type m_cookies;
	value_list_type m_chunk_attributes;

	str_type m_redirect_url;
	write_callback m_writer {};
	std::function<size_t()> m_get_write_buffer_size {};

	bool m_headers_writed = false;
	bool m_chunk_end_writed = false;
};

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(str_view_type version, const headers_type &request_headers) :
	m_impl(new impl(this, version, request_headers))
{

}

template <concept_char_type CharT>
basic_response_helper<CharT>::~basic_response_helper()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(basic_response_helper &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl(&other);
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::operator=(basic_response_helper &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl(&other);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_status(uint32_t status)
{
	status_check(status);
	m_impl->m_status = static_cast<http::status>(status);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_status(http::status status)
{
	status_check(status);
	m_impl->m_status = status;
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_header(str_view_type key, value_type value) noexcept
{
	m_impl->m_response_headers[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_cookie(str_view_type key, cookie_type cookie) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(cookie);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_redirect(str_view_type url, redirect_type type)
{
	switch(type)
	{
#define X_MACRO(e,v) case redirect_type::e : set_status(v); break;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		default: throw runtime_error("libgs::http::response_helper::redirect: Invalid redirect type: '{}'.", type);
	}
	return set_header(header_type::location, {url.data(), url.size()});
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attribute(value_type attribute)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	m_impl->m_chunk_attributes.emplace_back(std::move(attribute));
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attributes(value_list_type attributes)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	for(auto &value : attributes)
		m_impl->m_chunk_attributes.emplace_back(std::move(value));
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::on_write
(write_callback writer, std::function<size_t()> get_write_buffer_size)
{
	m_impl->m_writer = std::move(writer);
	m_impl->m_get_write_buffer_size = std::move(get_write_buffer_size);
	return *this;
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(opt_token<error_code&> tk)
{
	if( m_impl->m_headers_writed )
		throw runtime_error("libgs::http::response_helper::write: The http protocol header is sent repeatedly.");
	co_return co_await write(std::format("LIBGS: {} ({})", to_status_description(status()), status()), tk);
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(buffer<std::string_view> body, opt_token<error_code&> tk)
{
	if( not m_impl->m_writer )
		throw runtime_error("libgs::http::response_helper::write: No writer");

	error_code error;
	std::variant<size_t,error_code> var;

	using namespace std::chrono_literals;
	if( m_impl->m_headers_writed )
	{
		if( body.size == 0 )
			throw runtime_error("libgs::http::response_helper::write: The http protocol header is sent repeatedly.");
		else if( m_impl->m_chunk_end_writed )
			throw runtime_error("libgs::http::response_helper::write: Chunk has been written.");

		if( tk.rtime == 0ns )
			var = co_await m_impl->write_body(body.data.data(), body.size, error);
		else
			var = co_await (m_impl->write_body(body.data.data(), body.size, error) or co_sleep_for(tk.rtime));
	}
	else
	{
		auto lambda_write = [&]() mutable -> awaitable<size_t>
		{
			auto sum = co_await m_impl->write_header(body.size, error);
			if( error )
				co_return sum;
			else if( body.size > 0 )
				sum += co_await m_impl->write_body(body.data.data(), body.size, error);
			co_return sum;
		};
		if( tk.rtime == 0ns )
			var = co_await lambda_write();
		else
			var = co_await (lambda_write() or co_sleep_for(tk.rtime));
	}
	size_t sum = 0;
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	else
		sum = std::get<0>(var);

	io::detail::check_error(tk.error, error, "libgs::http::response_helper::write");
	co_return sum;
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(std::string_view file_name, opt_token<ranges,error_code&> tk)
{
	if( not m_impl->m_writer )
		throw runtime_error("libgs::http::response_helper::write: No writer");
	else if( m_impl->m_headers_writed )
		throw runtime_error("libgs::http::response_helper::write: The http protocol header is sent repeatedly.");
	else if( m_impl->m_chunk_end_writed )
		throw runtime_error("libgs::http::response_helper::chunk_end: Chunk has been written.");

	using namespace std::chrono_literals;
	error_code error;
	size_t sum = 0;

	if( tk.rtime == 0ns )
		sum = co_await m_impl->send_file(file_name, tk.ranges, error);
	else
	{
		auto var = co_await (m_impl->send_file(file_name, tk.ranges, error) or co_sleep_for(tk.rtime));
		if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
		else
			sum = std::get<0>(var);
	}
	io::detail::check_error(tk.error, error, "libgs::http::response_helper::write");
	co_return sum;
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::chunk_end(opt_token<const headers_type&, error_code&> tk)
{
	if( not m_impl->m_headers_writed )
		throw runtime_error("libgs::http::response_helper::chunk_end: Http header hasn't been send.");

	auto it = m_impl->m_response_headers.find(header_type::content_length);
	if( it != m_impl->m_request_headers.end() )
		throw runtime_error("libgs::http::response_helper::chunk_end: 'Content-Length' has been set.");

	else if( m_impl->m_chunk_end_writed )
		throw runtime_error("libgs::http::response_helper::chunk_end: Chunk has been written.");

	it = m_impl->m_response_headers.find(header_type::transfer_encoding);
	if( it == m_impl->m_response_headers.end() or str_to_lower(it->second) != detail::_response_helper_static_string<CharT>::chunked )
		throw runtime_error("libgs::http::response_helper::chunk_end: 'Transfer-Coding: chunked' not set.");

	m_impl->m_chunk_end_writed = true;
	error_code error;
	size_t sum = 0;

	auto lambda_write = [&]() mutable -> awaitable<size_t>
	{
		sum += co_await m_impl->writer("0\r\n", error);
		if( error )
			co_return sum;

		std::string buf;
		if( tk.headers )
		{
			for(auto &[key,value] : tk.headers)
				buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value.to_string()) + "\r\n";
		}
		sum += co_await m_impl->writer(buf + "\r\n", error);
		co_return sum;
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_write();

	auto var = co_await (lambda_write() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::response_helper::chunk_end");
	co_return sum;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_response_helper<CharT>::version() const noexcept
{
	return m_impl->m_version;
}

template <concept_char_type CharT>
http::status basic_response_helper<CharT>::status() const noexcept
{
	return m_impl->m_status;
}

template <concept_char_type CharT>
const typename basic_response_helper<CharT>::headers_type &basic_response_helper<CharT>::headers() const noexcept
{
	return m_impl->m_response_headers;
}

template <concept_char_type CharT>
const typename basic_response_helper<CharT>::cookies_type &basic_response_helper<CharT>::cookies() const noexcept
{
	return m_impl->m_cookies;
}

template <concept_char_type CharT>
bool basic_response_helper<CharT>::headers_writed() const noexcept
{
	return m_impl->m_headers_writed;
}

template <concept_char_type CharT>
bool basic_response_helper<CharT>::chunk_end_writed() const noexcept
{
	return m_impl->m_chunk_end_writed;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_header(str_view_type key)
{
	m_impl->m_response_headers.erase({key.data(), key.size()});
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_cookie(str_view_type key)
{
	m_impl->m_cookies.erase({key.data(), key.size()});
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_chunk_attribute(const value_type &attributes)
{
	m_impl->m_chunk_attributes.erase(attributes);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::reset()
{
	m_impl->m_version = detail::_key_static_string<CharT>::v_1_1;
	m_impl->m_status = status::ok;

	m_impl->m_response_headers = {
		{ header_type::content_type, detail::_response_helper_static_string<CharT>::content_type }
	};
	m_impl->m_cookies.clear();
	m_impl->m_chunk_attributes.clear();
	m_impl->m_redirect_url.clear();

	m_impl->m_chunk_end_writed = false;
	m_impl->m_headers_writed = false;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
