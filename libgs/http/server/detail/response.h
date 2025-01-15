
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

template <concepts::stream Stream, core_concepts::char_type CharT>
class basic_server_response<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

	using response_t = basic_server_response;
	using string_list_t = basic_string_list<char_t>;
	using static_string = detail::response_helper_static_string<char_t>;

public:
	impl(next_layer_t &&next_layer) :
		m_helper(next_layer.version(), next_layer.headers()),
		m_next_layer(std::move(next_layer)) {}

	template <typename Stream0>
	impl &operator=(typename basic_server_response<Stream0,char_t>::impl &&other) noexcept
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
	void set_header(string_view_t key, value_t value) {
		m_helper.set_header(key, std::move(value));
	}

	void set_status(status_t status) {
		m_helper.set_status(status);
	}

public:
	[[nodiscard]] size_t write(const const_buffer &body, error_code &error) noexcept
	{
		error = error_code();
		if( m_headers_writed )
		{
			return body.size() == 0 and m_chunk_end_writed ?
				0 : write_body(body, error);
		}
		auto sum = write_header(body.size(), error);
		if( error )
			return sum;
		else if( body.size() > 0 )
			sum += write_body(body, error);
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_write(const const_buffer &body, error_code &error) noexcept
	{
		error = error_code();
		if( m_headers_writed )
		{
			co_return body.size() == 0 and m_chunk_end_writed ?
				0 : co_await co_write_body(body, error);
		}
		auto sum = co_await co_write_header(body.size(), error);
		if( error )
			co_return sum;
		else if( body.size() > 0 )
			sum += co_await co_write_body(body, error);
		co_return sum;
	}

	void set_blocking(error_code &error)
	{
		socket_operation_helper<typename next_layer_t::next_layer_t>
		(m_next_layer.next_layer()).non_blocking(false, error);
	}

private:
	struct range_value : file_range
	{
		std::string cr_line;
		size_t end = 0;
	};

	struct fot_data
	{
		std::string mtype;
		size_t fsize = 0;
	};

public:
	template <typename Opt>
	[[nodiscard]] size_t send_file(Opt &&opt, error_code &error)
	{
		if( m_headers_writed )
		{
			error = make_error_code(asio::error::no_protocol_option);
			return 0;
		}
		fot_data data = 0;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			return 0;

		if( not token.ranges.empty() )
		{
			auto ranges = from_file_range(token.ranges, data.fsize, error);
			return error ? 0 : range_transfer(token, ranges, data, error);
		}
		auto it = m_next_layer.headers().find(header_t::range);
		if( it == m_next_layer.headers().end() )
			return default_transfer(token, data, error);

		std::list<range_value> ranges;
		auto status = range_text_parsing(it->second.to_string(), data.fsize, ranges);
		if( status != status::ok )
		{
			set_status(status::range_not_satisfiable);
			auto buf = std::format("{} ({})", status_description(status), status);
			return write(buffer(buf, buf.size()), error);
		}
		return range_transfer(token, ranges, data, error);
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_send_file(Opt &&opt, error_code &error)
	{
		if( m_headers_writed )
		{
			error = make_error_code(asio::error::no_protocol_option);
			co_return 0;
		}
		fot_data data;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			co_return 0;

		if( not token.ranges.empty() )
		{
			auto ranges = from_file_range(token.ranges, data.fsize, error);
			co_return error ? 0 : co_await co_range_transfer(token, ranges, data, error);
		}
		auto it = m_next_layer.headers().find(header_t::range);
		if( it == m_next_layer.headers().end() )
			co_return co_await co_default_transfer(token, data, error);

		std::list<range_value> ranges;
		auto status = range_text_parsing(it->second.to_string(), data.fsize, ranges);
		if( status != status::ok )
		{
			set_status(status::range_not_satisfiable);
			auto buf = std::format("{} ({})", status_description(status), status);
			co_return co_await co_write(buffer(buf, buf.size()), error);
		}
		co_return co_await co_range_transfer(token, ranges, data, error);
	}

public:
	[[nodiscard]] size_t chunk_end(const headers_t &headers, error_code &error, const char *func)
	{
		auto buf = chunk_end_check(headers, func);
		auto res = write_body(buffer(buf, buf.size()), error);
		if( not error )
			m_chunk_end_writed = true;
		return res;
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers, error_code &error, const char *func)
	{
		auto buf = chunk_end_check(headers, func);
		auto res = co_await co_write_body(buffer(buf, buf.size()), error);
		if( not error )
			m_chunk_end_writed = true;
		co_return res;
	}

	[[nodiscard]] size_t check_time_out(const auto &var, error_code &error) const
	{
		if( var.index() == 0 )
			return std::get<0>(var);
		else if( not std::get<1>(var) )
			error = make_error_code(errc::timed_out);
		return 0;
	}

private:
	template <typename Opt>
	[[nodiscard]] size_t default_transfer(Opt &&opt, const fot_data &data, error_code &error) noexcept
	{
		spdlog::debug("resource mime-type: {}.", data.mtype);
		size_t sum = 0;
		if( data.fsize == 0 )
			return sum;

		set_header(header_t::content_type, mbstoxx<char_t>(data.mtype));
		sum += write_header(data.fsize, error);
		if( error )
			return sum;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		opt.stream->seekg(0);
		while( not opt.stream->eof() )
		{
			opt.stream->read(fr_buf, buf_size);
			auto size = opt.stream->gcount();
			if( size == 0 )
				break;

			sum += write_body(buffer(fr_buf, size), error);
			if( error )
				break;
//			sleep_for(512us);
		}
		return sum;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_default_transfer(Opt &&opt, const fot_data &data, error_code &error) noexcept
	{
		spdlog::debug("resource mime-type: {}.", data.mtype);
		size_t sum = 0;
		if( data.fsize == 0 )
			co_return sum;

		set_header(header_t::content_type, mbstoxx<char_t>(data.mtype));
		sum += co_await co_write_header(data.fsize, error);
		if( error )
			co_return sum;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		opt.stream->seekg(0);
		while( not opt.stream->eof() )
		{
			opt.stream->read(fr_buf, buf_size);
			auto size = opt.stream->gcount();
			if( size == 0 )
				break;

			sum += co_await co_write_body(buffer(fr_buf, size), error);
			if( error )
				break;
//			co_await co_sleep_for(512us /*,get_executor()*/);
		}
		co_return sum;
	}

public:
	[[nodiscard]] size_t range_transfer
	(auto &&opt, const std::list<range_value> &ranges, const fot_data &data, error_code &error)
	{
		set_status(status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			set_header(header_t::accept_ranges, static_string::bytes);
			set_header(header_t::content_type, mbstoxx<char_t>(mime_type));
			set_header(header_t::content_length, range.total);
			set_header(header_t::content_range, {
				static_string::content_range_format, range.begin, range.end, range.total
			});
			return send_range(opt.stream, "", "", ranges, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
		set_header(header_t::content_type, static_string::content_type_boundary + mbstoxx<char_t>(boundary));

		auto ct_line = std::format("{}: {}", header::content_type, data.mtype);
		std::size_t content_length = 0;

		for(auto &range: ranges)
		{
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
			content_length += 2 + boundary.size() + 2 +  // --boundary<CR><LF>
							  ct_line.size() + 2 +       // Content-Type: xxx<CR><LF>
							  range.cr_line.size() + 2 + // Content-Range: bytes 3-11/96<CR><LF>
							  2 +                        // <CR><LF>
							  range.total + 2;           // 012345678<CR><LF>
		}
		content_length += 2 + boundary.size() + 2 + 2;   // --boundary--<CR><LF>

		set_header(header_t::content_length, content_length);
		set_header(header_t::accept_ranges, static_string::bytes);
		return send_range(opt.stream, boundary, ct_line, ranges, error);
	}

	[[nodiscard]] awaitable<size_t> co_range_transfer
	(auto &&opt, const std::list<range_value> &ranges, const fot_data &data, error_code &error)
	{
		set_status(status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			set_header(header_t::accept_ranges, static_string::bytes);
			set_header(header_t::content_type, mbstoxx<char_t>(data.mtype));
			set_header(header_t::content_length, range.total);
			set_header(header_t::content_range, {
				static_string::content_range_format, range.begin, range.end, range.total
			});
			co_return co_await co_send_range(opt.stream, "", "", ranges, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
		set_header(header_t::content_type, static_string::content_type_boundary + mbstoxx<char_t>(boundary));

		auto ct_line = std::format("{}: {}", header::content_type, data.mtype);
		std::size_t content_length = 0;

		for(auto &range: ranges)
		{
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
			content_length += 2 + boundary.size() + 2 +  // --boundary<CR><LF>
							  ct_line.size() + 2 +       // Content-Type: xxx<CR><LF>
							  range.cr_line.size() + 2 + // Content-Range: bytes 3-11/96<CR><LF>
							  2 +                        // <CR><LF>
							  range.total + 2;           // 012345678<CR><LF>
		}
		content_length += 2 + boundary.size() + 2 + 2;   // --boundary--<CR><LF>

		set_header(header_t::content_length, content_length);
		set_header(header_t::accept_ranges, static_string::bytes);
		co_return co_await co_send_range(opt.stream, boundary, ct_line, ranges, error);
	}

private:
	template <typename FS>
	[[nodiscard]] size_t send_range(
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::list<range_value> ranges, error_code &error
	) noexcept
	{
		assert(not ranges.empty());
		auto sum = write_header(0, error);
		if( error )
			return sum;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] {0};

		if( ranges.size() == 1 )
		{
			auto &value = ranges.back();
			stream->seekg(value.begin, std::ios_base::beg);

			while( not stream->eof() )
			{
				if( value.total <= buf_size )
				{
					stream->read(buf, value.total);
					auto size = stream->gcount();

					sum += write_body(buffer(buf,size), error);
					if( error )
						break;

//					sleep_for(512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					break;

				value.size -= buf_size;
//				sleep_for(512us);
			}
			return sum;
		}
		for(auto &value: ranges)
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

			stream->seekg(value.begin, std::ios_base::beg);
			while( not stream->eof() )
			{
				if( value.size <= buf_size )
				{
					stream->read(buf, value.size);
					auto size = stream->gcount();
					if( size == 0 )
						break;

					buf[size + 0] = '\r';
					buf[size + 1] = '\n';

					sum += write_body(buffer(buf, size + 2), error);
					if( error )
						return sum;

//					sleep_for(512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					return sum;

				value.size -= buf_size;
//				sleep_for(512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		sum += write_body(buffer(abuf, abuf.size()), error);
		return sum;
	}

	template <typename FS>
	[[nodiscard]] awaitable<size_t> co_send_range(
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::list<range_value> ranges, error_code &error
	) noexcept
	{
		assert(not ranges.empty());
		auto sum = co_await co_write_header(0, error);
		if( error )
			co_return sum;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] {0};

		if( ranges.size() == 1 )
		{
			auto &value = ranges.back();
			stream->seekg(value.begin, std::ios_base::beg);

			while( not stream->eof() )
			{
				if( value.total <= buf_size )
				{
					stream->read(buf, value.total);
					auto size = stream->gcount();

					sum += co_await co_write_body(buffer(buf,size), error);
					if( error )
						break;

//					co_await co_sleep_for(512us /*,get_executor()*/);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					break;

				value.total -= buf_size;
//				co_await co_sleep_for(512us /*,get_executor()*/);
			}
			co_return sum;
		}
		for(auto &value : ranges)
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

			sum += co_await co_write_body(buffer(body, body.size()), error);
			if( error )
				co_return sum;

			stream->seekg(value.begin, std::ios_base::beg);
			while( not stream->eof() )
			{
				if( value.total <= buf_size )
				{
					stream->read(buf, value.total);
					auto size = stream->gcount();
					if( size == 0 )
						break;

					buf[size + 0] = '\r';
					buf[size + 1] = '\n';

					sum += co_await co_write_body(buffer(buf, size + 2), error);
					if( error )
						co_return sum;

//					co_await co_sleep_for(512us /*,get_executor()*/);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					co_return sum;

				value.total -= buf_size;
//				co_await co_sleep_for(512us /*,get_executor()*/);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		sum += co_await co_write_body(buffer(abuf, abuf.size()), error);
		co_return sum;
	}

private:
	[[nodiscard]] status_t range_text_parsing
	(string_view_t range_str_view, size_t file_size, std::list<range_value> &ranges)
	{
		string_t range_str(range_str_view.data(), range_str_view.size());
		for(auto i=range_str.size(); i>0; i--)
		{
			if( range_str[i] == 0x20/*SPACE*/ )
				range_str.erase(i,1);
		}
		if( range_str.empty() )
			return status::bad_request;

		// bytes=x-y, m-n, i-j ...
		else if( range_str.substr(0,6) != static_string::bytes_start )
			return status::range_not_satisfiable;

		// x-y, m-n, i-j ...
		auto cl_range_str = range_str.substr(6);
		if( cl_range_str.empty() )
			return status::range_not_satisfiable;

		// (x-y) ( m-n) ( i-j) ...
		for(auto &sub_range_str : string_list_t::from_string(cl_range_str, 0x2C/*,*/))
		{
			range_value range;
			range.total = 0;

			auto list = string_list_t::from_string(sub_range_str, '-', false);
			if( list.size() != 2 )
			{
				spdlog::debug("Range format error: {}.", xxtombs(sub_range_str));
				return status::range_not_satisfiable;
			}
			else if( list[0].empty() )
			{
				if( list[1].empty() )
				{
					spdlog::debug("Range format error.");
					return status::range_not_satisfiable;
				}
				range.total = ston_or<size_t>(list[1]);

				if( range.total == 0 or range.total > file_size )
				{
					spdlog::debug("Range size is invalid.");
					return status::range_not_satisfiable;
				}
				range.begin = file_size - range.total;
				range.end   = file_size - 1;
			}
			else if( list[1].empty() )
			{
				if( list[0].empty() )
				{
					spdlog::debug("Range format error.");
					return status::range_not_satisfiable;
				}
				range.begin = ston_or<size_t>(list[0]);
				range.end   = file_size - 1;

				if( range.begin > range.end )
				{
					spdlog::debug("Range is invalid.");
					return status::range_not_satisfiable;
				}
				range.total = file_size - range.begin;
			}
			else
			{
				range.begin = ston_or<size_t>(list[0]);
				range.end   = ston_or<size_t>(list[1]);

				if( range.begin > range.end or range.end >= file_size )
				{
					spdlog::debug("Range is invalid.");
					return status::range_not_satisfiable;
				}
				range.total = range.end - range.begin + 1;
			}
			range.cr_line = std::format("{}: bytes {}-{}/{}",
				header::content_range, range.begin, range.end, file_size
			);
			ranges.emplace_back(std::move(range));
		}
		return status::ok;
	}

	[[nodiscard]] std::list<range_value> from_file_range
	(const file_ranges &ranges, size_t file_size, error_code &error)
	{
		std::list<range_value> list;
		for(auto &range : ranges)
		{
			auto end = range.begin + range.total - 1;
			if( range.total == 0 or end >= file_size )
			{
				error = std::make_error_code(std::errc::invalid_seek);
				break;
			}
			range_value value;
			value.begin = range.begin;
			value.total = range.total;
			value.end   = end;

			value.cr_line = std::format("{}: bytes {}-{}/{}",
				header::content_range, value.begin, value.end, file_size
			);
			list.emplace_back(std::move(value));
		}
		return list;
	}

	[[nodiscard]] std::string chunk_end_check(const headers_t &headers, const char *func)
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
			str_to_lower(it->second.to_string()) != detail::string_pool<char_t>::chunked )
			throw runtime_error("libgs::http::server_response::{}: 'Transfer-Coding: chunked' not set.", func);

		std::string buf = "0\r\n";
		for(auto &[key,value] : headers)
			buf += xxtombs(key) + ": " + xxtombs(value.to_string()) + "\r\n";
		return buf + "\r\n";
	}

private:
	[[nodiscard]] size_t write_header(size_t size, error_code &error) noexcept {
		return write_funcdata(m_helper.header_data(size), true, error);
	}

	[[nodiscard]] awaitable<size_t> co_write_header(size_t size, error_code &error) noexcept {
		co_return co_await co_write_funcdata(m_helper.header_data(size), true, error);
	}

	[[nodiscard]] size_t write_body(const const_buffer &body, error_code &error) noexcept {
		return write_funcdata(m_helper.body_data(body), false, error);
	}

	[[nodiscard]] awaitable<size_t> co_write_body(const const_buffer &body, error_code &error) noexcept {
		co_return co_await co_write_funcdata(m_helper.body_data(body), false, error);
	}

private:
	[[nodiscard]] size_t write_funcdata(std::string &&data, bool wheader, error_code &error) noexcept
	{
		size_t sum = 0;
		for(;;)
		{
			m_next_layer.next_layer().non_blocking(false, error);
			if( error )
				break;

			sum += asio::write (
				m_next_layer.next_layer(),
				buffer(data.c_str() + sum, data.size() - sum),
				error
			);
			if( error and error.value() == errc::interrupted )
				continue;
			break;
		}
		if( not error and wheader )
			m_headers_writed = true;
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_write_funcdata(std::string &&data, bool wheader, error_code &error) noexcept
	{
		using namespace libgs::operators;
		error = error_code();
		size_t sum = 0;
		for(;;)
		{
			sum += co_await asio::async_write (
				m_next_layer.next_layer(),
				buffer(data.c_str() + sum, data.size() - sum),
				use_awaitable | error
			);
			if( error and error.value() == errc::interrupted )
				continue;
			break;
		}
		if( not error and wheader )
			m_headers_writed = true;
		co_return sum;
	}

private:
	template <typename Opt>
	[[nodiscard]] auto file_opt_token_helper(Opt &&opt, fot_data &data, error_code &error)
	{
		if constexpr( is_char_string_v<Opt> or is_fstream_v<Opt> or is_ofstream_v<Opt> )
		{
			using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
			using type = typename token_t::type;
			return _file_opt_token_helper(
				file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
				data, error
			);
		}
		else if constexpr( Opt::optype == file_optype::single )
		{
			using type = typename std::remove_cvref_t<Opt>::type;
			return _file_opt_token_helper(
				file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
				data, error
			);
		}
		else
			return _file_opt_token_helper(std::forward<Opt>(opt), data, error);
	}

	template <typename Opt>
	[[nodiscard]] auto _file_opt_token_helper(Opt &&opt, fot_data &data, error_code &error)
	{
		error = opt.init(std::ios::in | std::ios::binary);
		if( error )
			return std::forward<Opt>(opt);

		auto size = file_size(opt, io_permission::write);
		if( not size )
		{
			error = make_error_code(std::errc::permission_denied);
			return std::forward<Opt>(opt);
		}
		data.fsize = *size;
		data.mtype = mime_type(opt);
		return std::forward<Opt>(opt);
	}

private:
	bool token_check(auto &token, const error_code &error) noexcept
	{
		if( not error )
			return true;
		token.ec_ = error;
		return false;
	}

public:
	helper_t m_helper;
	next_layer_t m_next_layer;

	bool m_headers_writed = false;
	bool m_chunk_end_writed = false;
};

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT>::basic_server_response(next_layer_t &&next_layer) :
	m_impl(new impl(std::move(next_layer)))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT>::~basic_server_response()
{
	delete m_impl;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT>::basic_server_response(basic_server_response &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::operator=
(basic_server_response &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename Stream0>
basic_server_response<Stream,CharT>::basic_server_response
(basic_server_response<Stream0,char_t> &&other) noexcept
	requires core_concepts::constructible<next_layer_t,basic_server_request<Stream0,char_t>&&> :
	m_impl(new impl(this, std::move(*other.m_impl)))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename Stream0>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::operator=
(basic_server_response<Stream0,char_t> &&other) noexcept requires core_concepts::assignable<Stream,Stream0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_status(status_t status)
{
	m_impl->set_status(status);
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_header(string_view_t key, value_t value)
{
	m_impl->set_header(key, std::move(value));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_cookie(string_view_t key, cookie_t cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::write(const const_buffer &body, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
	{
		m_impl->set_blocking(token);
		return token ? 0 : m_impl->write(body, token);
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = write(body, error);
		if( error )
			throw system_error(error, "libgs::http::server_response::write");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(),
		[this, body, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_write(body, error) or
				co_sleep_for(timeout, get_executor())
			);
			auto res = m_impl->check_time_out(var, error);

			check_error(remove_const(ntoken), error, "libgs::http::server_response::write");
			co_return res;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(), [this, body, token]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto res = co_await m_impl->co_write(body, error);
			check_error(remove_const(token), error, "libgs::http::server_response::write");
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::write(Token &&token)
{
	return write({nullptr,0}, std::forward<Token>(token));
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::redirect
(core_concepts::basic_string_type<char_t> auto &&url, http::redirect redi, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
	{
		m_impl->set_blocking(token);
		if( not token )
		{
			m_impl->m_helper.set_redirect(std::forward<decltype(url)>(url), redi);
			return m_impl->write({nullptr,0}, token, "redirect");
		}
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = redirect(std::forward<decltype(url)>(url), redi, error);
		if( error )
			throw system_error(error, "libgs::http::server_response::redirect");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		m_impl->m_helper.set_redirect(std::forward<decltype(url)>(url), redi);
		if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
		{
			auto ntoken = unbound_redirect_time(token);
			return asio::co_spawn(get_executor(),
			[this, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
			{
				error_code error;
				auto var = co_await (
					m_impl->co_write({nullptr,0}, error) or
					co_sleep_for(timeout, get_executor())
				);
				auto res = m_impl->check_time_out(var, error);

				check_error(remove_const(ntoken), error, "libgs::http::server_response::redirect");
				co_return res;
			},
			ntoken);
		}
		else
		{
			return asio::co_spawn(get_executor(), [this, token]() mutable -> awaitable<size_t>
			{
				error_code error;
				auto res = co_await m_impl->co_write({nullptr,0}, error) or
				check_error(remove_const(token), error, "libgs::http::server_response::redirect");
				co_return res;
			},
			token);
		}
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::redirect
(core_concepts::basic_string_type<char_t> auto &&url, Token &&token)
{
	return redirect (
		std::forward<decltype(url)>(url), redirect_t::moved_permanently, std::forward<Token>(token)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::send_file
(concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt, Token &&token)
{
	using opt_t = decltype(opt);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( std::is_same_v<token_t, error_code> )
	{
		m_impl->set_blocking(token);
		return token ? 0 : m_impl->send_file(std::forward<opt_t>(opt), token);
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = save_file(std::forward<opt_t>(opt), error);
		if( error )
			throw system_error(error, "libgs::http::server_request::save_file");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(), [
			this, ntoken, opt = std::forward<opt_t>(opt), timeout = get_associated_redirect_time(token)
		]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_send_file(std::move(opt), error) or
				co_sleep_for(timeout, get_executor())
			);
			auto res = m_impl->check_time_out(var, error);

			check_error(remove_const(ntoken), error, "libgs::http::server_response::send_file");
			co_return res;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(),
		[this, token, opt = std::forward<opt_t>(opt)]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto res = co_await m_impl->co_send_file(std::move(opt), error) or
			check_error(remove_const(token), error, "libgs::http::server_response::send_file");
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_chunk_attribute(value_t attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::set_chunk_attributes(value_list_t attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::chunk_end(const headers_t &headers, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code&> )
	{
		m_impl->set_blocking(token);
		return token ? 0 : m_impl->chunk_end(headers, token, "chunk_end");
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = chunk_end(headers, error);
		if( error )
			throw system_error(error, "libgs::http::server_response::chunk_end");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(),
		[this, headers, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_chunk_end(headers, error, "chunk_end") or
				co_sleep_for(timeout /*,get_executor()*/)
			);
			auto res = m_impl->check_time_out(var, error);

			check_error(remove_const(ntoken), error, "libgs::http::server_response::chunk_end");
			co_return res;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(), [this, headers, token]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto res = co_await m_impl->co_chunk_end(headers, error, "chunk_end") or
			check_error(remove_const(token), error, "libgs::http::server_response::chunk_end");
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_response<Stream,CharT>::chunk_end(Token &&token)
{
	return chunk_end({}, std::forward<Token>(token));
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_response<Stream,CharT>::string_view_t
basic_server_response<Stream,CharT>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
status_t basic_server_response<Stream,CharT>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_response<Stream,CharT>::headers_t&
basic_server_response<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_response<Stream,CharT>::cookies_t&
basic_server_response<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_response<Stream,CharT>::headers_writed() const noexcept
{
	return m_impl->m_headers_writed;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_response<Stream,CharT>::chunk_end_writed() const noexcept
{
	return m_impl->m_chunk_end_writed;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_response<Stream,CharT>::executor_t
basic_server_response<Stream,CharT>::get_executor() noexcept
{
	return m_impl->m_next_layer.get_executor();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_header
(core_concepts::basic_string_type<char_t> auto &&key)
{
	m_impl->m_helper.unset_header(std::forward<decltype(key)>(key));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_cookie
(core_concepts::basic_string_type<char_t> auto &&key)
{
	m_impl->m_helper.unset_cookie(std::forward<decltype(key)>(key));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_server_response<Stream,CharT>::unset_chunk_attribute(const value_t &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_response<Stream,CharT>::next_layer_t&
basic_server_response<Stream,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_response<Stream,CharT>::next_layer_t&
basic_server_response<Stream,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
