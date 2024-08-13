
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H

#include <libgs/core/algorithm/uuid.h>
#include <spdlog/spdlog.h>

namespace libgs::http
{

template <concept_tcp_stream Stream, concept_char_type CharT>
class basic_server_response<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

	using response_t = basic_server_response<Stream, CharT>;
	using string_list_t = basic_string_list<CharT>;
	using static_string = detail::_response_helper_static_string<CharT>;

public:
	impl(next_layer_t &&next_layer) :
		m_helper(next_layer.version(), next_layer.headers()),
		m_next_layer(std::move(next_layer)) {}

	template <typename Stream0>
	impl &operator=(typename basic_server_response<Stream0,CharT>::impl &&other) noexcept
	{
		m_helper = std::move(other.m_helper);
		m_next_layer = std::move(other.m_next_layer);
		m_headers_writed = other.m_headers_writed;
		m_chunk_end_writed = other.m_chunk_end_writed;

		other.m_headers_writed = false;
		other.m_chunk_end_writed = false;
		return *this;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_helper = std::move(other.m_helper);
		m_next_layer = std::move(other.m_next_layer);
		m_headers_writed = other.m_headers_writed;
		m_chunk_end_writed = other.m_chunk_end_writed;

		other.m_headers_writed = false;
		other.m_chunk_end_writed = false;
		return *this;
	}

public:
	inline void set_header(string_view_t key, value_t value) {
		m_helper.set_header(key, std::move(value));
	}

	inline void set_status(uint32_t status) {
		m_helper.set_status(status);
	}

	inline void set_status(http::status status) {
		m_helper.set_status(status);
	}

public:
	template <typename Token>
	auto write_x(const const_buffer &body, Token &&token, const char *func)
	{
		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			token.assign(0, std::system_category());
			if( m_headers_writed )
			{
				write_runtime_error_check(body, func);
				return write_body_x(body, token);
			}
			auto sum = write_header_x(body.size());
			if( body.size() > 0 )
				sum += write_body_x(body);
			return sum;
		}
		else
		{
			return co_spawn([this, &body, &token, func]() mutable -> awaitable<size_t>
			{
				token.ec_ = error_code();
				if( m_headers_writed )
				{
					write_runtime_error_check(body, func);
					co_return co_await write_body_x(body, std::forward<Token>(token));
				}
				size_t sum = co_await write_header_x(body.size(), std::forward<Token>(token));
				if( body.size() > 0 )
					sum += co_await write_body_x(body, std::forward<Token>(token));
				co_return sum;
			},
			m_next_layer.get_executor());
		}
	}

	template <typename Token>
	auto send_file_x(std::string_view _file_name, const resp_ranges &ranges, Token &&token, const char *func)
	{
		namespace fs = std::filesystem;
		if( m_headers_writed )
			throw runtime_error("libgs::http::server_response::{}: The http protocol header is sent repeatedly.", func);

		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			size_t sum = 0;
			auto file_name = app::absolute_path(token, _file_name);
			if( token )
				return sum;
			else if( not fs::exists(file_name) )
			{
				token = std::make_error_code(std::errc::no_such_file_or_directory);
				return sum;
			}
			std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
			if( not file.is_open())
			{
				token = std::make_error_code(static_cast<std::errc>(errno));
				return sum;
			}
			else if( ranges.empty() )
			{
				auto it = m_next_layer.headers().find(header_t::range);
				if( it != m_next_layer.headers().end() )
					sum = range_transfer_x(file_name, file, it->second.to_string(), token, func);
				else
					sum = default_transfer_x(file_name, file, token);
			}
			else
			{
				string_t range_text = static_string::bytes_start;
				for(auto &range: ranges)
				{
					if( range.begin >= range.end )
						range_text += std::format(static_string::range_format, range.begin, range.end);
					else
						throw runtime_error("libgs::http::server_response::{}: range error: begin > end", func);
				}
				range_text.pop_back();
				sum = range_transfer_x(file_name, file, range_text, token, func);
			}
			file.close();
			return sum;
		}
		else
		{
			return co_spawn([this, _file_name, &ranges, &token, func]() mutable -> awaitable<size_t>
			{
				size_t sum = 0;
				error_code error;
				auto file_name = app::absolute_path(error, _file_name);

				if( not token_check(token, error) )
					co_return sum;
				else if( not fs::exists(file_name) )
				{
					token_check(token, std::make_error_code(std::errc::no_such_file_or_directory));
					co_return sum;
				}
				std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
				if( not file.is_open())
				{
					token_check(token, std::make_error_code(static_cast<std::errc>(errno)));
					co_return sum;
				}
				else if( ranges.empty() )
				{
					auto it = m_next_layer.headers().find(header_t::range);
					if( it != m_next_layer.headers().end() )
						sum = co_await range_transfer_x(file_name, file, it->second.to_string(), std::forward<Token>(token), func);
					else
						sum = co_await default_transfer_x(file_name, file, std::forward<Token>(token));
				}
				else
				{
					string_t range_text = static_string::bytes_start;
					for(auto &range: ranges)
					{
						if( range.begin >= range.end )
							range_text += std::format(static_string::range_format, range.begin, range.end);
						else
							throw runtime_error("libgs::http::response::{}: range error: begin > end", func);
					}
					range_text.pop_back();
					sum = co_await range_transfer_x(file_name, file, range_text, std::forward<Token>(token), func);
				}
				file.close();
				co_return sum;
			});
		}
	}

	template <typename Token>
	auto chunk_end_x(const headers_t &headers, Token &&token, const char *func)
	{
		if( not m_headers_writed )
			throw runtime_error("libgs::http::server_response::{}: Http header hasn't been send.", func);

		auto it = m_helper.headers().find(header_t::content_length);
		if( it != m_helper.headers().end() )
			throw runtime_error("libgs::http::server_response::{}: 'Content-Length' has been set.", func);

		else if( m_chunk_end_writed )
			throw runtime_error("libgs::http::server_response::{}: Chunk has been written.", func);

		it = m_helper.headers().find(header_t::transfer_encoding);
		if( it == m_helper.headers().end() or
			str_to_lower(it->second.to_string()) != detail::string_pool<CharT>::chunked )
			throw runtime_error("libgs::http::server_response::{}: 'Transfer-Coding: chunked' not set.", func);

		std::string buf = "0\r\n";
		for(auto &[key,value] : headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value.to_string()) + "\r\n";
		buf += "\r\n";

		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			auto res = write_body_x(buffer(buf, buf.size()), token);
			if( not token )
				m_chunk_end_writed = true;
			return res;
		}
		else
		{
			return co_spawn([this, token = std::forward<Token>(token), buf = std::move(buf)]() mutable -> awaitable<size_t>
			{
				auto res = co_await write_body_x(buffer(buf, buf.size()), std::forward<Token>(token));
				if( not token.ec_ )
					m_chunk_end_writed = true;
				co_return res;
			});
		}
	}

	size_t check_time_out(const auto &var, error_code &error) const
	{
		if( var.index() == 0 )
			return std::get<0>(var);
		error = asio::error::make_error_code(errc::timed_out);
		return 0;
	}

private:
	template <typename Token>
	auto default_transfer_x(std::string_view file_name, std::ifstream &file, Token &&token) noexcept
	{
		using namespace std::chrono_literals;
		namespace fs = std::filesystem;

		auto mime_type = get_mime_type(file_name);
		spdlog::debug("resource mime-type: {}.", mime_type);

		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			size_t sum = 0;
			auto file_size = fs::file_size(file_name);

			if( file_size == 0 )
				return sum;

			set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
			sum += write_header_x(file_size, token);
			if( token )
				return sum;

			constexpr size_t buf_size = 0xFFFF;
			char fr_buf[buf_size]{0};

			while(not file.eof())
			{
				file.read(fr_buf, buf_size);
				auto size = file.gcount();
				if( size == 0 )
					break;

				sum += write_body_x(buffer(fr_buf, size), token);
				if( token )
					break;
//				sleep_for(512us);
			}
			return sum;
		}
		else
		{
			return co_spawn([this, file_name, &file, &token]() mutable -> awaitable<size_t>
			{
				auto mime_type = get_mime_type(file_name);
				spdlog::debug("resource mime-type: {}.", mime_type);

				size_t sum = 0;
				auto file_size = fs::file_size(file_name);

				if( file_size == 0 )
					co_return sum;

				set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
				sum += co_await write_header_x(file_size, std::forward<Token>(token));

				if( token.ec_ )
					co_return sum;

				constexpr size_t buf_size = 0xFFFF;
				char fr_buf[buf_size]{0};

				while(not file.eof())
				{
					file.read(fr_buf, buf_size);
					auto size = file.gcount();
					if( size == 0 )
						break;

					sum += co_await write_body_x(buffer(fr_buf, size), std::forward<Token>(token));
					if( token.ec_ )
						break;
//					co_await co_sleep_for(512us, get_executor());
				}
				co_return sum;
			});
		}
	}

	template <typename Token> auto range_transfer_x
	(std::string_view file_name, std::ifstream &file, string_view_t _range_str, Token &&token, const char *func)
	{
		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			string_t range_str(_range_str.data(), _range_str.size());
			for(auto i=range_str.size(); i>0; i--)
			{
				if( range_str[i] == 0x20/*SPACE*/ )
					range_str.erase(i,1);
			}
			auto status = m_helper.status();
			if( range_str.empty())
			{
				set_status(status::bad_request);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return write_x(buffer(buf, buf.size()), token, func);
			}

			// bytes=x-y, m-n, i-j ...
			else if( range_str.substr(0, 6) != static_string::bytes_start )
			{
				set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return write_x(buffer(buf, buf.size()), token, func);
			}

			// x-y, m-n, i-j ...
			auto range_list_str = range_str.substr(6);
			if( range_list_str.empty())
			{
				set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return write_x(buffer(buf, buf.size()), token, func);
			}

			// (x-y) ( m-n) ( i-j) ...
			auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
			auto mime_type = get_mime_type(file_name);

			set_status(status::partial_content);
			std::list<range_value> range_value_list;

			if( range_list.size() == 1 )
			{
				range_value_list.emplace_back();
				auto &range_value = range_value_list.back();

				if( not range_parsing(file_name, range_list[0], range_value))
				{
					set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					return write_x(buffer(buf, buf.size()), token, func);
				}
				set_header(header_t::accept_ranges, static_string::bytes);
				set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
				set_header(header_t::content_length, range_value.size);
				set_header(header_t::content_range,
						   {static_string::content_range_format, range_value.begin, range_value.end, range_value.size});

				return send_range_x(file, "", "", std::move(range_value_list), token);
			} // if( rangeList.size() == 1 )

			using namespace std::chrono;
			auto boundary = std::format("{}_{}", uuid::generate().to_string(),
										duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

			set_header(header_t::content_type, static_string::content_type_boundary + mbstoxx<CharT>(boundary));

			auto ct_line = std::format("{}: {}", header::content_type, mime_type);
			std::size_t content_length = 0;

			for(auto &str: range_list)
			{
				range_value_list.emplace_back();
				auto &range_value = range_value_list.back();

				if( not range_parsing(file_name, str_trimmed(str), range_value))
				{
					set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					return write_x(buffer(buf, buf.size()), token, func);
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

			set_header(header_t::content_length, content_length);
			set_header(header_t::accept_ranges, static_string::bytes);
			return send_range_x(file, boundary, ct_line, std::move(range_value_list), token);
		}
		else
		{
			return co_spawn([this, file_name, &file, _range_str, &token, func]() mutable -> awaitable<size_t>
			{
				string_t range_str(_range_str.data(), _range_str.size());
				for(auto i=range_str.size(); i>0; i--)
				{
					if( range_str[i] == 0x20/*SPACE*/ )
						range_str.erase(i,1);
				}
				auto status = m_helper.status();
				if( range_str.empty())
				{
					set_status(status::bad_request);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await write_x(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// bytes=x-y, m-n, i-j ...
				else if( range_str.substr(0, 6) != static_string::bytes_start )
				{
					set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await write_x(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// x-y, m-n, i-j ...
				auto range_list_str = range_str.substr(6);
				if( range_list_str.empty())
				{
					set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await write_x(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// (x-y) ( m-n) ( i-j) ...
				auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
				auto mime_type = get_mime_type(file_name);

				set_status(status::partial_content);
				std::list<range_value> range_value_list;

				if( range_list.size() == 1 )
				{
					range_value_list.emplace_back();
					auto &range_value = range_value_list.back();

					if( not range_parsing(file_name, range_list[0], range_value))
					{
						set_status(status::range_not_satisfiable);
						auto buf = std::format("{} ({})", to_status_description(status), status);
						co_return co_await write_x(buffer(buf, buf.size()), std::forward<Token>(token), func);
					}
					set_header(header_t::accept_ranges, static_string::bytes);
					set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
					set_header(header_t::content_length, range_value.size);
					set_header(header_t::content_range,
							   {static_string::content_range_format, range_value.begin, range_value.end, range_value.size});

					co_return co_await send_range_x
					(file, "", "", std::move(range_value_list), std::forward<Token>(token));
				} // if( rangeList.size() == 1 )

				using namespace std::chrono;
				auto boundary = std::format("{}_{}", uuid::generate().to_string(),
											duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

				set_header(header_t::content_type, static_string::content_type_boundary + mbstoxx<CharT>(boundary));

				auto ct_line = std::format("{}: {}", header::content_type, mime_type);
				std::size_t content_length = 0;

				for(auto &str: range_list)
				{
					range_value_list.emplace_back();
					auto &range_value = range_value_list.back();

					if( not range_parsing(file_name, str_trimmed(str), range_value))
					{
						set_status(status::range_not_satisfiable);
						auto buf = std::format("{} ({})", to_status_description(status), status);
						co_return co_await write_x(buffer(buf, buf.size()), std::forward<Token>(token), func);
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

				set_header(header_t::content_length, content_length);
				set_header(header_t::accept_ranges, static_string::bytes);

				co_return co_await send_range_x
				(file, boundary, ct_line, std::move(range_value_list), std::forward<Token>(token));
			});
		}
	}

private:
	struct range_value : resp_range
	{
		std::string cr_line{};
		std::size_t size = 0;
	};

	bool range_parsing(std::string_view file_name, string_view_t range_str, range_value &rv) noexcept
	{
		rv.size = 0;
		auto list = string_list_t::from_string(range_str, '-', false);

		if( list.size() > 2 )
		{
			spdlog::debug("Range format error: {}.", xxtombs<CharT>(range_str));
			return false;
		}
		namespace fs = std::filesystem;
		auto file_size = fs::file_size(file_name);

		if( list[0].empty())
		{
			if( list[1].empty())
			{
				spdlog::debug("Range format error.");
				return false;
			}
			rv.size = ston_or<size_t>(list[1]);

			if( rv.size == 0 or rv.size > file_size )
			{
				spdlog::debug("Range size is invalid.");
				return false;
			}
			rv.end = file_size - 1;
			rv.begin = file_size - rv.size;
		}
		else if( list[1].empty())
		{
			if( list[0].empty())
			{
				spdlog::debug("Range format error.");
				return false;
			}
			rv.begin = ston_or<size_t>(list[0]);
			rv.end = file_size - 1;

			if( rv.begin > rv.end )
			{
				spdlog::debug("Range is invalid.");
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
				spdlog::debug("Range is invalid.");
				return false;
			}
			rv.size = rv.end - rv.begin + 1;
		}
		return true;
	}

public:
	template <typename Token>
	auto send_range_x(std::ifstream &file,
					  std::string_view boundary,
					  std::string_view ct_line,
					  std::list<range_value> range_value_queue,
					  Token &&token) noexcept
	{
		using namespace std::chrono_literals;
		assert(not range_value_queue.empty());

		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			auto sum = write_header_x(0, token);
			constexpr size_t buf_size = 0xFFFF;

			if( range_value_queue.size() == 1 )
			{
				auto &value = range_value_queue.back();
				file.seekg(value.begin, std::ios_base::beg);

				char buf[buf_size]{0};
				while(not file.eof())
				{
					if( value.size <= static_cast<size_t>(buf_size))
					{
						file.read(buf, value.size);
						auto size = file.gcount();

						sum += write_body_x(buffer(buf,size), token);
						if( token )
							break;

//						sleep_for(512us);
						break;
					}
					file.read(buf, buf_size);
					auto size = file.gcount();

					sum += write_body_x(buffer(buf,size), token);
					if( token )
						break;

					value.size -= buf_size;
//					sleep_for(512us);
				}
				return sum;
			}
			else if( range_value_queue.empty())
				return sum;

			char buf[buf_size] {0};
			for(auto &value: range_value_queue)
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

				sum += write_body_x(buffer(body, body.size()), token);
				if( token )
					return sum;

				file.seekg(value.begin, std::ios_base::beg);
				while(not file.eof())
				{
					if( value.size <= static_cast<size_t>(buf_size))
					{
						file.read(buf, value.size);
						auto size = file.gcount();

						if( size == 0 )
							break;

						buf[size + 0] = '\r';
						buf[size + 1] = '\n';

						sum += write_body_x(buffer(buf, size + 2), token);
						if( token )
							return sum;

//						sleep_for(512us);
						break;
					}
					file.read(buf, buf_size);
					auto size = file.gcount();

					sum += write_body_x(buffer(buf,size), token);
					if( token )
						return sum;

					value.size -= buf_size;
//					sleep_for(512us);
				}
			}
			auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
			sum += write_body_x(buffer(abuf, abuf.size()), token);
			return sum;
		}
		else
		{
			return co_spawn([this, &file, boundary, ct_line, &range_value_queue, &token]() mutable -> awaitable<size_t>
			{
				auto sum = co_await write_header_x(0, std::forward<Token>(token));
				constexpr size_t buf_size = 0xFFFF;

				if( range_value_queue.size() == 1 )
				{
					auto &value = range_value_queue.back();
					file.seekg(value.begin, std::ios_base::beg);

					char buf[buf_size]{0};
					while(not file.eof())
					{
						if( value.size <= static_cast<size_t>(buf_size))
						{
							file.read(buf, value.size);
							auto size = file.gcount();

							sum += co_await write_body_x(buffer(buf,size), std::forward<Token>(token));
							if( token.ec_ )
								break;

//							co_await co_sleep_for(512us, get_executor());
							break;
						}
						file.read(buf, buf_size);
						auto size = file.gcount();

						sum += co_await write_body_x(buffer(buf,size), std::forward<Token>(token));
						if( token.ec_ )
							break;

						value.size -= buf_size;
//						co_await co_sleep_for(512us, get_executor());
					}
					co_return sum;
				}
				else if( range_value_queue.empty())
					co_return sum;

				char buf[buf_size] {0};
				for(auto &value: range_value_queue)
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

					sum += co_await write_body_x(buffer(body, body.size()), std::forward<Token>(token));
					if( token.ec_ )
						co_return sum;

					file.seekg(value.begin, std::ios_base::beg);
					while(not file.eof())
					{
						if( value.size <= static_cast<size_t>(buf_size))
						{
							file.read(buf, value.size);
							auto size = file.gcount();

							if( size == 0 )
								break;

							buf[size + 0] = '\r';
							buf[size + 1] = '\n';

							sum += co_await write_body_x(buffer(buf, size + 2), std::forward<Token>(token));
							if( token.ec_ )
								co_return sum;

//							co_await co_sleep_for(512us, get_executor());
							break;
						}
						file.read(buf, buf_size);
						auto size = file.gcount();

						sum += co_await write_body_x(buffer(buf,size), std::forward<Token>(token));
						if( token.ec_ )
							co_return sum;

						value.size -= buf_size;
//						co_await co_sleep_for(512us, get_executor());
					}
				}
				auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
				sum += co_await write_body_x(buffer(abuf, abuf.size()), std::forward<Token>(token));
				co_return sum;
			});
		}
	}

private:
	template <typename Token>
	auto write_header_x(size_t size, Token &&token) noexcept {
		return write_funcdata_x(m_helper.header_data(size), true, std::forward<Token>(token));
	}

	template <typename Token>
	auto write_body_x(const const_buffer &body, Token &&token) noexcept {
		return write_funcdata_x(m_helper.body_data(body), false, std::forward<Token>(token));
	}

	template <typename Token>
	auto write_funcdata_x(std::string &&data, bool wheader, Token &&token) noexcept
	{
		if constexpr( std::is_same_v<std::decay_t<Token>, error_code> )
		{
			size_t sum = 0;
			do {
				m_next_layer.next_layer().non_blocking(false, token);
				if( token )
					break;

				sum += asio::write(m_next_layer.next_layer(), buffer(data.c_str() + sum, data.size() - sum), token);
				if( token and token.value() == errc::interrupted )
					continue;
			}
			while(false);
			if( wheader and not token )
				m_headers_writed = true;
			return sum;
		}
		else
		{
			return co_spawn([this, data = std::move(data), wheader, &token]() mutable -> awaitable<size_t>
			{
				token.ec_ = error_code();
				size_t sum = 0;
				do {
					sum += co_await asio::async_write(
							m_next_layer.next_layer(),
							buffer(data.c_str() + sum, data.size() - sum),
							token);

					if( token.ec_ and token.ec_.value() == errc::interrupted )
						continue;
				}
				while(false);
				if( token_check(token, token.ec_) and wheader )
					m_headers_writed = true;
				co_return sum;
			});
		}
	}

private:
	bool token_check(auto &token, const error_code &error) noexcept
	{
		if( not error )
			return true;
		token.ec_ = error;
		return false;
	}

	void write_runtime_error_check(const const_buffer &body, const char *func)
	{
		if( body.size() == 0 )
			throw runtime_error("libgs::http::response::{}: The http protocol header is sent repeatedly.", func);
		else if( m_chunk_end_writed )
			throw runtime_error("libgs::http::response::{}: Chunk has been written.", func);
	}

public:
	helper_t m_helper;
	next_layer_t m_next_layer;

	bool m_headers_writed = false;
	bool m_chunk_end_writed = false;
};

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT>::basic_server_response(next_layer_t &&next_layer) :
	m_impl(new impl(std::move(next_layer)))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT>::~basic_server_response()
{
	delete m_impl;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT>::basic_server_response(basic_server_response &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::operator=
(basic_server_response &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <typename Stream0>
basic_server_response<Stream,CharT>::basic_server_response
(basic_server_response<Stream0,CharT> &&other) noexcept
	requires concept_constructible<next_layer_t,basic_server_request<Stream0,CharT>&&> :
	m_impl(new impl(this, std::move(*other.m_impl)))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <typename Stream0>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::operator=
(basic_server_response<Stream0,CharT> &&other) noexcept requires concept_assignable<Stream,Stream0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_status(uint32_t status)
{
	m_impl->set_status(status);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_status(http::status status)
{
	m_impl->set_status(status);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_header(string_view_t key, value_t value)
{
	m_impl->set_header(key, std::move(value));
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_cookie(string_view_t key, cookie_t cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::write(const const_buffer &body)
{
	error_code error;
	auto res = write(body, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::write");
	return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::write(const const_buffer &body, error_code &error)
{
	return m_impl->write_x(body, error, "write");
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::write(error_code &error)
{
	return write({nullptr,0}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_write(const const_buffer &body)
{
	error_code error;
	auto res = co_await co_write(body, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::co_write");
	co_return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_write(const const_buffer &body, error_code &error)
{
	using namespace std::chrono_literals;
	auto var = co_await(m_impl->write_x(body, use_awaitable|error, "co_write") or
						co_sleep_for(30s, get_executor()));
	co_return m_impl->check_time_out(var, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_write(error_code &error)
{
	co_return co_await co_write({nullptr,0}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::redirect(string_view_t url, http::redirect redi)
{
	error_code error;
	auto res = redirect(url, redi, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::redirect");
	return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::redirect(string_view_t url, http::redirect redi, error_code &error)
{
	m_impl->m_helper.set_redirect(url, redi);
	return m_impl->write_x({nullptr,0}, error, "redirect");
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::redirect(string_view_t url, error_code &error)
{
	return redirect(url, http::redirect::moved_permanently, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_redirect(string_view_t url, http::redirect redi)
{
	error_code error;
	auto res = co_await co_redirect(url, redi, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::co_redirect");
	co_return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_redirect
(string_view_t url, http::redirect redi, error_code &error)
{
	using namespace std::chrono_literals;
	m_impl->m_helper.set_redirect(url, redi);

	auto var = co_await (m_impl->write_x({nullptr,0}, use_awaitable|error, "co_redirect") or
						 co_sleep_for(30s, get_executor()));
	co_return m_impl->check_time_out(var, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_redirect(string_view_t url, error_code &error)
{
	co_return co_await co_redirect(url, http::redirect::moved_permanently, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::send_file(std::string_view file_name, const resp_ranges &ranges)
{
	error_code error;
	auto res = send_file(file_name, ranges, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::send_file");
	return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::send_file
(std::string_view file_name, const resp_ranges &ranges, error_code &error)
{
	return m_impl->send_file_x(file_name, ranges, error, "send_file");
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::send_file(std::string_view file_name, error_code &error)
{
	return send_file(file_name, {}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_send_file
(std::string_view file_name, const resp_ranges &ranges)
{
	error_code error;
	auto res = co_await co_send_file(file_name, ranges, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::co_send_file");
	co_return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_send_file
(std::string_view file_name, const resp_ranges &ranges, error_code &error)
{
	using namespace std::chrono_literals;
	auto var = co_await (m_impl->send_file_x(file_name, ranges, use_awaitable|error, "co_send_file") or
						 co_sleep_for(30s, get_executor()));
	co_return m_impl->check_time_out(var, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_send_file
(std::string_view file_name, error_code &error)
{
	co_return co_await co_send_file(file_name, {}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_chunk_attribute(value_t attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_chunk_attributes(value_list_t attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::chunk_end(const headers_t &headers)
{
	error_code error;
	auto res = chunk_end(headers, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::chunk_end");
	return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::chunk_end(const headers_t &headers, error_code &error)
{
	return m_impl->chunk_end_x(headers, error, "chunk_end");
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_response<Stream,CharT>::chunk_end(error_code &error)
{
	return chunk_end({}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_chunk_end(const headers_t &headers)
{
	error_code error;
	auto res = co_await co_chunk_end(headers, error);
	if( error )
		throw system_error(error, "libgs::http::server_response::co_chunk_end");
	co_return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_chunk_end(const headers_t &headers, error_code &error)
{
	using namespace std::chrono_literals;
	auto var = co_await (m_impl->chunk_end_x(headers, use_awaitable|error, "co_chunk_end") or
						 co_sleep_for(30s, get_executor()));
	co_return m_impl->check_time_out(var, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
awaitable<size_t> basic_server_response<Stream,CharT>::co_chunk_end(error_code &error)
{
	co_return co_await chunk_end({}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT>::string_view_t
basic_server_response<Stream,CharT>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
http::status basic_server_response<Stream,CharT>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_response<Stream,CharT>::headers_t&
basic_server_response<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_response<Stream,CharT>::cookies_t&
basic_server_response<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_response<Stream,CharT>::headers_writed() const noexcept
{
	return m_impl->m_headers_writed;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_response<Stream,CharT>::chunk_end_writed() const noexcept
{
	return m_impl->m_chunk_end_writed;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_response<Stream,CharT>::executor_t&
basic_server_response<Stream,CharT>::get_executor() noexcept
{
	return m_impl->m_next_layer.get_executor();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_header(string_view_t key)
{
	m_impl->m_helper.unset_header(key);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_helper.unset_cookie(key);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_chunk_attribute(const value_t &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_response<Stream,CharT>::next_layer_t&
basic_server_response<Stream,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_response<Stream,CharT>::next_layer_t&
basic_server_response<Stream,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
