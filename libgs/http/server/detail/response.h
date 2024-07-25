
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

namespace libgs::http
{

template <typename Request, concept_char_type CharT>
class basic_server_response<Request,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

	using response_t = basic_server_response<Request, CharT>;
	using string_list_t = basic_string_list<CharT>;
	using static_string = detail::_response_helper_static_string<CharT>;

public:
	impl(response_t *q_ptr, next_layer_t &&next_layer) :
			q_ptr(q_ptr), m_helper(next_layer.version(), next_layer.headers()),
			m_next_layer(std::move(next_layer)) {}

	impl(impl &&other) noexcept = default;

	impl &operator=(impl &&other) noexcept = default;

public:
	size_t write(const const_buffer &body, const char *func)
	{
		error_code error;
		auto res = write(body, error);
		if( error )
			throw system_error(error, "libgs::http::response::{}", func);
		return res;
	}

	size_t write(const const_buffer &body, error_code &error, const char *func)
	{
		error.assign(0, std::system_category());
		if( m_headers_writed )
		{
			write_runtime_error_check(body, func);
			return write_body(body, error);
		}
		auto sum = write_header(body.size(), error);
		if( body.size() > 0 )
			sum += write_body(body, error);
		return sum;
	}

	template<typename Token>
	auto async_write(const const_buffer &body, Token &&token, const char *func)
	{
		if constexpr( is_function_v<Token> )
		{
			co_spawn_detached([this, body, callback = std::move(token)]() -> awaitable<void>
			{
				error_code error;
				auto buf = co_await async_write(body, use_awaitable | error);
				callback(buf, error);
				co_return;
			},
			m_next_layer.get_executor());
		}
#ifdef LIBGS_USING_BOOST_ASIO
		else if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			if( token.ec_ )
				*token.ec_ = error_code();

			if( m_headers_writed )
			{
				write_runtime_error_check(body, "async_write");
				return async_write_body(body, std::forward<Token>(token), func);
			}
			size_t sum = async_write_header(body.size(), std::forward<Token>(token), func);
			if( body.size() > 0 )
				sum += async_write_body(body, std::forward<Token>(token), func);
			return sum;
		}
#endif //LIBGS_USING_BOOST_ASIO
		else
		{
			token_reset(token);
			return [=, this]() mutable -> awaitable<size_t>
			{
				if( m_headers_writed )
				{
					write_runtime_error_check(body, func);
					co_return co_await async_write_body(body, std::forward<Token>(token), func);
				}
				size_t sum = co_await async_write_header(body.size(), std::forward<Token>(token), func);
				if( body.size() > 0 )
					sum += co_await async_write_body(body, std::forward<Token>(token), func);
				co_return sum;
			}();
		}
	}

public:
	template<typename Token>
	void token_reset(Token &token)
	{
		if constexpr( is_function_v<Token> )
			return;
		else if constexpr( detail::concept_has_ec_<Token> )
			token.ec_ = error_code();
		else if constexpr( detail::concept_has_get<Token> )
		{
			if constexpr( detail::concept_has_ec_<decltype(token.get())> )
				token.get().ec_ = error_code();
		}
	}

	template<typename Token>
	void token_handle(Token &token, const error_code &error, const char *func)
	{
		if constexpr( is_function_v<Token> )
			return;
		else if constexpr( detail::concept_has_ec_<Token> )
		{
			token.ec_ = error;
			return;
		}
		else if constexpr( detail::concept_has_get<Token> )
		{
			if constexpr( detail::concept_has_ec_<decltype(token.get())> )
			{
				token.get().ec_ = error;
				return;
			}
		}
		throw system_error(error, "libgs::http::server_response::{}", func);
	}

	void write_runtime_error_check(const const_buffer &body, const char *func)
	{
		if( body.size() == 0 )
			throw runtime_error("libgs::http::response::{}: The http protocol header is sent repeatedly.", func);
		else if( m_chunk_end_writed )
			throw runtime_error("libgs::http::response::{}: Chunk has been written.", func);
	}

public:
	size_t send_file(std::string_view file_name, const resp_ranges &ranges, error_code &error, const char *func)
	{
		if( m_headers_writed )
			throw runtime_error("libgs::http::response::{}: The http protocol header is sent repeatedly.", func);

		namespace fs = std::filesystem;
		if( not fs::exists(file_name))
		{
			error = std::make_error_code(std::errc::no_such_file_or_directory);
			return 0;
		}
		std::ifstream file(file_name.data(), std::ios_base::in | std::ios_base::binary);
		if( not file.is_open())
		{
			error = std::make_error_code(static_cast<std::errc>(errno));
			return 0;
		}
		size_t res = 0;
		if( ranges.empty())
		{
			auto it = m_helper.headers().find(header_t::range);
			if( it != m_helper.headers().end())
				res = range_transfer(file_name, file, it->second.to_string(), error, func);
			else
				res = default_transfer(file_name, file, error);
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
			res = range_transfer(file_name, file, range_text, error, func);
		}
		file.close();
		return res;
	}

	template <typename Token>
	auto async_send_file(std::string_view file_name, const resp_ranges &ranges, Token &&token, const char *func)
	{
		namespace fs = std::filesystem;
		if( m_headers_writed )
			throw runtime_error("libgs::http::response::{}: The http protocol header is sent repeatedly.", func);

		if constexpr( is_function_v<Token> )
		{
			auto _file_name = std::string(file_name.data(), file_name.size());
			co_spawn_detached([this, _file_name = std::move(_file_name), ranges, callback = std::move(token), func]() -> awaitable<void>
			{
				error_code error;
				auto buf = co_await async_send_file(_file_name, ranges, use_awaitable|error, func);
				callback(buf, error);
				co_return;
			},
			m_next_layer.get_executor());
		}
#ifdef LIBGS_USING_BOOST_ASIO
		else if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			if( not fs::exists(file_name))
			{
				token_handle(token, std::make_error_code(std::errc::no_such_file_or_directory), func);
				return 0;
			}
			std::ifstream file(file_name.data(), std::ios_base::in | std::ios_base::binary);
			if( not file.is_open())
			{
				token_handle(token, std::make_error_code(static_cast<std::errc>(errno)), func);
				return 0;
			}
			size_t res = 0;
			if( ranges.empty())
			{
				auto it = m_helper.headers().find(header_t::range);
				if( it != m_helper.headers().end())
					res = async_range_transfer(file_name, file, it->second.to_string(), std::forward<Token>(token), func);
				else
					res = async_default_transfer(file_name, file, std::forward<Token>(token));
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
				res = async_range_transfer(file_name, file, range_text, std::forward<Token>(token), func);
			}
			file.close();
			return res;
		}
#endif //LIBGS_USING_BOOST_ASIO
		else
		{
			return [=,this]() mutable -> awaitable<size_t>
			{
				if( not fs::exists(file_name))
				{
					token_handle(token, std::make_error_code(std::errc::no_such_file_or_directory), func);
					co_return 0;
				}
				std::ifstream file(file_name.data(), std::ios_base::in | std::ios_base::binary);
				if( not file.is_open())
				{
					token_handle(token, std::make_error_code(static_cast<std::errc>(errno)), func);
					co_return 0;
				}
				size_t res = 0;
				if( ranges.empty())
				{
					auto it = m_helper.headers().find(header_t::range);
					if( it != m_helper.headers().end())
						res = co_await async_range_transfer(file_name, file, it->second.to_string(), std::forward<Token>(token), func);
					else
						res = co_await async_default_transfer(file_name, file, std::forward<Token>(token));
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
					res = co_await async_range_transfer(file_name, file, range_text, std::forward<Token>(token), func);
				}
				file.close();
				co_return res;
			}();
		}
	}

private:
	size_t default_transfer(std::string_view file_name, std::ifstream &file, error_code &error)
	{
		using namespace std::chrono_literals;
		namespace fs = std::filesystem;

		auto mime_type = get_mime_type(file_name);
		spdlog::debug("resource mime-type: {}.", mime_type);

		auto file_size = fs::file_size(file_name);
		if( file_size == 0 )
			return 0;

		q_ptr->set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
		auto sum = write_header(file_size, error);
		if( error )
			return sum;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size]{0};

		while(not file.eof())
		{
			file.read(fr_buf, buf_size);
			auto size = file.gcount();
			if( size == 0 )
				break;

			sum += write_body(buffer(fr_buf, size), error);
			if( error )
				break;

			sleep_for(512us);
		}
		return sum;
	}

	template <typename Token>
	auto async_default_transfer(std::string_view file_name, std::ifstream &file, Token &&token, const char *func)
	{
		using namespace std::chrono_literals;
		namespace fs = std::filesystem;

#ifdef LIBGS_USING_BOOST_ASIO
		if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			auto mime_type = get_mime_type(file_name);
			spdlog::debug("resource mime-type: {}.", mime_type);

			auto file_size = fs::file_size(file_name);
			if( file_size == 0 )
				return 0;

			q_ptr->set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
			auto sum = async_write_header(file_size, std::forward<Token>(token));

			if( token.ec_ and *token.ec_ )
				return sum;

			constexpr size_t buf_size = 0xFFFF;
			char fr_buf[buf_size]{0};

			while(not file.eof())
			{
				file.read(fr_buf, buf_size);
				auto size = file.gcount();
				if( size == 0 )
					break;

				sum += async_write_body(buffer(fr_buf, size), std::forward<Token>(token), func);
				if( token.ec_ and *token.ec_ )
					break;

				sleep_for(512us);
			}
			return sum;
		}
		else
#endif //LIBGS_USING_BOOST_ASIO
		{
			return [=,this]() mutable -> awaitable<size_t>
			{
				auto mime_type = get_mime_type(file_name);
				spdlog::debug("resource mime-type: {}.", mime_type);

				auto file_size = fs::file_size(file_name);
				if( file_size == 0 )
					co_return 0;

				q_ptr->set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
				auto sum = co_await async_write_header(file_size, std::forward<Token>(token), func);

				if( not check_token_error(token) )
					co_return sum;

				constexpr size_t buf_size = 0xFFFF;
				char fr_buf[buf_size]{0};

				while(not file.eof())
				{
					file.read(fr_buf, buf_size);
					auto size = file.gcount();
					if( size == 0 )
						break;

					sum += async_write_body(buffer(fr_buf, size), std::forward<Token>(token), func);
					if( not check_token_error(token) )
						break;

					co_await co_sleep_for(512us);
				}
				co_return sum;
			}();
		}
	}

private:
	struct range_value : resp_range
	{
		std::string cr_line{};
		std::size_t size = 0;
	};

	bool range_parsing(std::string_view file_name, string_view_t range_str, range_value &rv)
	{
		rv.size = 0;
		auto list = string_list_t::from_string(range_str, '-', false);

		if( list.size() > 2 )
		{
			spdlog::debug("Range format error: {}.", range_str);
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
	size_t send_range(std::ifstream &file,
					  std::string_view boundary,
					  std::string_view ct_line,
					  std::list<range_value> &range_value_queue,
					  error_code &error)
	{
		using namespace std::chrono_literals;
		assert(not range_value_queue.empty());

		auto sum = write_header(0,error);
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

					sum += write_body(buffer(buf,size), error);
					if( error )
						break;

					sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					break;

				value.size -= buf_size;
				sleep_for(512us);
			}
			return sum;
		}
		else if( range_value_queue.empty())
			return 0;

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

			sum += write_body(buffer(body, body.size()), error);
			if( error )
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

					sum += write_body(buffer(buf, size + 2), error);
					if( error )
						return sum;

					sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					return sum;

				value.size -= buf_size;
				sleep_for(512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		sum += write_body(buffer(abuf, abuf.size()), error);
		return sum;
	}

	size_t range_transfer
	(std::string_view file_name, std::ifstream &file, string_view_t _range_str, error_code &error, const char *func)
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
			q_ptr->set_status(status::bad_request);
			auto buf = std::format("{} ({})", to_status_description(status), status);
			return write(buffer(buf, buf.size()), error, func);
		}

		// bytes=x-y, m-n, i-j ...
		else if( range_str.substr(0, 6) != static_string::bytes_start )
		{
			q_ptr->set_status(status::range_not_satisfiable);
			auto buf = std::format("{} ({})", to_status_description(status), status);
			return write(buffer(buf, buf.size()), error, func);
		}

		// x-y, m-n, i-j ...
		auto range_list_str = range_str.substr(6);
		if( range_list_str.empty())
		{
			q_ptr->set_status(status::range_not_satisfiable);
			auto buf = std::format("{} ({})", to_status_description(status), status);
			return write(buffer(buf, buf.size()), error, func);
		}

		// (x-y) ( m-n) ( i-j) ...
		auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
		auto mime_type = get_mime_type(file_name);

		q_ptr->set_status(status::partial_content);
		std::list<range_value> range_value_list;

		if( range_list.size() == 1 )
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(file_name, range_list[0], range_value))
			{
				q_ptr->set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return write(buffer(buf, buf.size()), error, func);
			}
			q_ptr->set_header(header_t::accept_ranges, static_string::bytes)
				  .set_header(header_t::content_type, mime_type)
				  .set_header(header_t::content_length, range_value.size)
				  .set_header(header_t::content_range,
							  {static_string::content_range_format, range_value.begin, range_value.end,
							   range_value.size});

			return send_range(file, "", "", range_value_list, error, func);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}", __func__,
									duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

		q_ptr->set_header(header_t::content_type, static_string::content_type_boundary + boundary);

		auto ct_line = std::format("{}: {}", header::content_type, mime_type);
		std::size_t content_length = 0;

		for(auto &str: range_list)
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(file_name, str_trimmed(str), range_value))
			{
				q_ptr->set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return write(buffer(buf, buf.size()), error, func);
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

		q_ptr->set_header(header_t::content_length, content_length)
			  .set_header(header_t::accept_ranges, static_string::bytes);

		return send_range(file, boundary, ct_line, range_value_list, error, func);
	}

public:
	template <typename Token>
	auto async_send_range(std::ifstream &file,
						  std::string_view boundary,
						  std::string_view ct_line,
						  std::list<range_value> &range_value_queue,
						  Token &&token,
						  const char *func)
	{
		using namespace std::chrono_literals;
		assert(not range_value_queue.empty());

#ifdef LIBGS_USING_BOOST_ASIO
		if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			auto sum = async_write_header(0,std::forward<Token>(token), func);
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

						sum += async_write_body(buffer(buf,size), std::forward<Token>(token), func);
						if( token.ec_ and *token.ec_ )
							break;

						sleep_for(512us);
						break;
					}
					file.read(buf, buf_size);
					auto size = file.gcount();

					sum += async_write_body(buffer(buf,size), std::forward<Token>(token), func);
					if( token.ec_ and *token.ec_ )
						break;

					value.size -= buf_size;
					sleep_for(512us);
				}
				return sum;
			}
			else if( range_value_queue.empty())
				return 0;

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

				sum += async_write_body(buffer(body, body.size()), std::forward<Token>(token), func);
				if( token.ec_ and *token.ec_ )
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

						sum += async_write_body(buffer(buf, size + 2), std::forward<Token>(token), func);
						if( token.ec_ and *token.ec_ )
							return sum;

						sleep_for(512us);
						break;
					}
					file.read(buf, buf_size);
					auto size = file.gcount();

					sum += async_write_body(buffer(buf,size), std::forward<Token>(token), func);
					if( token.ec_ and *token.ec_ )
						return sum;

					value.size -= buf_size;
					sleep_for(512us);
				}
			}
			auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
			sum += async_write_body(buffer(abuf, abuf.size()), std::forward<Token>(token), func);
			return sum;
		}
		else
#endif //LIBGS_USING_BOOST_ASIO
		{
			return [=,this]() mutable -> awaitable<size_t>
			{
				auto sum = co_await async_write_header(0,std::forward<Token>(token), func);
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

							sum += co_await async_write_body(buffer(buf,size), std::forward<Token>(token), func);
							if( not check_token_error(token) )
								break;

							co_await co_sleep_for(512us);
							break;
						}
						file.read(buf, buf_size);
						auto size = file.gcount();

						sum += co_await async_write_body(buffer(buf,size), std::forward<Token>(token), func);
						if( not check_token_error(token) )
							break;

						value.size -= buf_size;
						co_await co_sleep_for(512us);
					}
					co_return sum;
				}
				else if( range_value_queue.empty())
					co_return 0;

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

					sum += co_await async_write_body(buffer(body, body.size()), std::forward<Token>(token), func);
					if( not check_token_error(token) )
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

							sum += co_await async_write_body(buffer(buf, size + 2), std::forward<Token>(token), func);
							if( not check_token_error(token) )
								co_return sum;

							co_await co_sleep_for(512us);
							break;
						}
						file.read(buf, buf_size);
						auto size = file.gcount();

						sum += co_await async_write_body(buffer(buf,size), std::forward<Token>(token), func);
						if( not check_token_error(token) )
							co_return sum;

						value.size -= buf_size;
						co_await co_sleep_for(512us);
					}
				}
				auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
				sum += co_await async_write_body(buffer(abuf, abuf.size()), std::forward<Token>(token), func);
				co_return sum;
			}();
		}
	}

	template <typename Token> auto async_range_transfer
	(std::string_view file_name, std::ifstream &file, string_view_t _range_str, Token &&token, const char *func)
	{
#ifdef LIBGS_USING_BOOST_ASIO
		if( std::is_same_v<std::decay_t<Token>, yield_context> )
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
				q_ptr->set_status(status::bad_request);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
			}

				// bytes=x-y, m-n, i-j ...
			else if( range_str.substr(0, 6) != static_string::bytes_start )
			{
				q_ptr->set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
			}

			// x-y, m-n, i-j ...
			auto range_list_str = range_str.substr(6);
			if( range_list_str.empty())
			{
				q_ptr->set_status(status::range_not_satisfiable);
				auto buf = std::format("{} ({})", to_status_description(status), status);
				return async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
			}

			// (x-y) ( m-n) ( i-j) ...
			auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
			auto mime_type = get_mime_type(file_name);

			q_ptr->set_status(status::partial_content);
			std::list<range_value> range_value_list;

			if( range_list.size() == 1 )
			{
				range_value_list.emplace_back();
				auto &range_value = range_value_list.back();

				if( not range_parsing(file_name, range_list[0], range_value))
				{
					q_ptr->set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					return async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}
				q_ptr->set_header(header_t::accept_ranges, static_string::bytes)
					  .set_header(header_t::content_type, mime_type)
					  .set_header(header_t::content_length, range_value.size)
					  .set_header(header_t::content_range,
								  {static_string::content_range_format, range_value.begin, range_value.end,
								   range_value.size});

				return async_send_range(file, "", "", range_value_list, std::forward<Token>(token), func);
			} // if( rangeList.size() == 1 )

			using namespace std::chrono;
			auto boundary = std::format("{}_{}", __func__,
										duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

			q_ptr->set_header(header_t::content_type, static_string::content_type_boundary + boundary);

			auto ct_line = std::format("{}: {}", header::content_type, mime_type);
			std::size_t content_length = 0;

			for(auto &str: range_list)
			{
				range_value_list.emplace_back();
				auto &range_value = range_value_list.back();

				if( not range_parsing(file_name, str_trimmed(str), range_value))
				{
					q_ptr->set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					return async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
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

			q_ptr->set_header(header_t::content_length, content_length)
				  .set_header(header_t::accept_ranges, static_string::bytes);

			return async_send_range(file, boundary, ct_line, range_value_list, std::forward<Token>(token), func);
		}
		else
#endif //LIBGS_USING_BOOST_ASIO
		{
			return [=,this]() mutable -> awaitable<size_t>
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
					q_ptr->set_status(status::bad_request);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// bytes=x-y, m-n, i-j ...
				else if( range_str.substr(0, 6) != static_string::bytes_start )
				{
					q_ptr->set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// x-y, m-n, i-j ...
				auto range_list_str = range_str.substr(6);
				if( range_list_str.empty())
				{
					q_ptr->set_status(status::range_not_satisfiable);
					auto buf = std::format("{} ({})", to_status_description(status), status);
					co_return co_await async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
				}

				// (x-y) ( m-n) ( i-j) ...
				auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
				auto mime_type = get_mime_type(file_name);

				q_ptr->set_status(status::partial_content);
				std::list<range_value> range_value_list;

				if( range_list.size() == 1 )
				{
					range_value_list.emplace_back();
					auto &range_value = range_value_list.back();

					if( not range_parsing(file_name, range_list[0], range_value))
					{
						q_ptr->set_status(status::range_not_satisfiable);
						auto buf = std::format("{} ({})", to_status_description(status), status);
						co_return co_await async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
					}
					q_ptr->set_header(header_t::accept_ranges, static_string::bytes)
						  .set_header(header_t::content_type, mime_type)
						  .set_header(header_t::content_length, range_value.size)
						  .set_header(header_t::content_range,
									  {static_string::content_range_format, range_value.begin, range_value.end,
									   range_value.size});

					co_return co_await async_send_range(file, "", "", range_value_list, std::forward<Token>(token), func);
				} // if( rangeList.size() == 1 )

				using namespace std::chrono;
				auto boundary = std::format("{}_{}", __func__,
											duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

				q_ptr->set_header(header_t::content_type, static_string::content_type_boundary + boundary);

				auto ct_line = std::format("{}: {}", header::content_type, mime_type);
				std::size_t content_length = 0;

				for(auto &str: range_list)
				{
					range_value_list.emplace_back();
					auto &range_value = range_value_list.back();

					if( not range_parsing(file_name, str_trimmed(str), range_value))
					{
						q_ptr->set_status(status::range_not_satisfiable);
						auto buf = std::format("{} ({})", to_status_description(status), status);
						co_return co_await async_write(buffer(buf, buf.size()), std::forward<Token>(token), func);
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

				q_ptr->set_header(header_t::content_length, content_length)
					  .set_header(header_t::accept_ranges, static_string::bytes);

				co_return co_await async_send_range(file, boundary, ct_line, range_value_list, std::forward<Token>(token), func);
			}();
		}
	}

private:
	size_t write_header(size_t size, error_code &error)
	{
		auto header_data = m_helper.header_data(size);
		auto sum = m_next_layer.next_layer().write(buffer(header_data, header_data.size()), error);
		if( not error )
			m_headers_writed = true;
		return sum;
	}

	size_t write_body(const const_buffer &body, error_code &error)
	{
		return m_next_layer.next_layer().write(body, error);
	}

	template <typename Token>
	auto async_write_header(size_t size, Token &&token, const char *func)
	{
#ifdef LIBGS_USING_BOOST_ASIO
		if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			auto header_data = m_helper.header_data(size);
			auto sum = _async_write(buffer(header_data, header_data.size()), std::forward<Token>(token));
			if( not token.ec_ or not *token.ec_ )
				m_headers_writed = true;
			return sum;
		}
		else
#endif //LIBGS_USING_BOOST_ASIO
		{
			return [=,this]() mutable -> awaitable<size_t>
			{
				auto header_data = m_helper.header_data(size);
				auto sum = co_await _async_write(buffer(header_data, header_data.size()), std::forward<Token>(token), func);
				if( check_token_error(token) )
					m_headers_writed = true;
				co_return sum;
			}();
		}
	}

	template <typename Token>
	auto async_write_body(const const_buffer &body, Token &&token, const char *func)
	{
		return _async_write(body, std::forward<Token>(token), func);
	}

private:
	size_t _write(const const_buffer &buf, error_code &error) noexcept
	{
		size_t sum = 0;
		do
		{
			sum += m_next_layer.next_layer().write(buffer(buf, buf.size()), error);
			if( error and error.value() == errc::interrupted )
				continue;
		}
		while(false);
		return sum;
	}

	template<typename Token>
	auto _async_write(const const_buffer &buf, Token &&token, const char *func)
	{
#ifdef LIBGS_USING_BOOST_ASIO
		if( std::is_same_v<std::decay_t<Token>, yield_context> )
		{
			size_t sum = 0;
			error_code error;

			if( token.ec_ )
				*token.ec_ = error;
			do {
				sum += m_next_layer.next_layer().async_write(buffer(buf, buf.size()), token[error]);
				if( error and error.value() == errc::interrupted )
					continue;
			}
			while(false);

			if( not error )
				return sum;
			else if( token.ec_ )
				*token.ec_ = error;
			else
				throw system_error(error, "libgs::http::server_response::{}", func);
			return sum;
		}
		else
#endif //LIBGS_USING_BOOST_ASIO
		{
			token_reset(token);
			return [=, this]() mutable -> awaitable<size_t>
			{
				size_t sum = 0;
				error_code error;
				do
				{
					if constexpr( detail::concept_has_get<Token> )
					{
						sum += co_await m_next_layer.next_layer().async_write
								(buffer(buf, buf.size()), use_awaitable|error|token.get_cancellation_slot());
					}
					else
					{
						sum += co_await m_next_layer.next_layer().async_write
								(buffer(buf, buf.size()), use_awaitable|error);
					}
					if( error and error.value() == errc::interrupted )
						continue;
				}
				while(false);
				token_handle(token, error, func);
				co_return sum;
			}();
		}
	}

	template <typename Token>
	[[nodiscard]] bool check_token_error(Token &token)
	{
		if constexpr( detail::concept_has_ec_<Token> )
		{
			if( token.ec_ )
				return false;
		}
		else if constexpr( detail::concept_has_get<Token> )
		{
			if constexpr( detail::concept_has_ec_<decltype(token.get())> )
			{
				if( token.get().ec_ )
					return false;
			}
		}
		return true;
	}

public:
	response_t *q_ptr;
	helper_t m_helper;
	next_layer_t m_next_layer;

	bool m_headers_writed = false;
	bool m_chunk_end_writed = false;
};

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::basic_server_response(next_layer_t &&next_layer) :
	m_impl(new impl(std::move(next_layer)))
{

}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::~basic_server_response()
{
	delete m_impl;
}

template <typename Request, concept_char_type CharT>
template <typename Request0>
basic_server_response<Request,CharT>::basic_server_response
(basic_server_response<Request0,CharT> &&other) noexcept
	requires concept_constructible<Request0,Request0&&> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <typename Request, concept_char_type CharT>
template <typename Request0>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::operator=
(basic_server_response<Request0,CharT> &&other) noexcept
	requires concept_constructible<Request0,Request0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_status(uint32_t status)
{
	m_impl->m_helper.set_status(status);
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_status(http::status status)
{
	m_impl->m_helper.set_status(status);
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_header(string_view_t key, value_t value)
{
	m_impl->m_helper.set_header(key, std::move(value));
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_cookie(string_view_t key, cookie_t cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return *this;
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::write(const const_buffer &body)
{
	return m_impl->write(body, "write");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::write(const const_buffer &body, error_code &error)
{
	return m_impl->write(body, error, "write");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::write(error_code &error) noexcept
{
	return write({nullptr,0}, error);
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_write(const const_buffer &body, Token &&token)
{
	return m_impl->async_write(body, std::forward<Token>(token), "async_write");
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_write(Token &&token)
{
	return async_write({nullptr,0}, std::forward<Token>(token));
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, http::redirect redi)
{
	m_impl->m_helper.set_redirect(url, redi);
	return m_impl->write({nullptr,0}, "error");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, http::redirect redi, error_code &error) noexcept
{
	m_impl->m_helper.set_redirect(url, redi);
	return m_impl->write({nullptr,0}, error, "error");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, error_code &error) noexcept
{
	m_impl->m_helper.set_redirect(url, http::redirect::moved_permanently);
	return m_impl->write({nullptr,0}, error, "error");
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_redirect(string_view_t url, http::redirect redi, Token &&token)
{
	m_impl->m_helper.set_redirect(url, redi);
	return m_impl->async_write({nullptr,0}, std::forward<Token>(token), "async_redirect");
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_redirect(string_view_t url, Token &&token)
{
	m_impl->m_helper.set_redirect(url, http::redirect::moved_permanently);
	return m_impl->async_write({nullptr,0}, std::forward<Token>(token), "async_redirect");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file(std::string_view file_name, const resp_ranges &ranges)
{
	error_code error;
	auto res = send_file(file_name, ranges, error, "send_file");
	if( error )
		throw system_error(error, "libgs::http::response::send_file");
	return res;
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file
(std::string_view file_name, const resp_ranges &ranges, error_code &error) noexcept
{
	return m_impl->send_file(file_name, ranges, error, "send_file");
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file(std::string_view file_name, error_code &error) noexcept
{
	return send_file(file_name, {}, error);
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_send_file
(std::string_view file_name, const resp_ranges &ranges, Token &&token)
{
	return m_impl->async_send_file(file_name, ranges, std::forward<Token>(token), "send_file");
}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_send_file(std::string_view file_name, Token &&token)
{
	return async_send_file(file_name, {}, std::forward<Token>(token));
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_chunk_attribute(value_t attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_chunk_attributes(value_list_t attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return *this;
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::chunk_end(const headers_t &headers)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::chunk_end(const headers_t &headers, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_chunk_end(const headers_t &headers, Token &&token)
{

}




template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::string_view_t
basic_server_response<Request,CharT>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <typename Request, concept_char_type CharT>
http::status basic_server_response<Request,CharT>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <typename Request, concept_char_type CharT>
const typename basic_server_response<Request,CharT>::headers_t&
basic_server_response<Request,CharT>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <typename Request, concept_char_type CharT>
const typename basic_server_response<Request,CharT>::cookies_t&
basic_server_response<Request,CharT>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <typename Request, concept_char_type CharT>
bool basic_server_response<Request,CharT>::headers_writed() const noexcept
{
	return m_impl->m_helper.headers_writed();
}

template <typename Request, concept_char_type CharT>
bool basic_server_response<Request,CharT>::chunk_end_writed() const noexcept
{
	return m_impl->m_helper.chunk_end_writed();
}

template <typename Request, concept_char_type CharT>
const typename basic_server_response<Request,CharT>::executor_t&
basic_server_response<Request,CharT>::get_executor() noexcept
{
	return m_impl->m_next_layer.get_executor();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_header(string_view_t key)
{
	m_impl->m_helper.unset_header(key);
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_helper.unset_cookie(key);
	return *this;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_chunk_attribute(const value_t &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return *this;
}

template <typename Request, concept_char_type CharT>
const basic_server_response<Request,CharT>::next_layer_t&
basic_server_response<Request,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::next_layer_t&
basic_server_response<Request,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
