
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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
#include <libgs/core/string_vector.h>

namespace libgs::http
{

template <concepts::stream Stream>
class basic_response<Stream>::impl
{
	LIBGS_DISABLE_COPY(impl)

	using response_t = basic_response;
	using sock_helper_t = socket_operation_helper<typename next_layer_t::next_layer_t>;

public:
	explicit impl(next_layer_t &&next_layer) :
		m_helper(next_layer.version(), next_layer.headers()),
		m_next_layer(std::move(next_layer)) {}

	template <typename Stream0>
	impl &operator=(basic_response<Stream0>::impl &&other) noexcept
	{
		m_helper = std::move(other.m_helper);
		m_next_layer = std::move(other.m_next_layer);
		return *this;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_helper = std::move(other.m_helper);
		m_next_layer = std::move(other.m_next_layer);
		return *this;
	}

public:
	void set_status(protocol::status_enum status) {
		m_helper.set_status(status);
	}

	[[nodiscard]] auto pro_state() const noexcept {
		return m_helper.pro_state();
	}

public:
	[[nodiscard]] size_t write(const const_buffer &body, error_code &error) noexcept
	{
		if( pro_state() == protocol::generator_state::finish )
			return 0;

		error = error_code();
		size_t sum = 0;

		if( pro_state() == protocol::generator_state::header )
		{
			sum += write_header(body.size(), error);
			if( error )
				return sum;
		}
		if( body.size() > 0 )
			sum += write_body(body, error);
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_write(const const_buffer &body, error_code &error) noexcept
	{
		if( pro_state() == protocol::generator_state::finish )
			co_return 0;

		error = error_code();
		size_t sum = 0;

		if( pro_state() == protocol::generator_state::header )
		{
			sum += co_await co_write_header(body.size(), error);
			if( error )
				co_return sum;
		}
		if( body.size() > 0 )
			sum += co_await co_write_body(body, error);
		co_return sum;
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
		if( pro_state() != protocol::generator_state::header )
			return 0;

		fot_data data = 0;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			return 0;

		if( not token.ranges.empty() )
		{
			auto ranges = from_file_range(token.ranges, data.fsize, error);
			return error ? 0 : range_transfer(token, ranges, data, error);
		}
		auto it = m_next_layer.headers().find(protocol::header::range);
		if( it == m_next_layer.headers().end() )
			return default_transfer(token, data, error);

		std::vector<range_value> ranges;
		auto status = range_text_parsing(it->second.to_string(), data.fsize, ranges);
		if( status != protocol::status::ok )
		{
			set_status(protocol::status::range_not_satisfiable);
			auto buf = std::format("{} ({})", status_description(status), status);
			return write(buffer(buf, buf.size()), error);
		}
		return range_transfer(token, ranges, data, error);
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_send_file(Opt &&opt, error_code &error)
	{
		if( pro_state() != protocol::generator_state::header )
			co_return 0;

		fot_data data;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			co_return 0;

		if( not token.ranges.empty() )
		{
			auto ranges = from_file_range(token.ranges, data.fsize, error);
			co_return error ? 0 : co_await co_range_transfer(token, ranges, data, error);
		}
		auto it = m_next_layer.headers().find(protocol::header::range);
		if( it == m_next_layer.headers().end() )
			co_return co_await co_default_transfer(token, data, error);

		std::vector<range_value> ranges;
		auto status = range_text_parsing(it->second.to_string(), data.fsize, ranges);
		if( status != protocol::status::ok )
		{
			set_status(protocol::status::range_not_satisfiable);
			auto buf = std::format("{} ({})", protocol::status::description(status), status);
			co_return co_await co_write(buffer(buf, buf.size()), error);
		}
		co_return co_await co_range_transfer(token, ranges, data, error);
	}

public:
	[[nodiscard]] size_t chunk_end(const headers_t &headers, error_code &error)
	{
		if( pro_state() != protocol::generator_state::chunk )
			return 0;
		auto buf = m_helper.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return write_body(buffer(buf), error);
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers, error_code &error)
	{
		if( pro_state() != protocol::generator_state::chunk )
			co_return 0;
		auto buf = m_helper.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;
		co_return co_await co_write_body(buffer(buf), error);
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
	[[nodiscard]] size_t default_transfer
	(Opt &&opt, const fot_data &data, error_code &error) noexcept
	{
		size_t sum = 0;
		if( data.fsize == 0 )
			return sum;

		m_helper.set_header(protocol::header::content_type, data.mtype);
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
	[[nodiscard]] awaitable<size_t> co_default_transfer
	(Opt &&opt, const fot_data &data, error_code &error) noexcept
	{
		size_t sum = 0;
		if( data.fsize == 0 )
			co_return sum;

		m_helper.set_header(protocol::header::content_type, data.mtype);
		sum += co_await co_write_header(data.fsize, error);
		if( error )
			co_return sum;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		opt.stream->seekg(0);
		while( not opt.stream->eof() )
		{
			opt.stream->read(fr_buf, buf_size);
			auto size = static_cast<size_t>(opt.stream->gcount());
			if( size == 0 )
				break;

			sum += co_await co_write_body(buffer(fr_buf, size), error);
			if( error )
				break;
//			co_await sleep_for(get_executor(), 512us);
		}
		co_return sum;
	}

public:
	[[nodiscard]] size_t range_transfer
	(auto &&opt, const std::vector<range_value> &ranges, const fot_data &data, error_code &error)
	{
		set_status(protocol::status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_helper
			.set_header(protocol::header::accept_ranges , "bytes"    )
			.set_header(protocol::header::content_type  , data.mtype )
			.set_header(protocol::header::content_length, range.total)

			.set_header(protocol::header::content_range , value_t {
				"{}-{}/{}", range.begin, range.end, range.total
			});
			return send_range(opt.stream, "", "", ranges, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()
			).count()
		);
		m_helper.set_header(protocol::header::content_type,
			"multipart/byteranges; boundary=" + boundary
		);
		auto ct_line = std::format("{}: {}", protocol::header::content_type, data.mtype);
		std::size_t content_length = 0;

		for(auto &range : ranges)
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

		m_helper
		.set_header(protocol::header::content_length, content_length)
		.set_header(protocol::header::accept_ranges , "bytes");

		return send_range (
			opt.stream, boundary, ct_line, ranges, error
		);
	}

	[[nodiscard]] awaitable<size_t> co_range_transfer
	(auto &&opt, const std::vector<range_value> &ranges, const fot_data &data, error_code &error)
	{
		set_status(protocol::status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_helper
			.set_header(protocol::header::accept_ranges , "bytes"    )
			.set_header(protocol::header::content_type  , data.mtype )
			.set_header(protocol::header::content_length, range.total)

			.set_header(protocol::header::content_range, value_t {
				"{}-{}/{}", range.begin, range.end, range.total
			});
			co_return co_await co_send_range(opt.stream, "", "", ranges, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
		m_helper.set_header(protocol::header::content_type,
			"multipart/byteranges; boundary=" + boundary
		);
		auto ct_line = std::format("{}: {}", protocol::header::content_type, data.mtype);
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

		m_helper
		.set_header(protocol::header::content_length, content_length)
		.set_header(protocol::header::accept_ranges , "bytes");

		co_return co_await co_send_range (
			opt.stream, boundary, ct_line, ranges, error
		);
	}

private:
	template <typename FS>
	[[nodiscard]] size_t send_range(
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::vector<range_value> ranges, error_code &error
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
		std::vector<range_value> ranges, error_code &error
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
					auto size = static_cast<size_t>(stream->gcount());

					sum += co_await co_write_body(buffer(buf,size), error);
					if( error )
						break;

//					co_await sleep_for(get_executor(), 512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					break;

				value.total -= buf_size;
//				co_await sleep_for(get_executor(), 512us);
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
					auto size = static_cast<size_t>(stream->gcount());
					if( size == 0 )
						break;

					buf[size + 0] = '\r';
					buf[size + 1] = '\n';

					sum += co_await co_write_body(buffer(buf, size + 2), error);
					if( error )
						co_return sum;

//					co_await sleep_for(get_executor(), 512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					co_return sum;

				value.total -= buf_size;
//				co_await sleep_for(get_executor(), 512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		sum += co_await co_write_body(buffer(abuf, abuf.size()), error);
		co_return sum;
	}

private:
	[[nodiscard]] protocol::status_enum range_text_parsing
	(std::string_view range_str_view, size_t file_size, std::vector<range_value> &ranges)
	{
		std::string range_str(range_str_view.data(), range_str_view.size());
		for(auto i=range_str.size(); i>0; i--)
		{
			if( range_str[i] == 0x20/*SPACE*/ )
				range_str.erase(i,1);
		}
		if( range_str.empty() )
			return protocol::status::bad_request;

		// bytes=x-y, m-n, i-j ...
		else if( range_str.substr(0,6) != "bytes=" )
			return protocol::status::range_not_satisfiable;

		// x-y, m-n, i-j ...
		auto cl_range_str = range_str.substr(6);
		if( cl_range_str.empty() )
			return protocol::status::range_not_satisfiable;

		// (x-y) ( m-n) ( i-j) ...
		for(auto &sub_range_str : string_vector::from_string(cl_range_str, ','))
		{
			range_value range;
			range.total = 0;

			auto str_vector = string_vector::from_string(sub_range_str, '-', false);
			if( str_vector.size() != 2 )
				return protocol::status::range_not_satisfiable;

			else if( str_vector[0].empty() )
			{
				if( str_vector[1].empty() )
					return protocol::status::range_not_satisfiable;

				range.total = *strtls::to_arith<size_t>(str_vector[1]).or_else();
				if( range.total == 0 or range.total > file_size )
					return protocol::status::range_not_satisfiable;

				range.begin = file_size - range.total;
				range.end   = file_size - 1;
			}
			else if( str_vector[1].empty() )
			{
				if( str_vector[0].empty() )
					return protocol::status::range_not_satisfiable;

				range.begin = *strtls::to_arith<size_t>(str_vector[0]).or_else();
				range.end   = file_size - 1;

				if( range.begin > range.end )
					return protocol::status::range_not_satisfiable;
				range.total = file_size - range.begin;
			}
			else
			{
				range.begin = *strtls::to_arith<size_t>(str_vector[0]).or_else();
				range.end   = *strtls::to_arith<size_t>(str_vector[1]).or_else();

				if( range.begin > range.end or range.end >= file_size )
					return protocol::status::range_not_satisfiable;
				range.total = range.end - range.begin + 1;
			}
			range.cr_line = std::format("{}: bytes {}-{}/{}",
				protocol::header::content_range, range.begin, range.end, file_size
			);
			ranges.emplace_back(std::move(range));
		}
		return protocol::status::ok;
	}

	[[nodiscard]] std::vector<range_value> from_file_range
	(const file_ranges &ranges, size_t file_size, error_code &error)
	{
		std::vector<range_value> vector;
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
				protocol::header::content_range, value.begin, value.end, file_size
			);
			vector.emplace_back(std::move(value));
		}
		return vector;
	}

private:
	[[nodiscard]] size_t write_header(size_t size, error_code &error) noexcept {
		return base_write(m_helper.header_data(size), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_header(size_t size, error_code &error) noexcept {
		co_return co_await co_base_write(m_helper.header_data(size), error);
	}

	[[nodiscard]] size_t write_body(const const_buffer &body, error_code &error) noexcept {
		return base_write(m_helper.body_data(body), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_body(const const_buffer &body, error_code &error) noexcept {
		co_return co_await co_base_write(m_helper.body_data(body), error);
	}

private:
	[[nodiscard]] size_t base_write(std::string &&data, error_code &error)
	{
		size_t sent = 0;
		sock_helper_t sock_helper(m_next_layer.next_layer());

		sock_helper.non_blocking(false, error);
		if( error )
			return sent;

		sent += sock_helper.write(data, error);
		return sent;
	}

	[[nodiscard]] awaitable<size_t> co_base_write(std::string &&data, error_code &error)
	{
		sock_helper_t sock_helper(m_next_layer.next_layer());
		size_t sent = 0;

		using namespace libgs::operators;
		sent += co_await sock_helper.write(data, use_awaitable | error);
		co_return sent;
	}

private:
	template <typename Opt>
	[[nodiscard]] auto file_opt_token_helper(Opt &&opt, fot_data &data, error_code &error)
	{
		if constexpr( is_any_string_v<Opt> or is_fstream_v<Opt,char> or is_ofstream_v<Opt,char> )
		{
			using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
			using type = typename token_t::type;
			return _file_opt_token_helper (
				http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
				data, error
			);
		}
		else if constexpr( Opt::optype == file_optype::single )
		{
			using type = typename std::remove_cvref_t<Opt>::type;
			return _file_opt_token_helper (
				http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
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

		file_size(opt, io_permission::write)
		.transform([&](auto value)
		{
			data.mtype = mime_type(opt);
			data.fsize = value;
		})
		.or_else([&]{
			error = make_error_code(std::errc::permission_denied);
		});
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
};

template <concepts::stream Stream>
basic_response<Stream>::basic_response(next_layer_t &&next_layer) :
	m_impl(new impl(std::move(next_layer)))
{

}

template <concepts::stream Stream>
basic_response<Stream>::~basic_response()
{
	delete m_impl;
}

template <concepts::stream Stream>
basic_response<Stream>::basic_response(basic_response &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::operator=
(basic_response &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream>
template <typename Stream0>
basic_response<Stream>::basic_response
(basic_response<Stream0> &&other) noexcept
	requires core_concepts::constructible<next_layer_t,basic_server_request<Stream0>&&> :
	m_impl(new impl(this, std::move(*other.m_impl)))
{

}

template <concepts::stream Stream>
template <typename Stream0>
basic_response<Stream> &basic_response<Stream>::operator=
(basic_response<Stream0> &&other) noexcept requires
	core_concepts::assignable<Stream,Stream0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream>
basic_response<Stream>&
basic_response<Stream>::set_status(protocol::status_enum status)
{
	m_impl->set_status(status);
	return *this;
}

template <concepts::stream Stream>
std::string_view basic_response<Stream>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <concepts::stream Stream>
protocol::status_enum basic_response<Stream>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::set_header
(core_concepts::text_p<char> auto &&key, value_t value) noexcept
{
	m_impl->m_helper.set_header(std::forward<decltype(key)>(key), std::move(value));
	return *this;
}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::unset_header
(const core_concepts::text_p<char> auto &key) noexcept
{
	m_impl->m_helper.unset_header(key);
	return *this;
}

template <concepts::stream Stream>
const typename basic_response<Stream>::headers_t&
	basic_response<Stream>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <concepts::stream Stream>
typename basic_response<Stream>::headers_t&
basic_response<Stream>::headers() noexcept
{
	return m_impl->m_helper.headers();
}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::set_cookie
(core_concepts::text_p<char> auto &&key, cookie_t cookie) noexcept
{
	m_impl->m_helper.set_cookie(std::forward<decltype(key)>(key), std::move(cookie));
	return *this;
}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::unset_cookie
(const core_concepts::text_p<char> auto &key) noexcept
{
	m_impl->m_helper.unset_cookie(key);
	return *this;
}

template <concepts::stream Stream>
const typename basic_response<Stream>::cookies_t&
basic_response<Stream>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <concepts::stream Stream>
typename basic_response<Stream>::cookies_t&
basic_response<Stream>::cookies() noexcept
{
	return m_impl->m_helper.cookies();
}

template <concepts::stream Stream>
basic_response<Stream>&
basic_response<Stream>::set_chunk_attribute(value_t attr) noexcept
{
	m_impl->m_helper.set_chunk_attribute(std::move(attr));
	return *this;
}

template <concepts::stream Stream>
basic_response<Stream>&
basic_response<Stream>::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_helper.unset_chunk_attribute(attr);
	return *this;
}

template <concepts::stream Stream>
const std::set<typename basic_response<Stream>::value_t>&
basic_response<Stream>::chunk_attributes() const noexcept
{
	return m_impl->m_helper.chunk_attributes();
}

template <concepts::stream Stream>
std::set<typename basic_response<Stream>::value_t>&
basic_response<Stream>::chunk_attributes() noexcept
{
	return m_impl->m_helper.chunk_attributes();
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::write(const const_buffer &body, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return token ? 0 : m_impl->write(body, token);

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
	else if constexpr( is_redirect_time_v<token_t> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(),
		[this, body, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_write(body, error) or
				sleep_for(get_executor(), timeout)
			);
			auto res = m_impl->check_time_out(var, error);
			coro::check_error(remove_const(ntoken),
				error, "libgs::http::server_response::write"
			);
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
			coro::check_error(remove_const(token),
				error, "libgs::http::server_response::write"
			);
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::write(Token &&token)
{
	return write({nullptr,0}, std::forward<Token>(token));
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::redirect
(core_concepts::text_p<char> auto &&url, protocol::redirect_enum redi, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
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
		if constexpr( is_redirect_time_v<token_t> )
		{
			auto ntoken = unbound_redirect_time(token);
			return asio::co_spawn(get_executor(),
			[this, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
			{
				error_code error;
				auto var = co_await (
					m_impl->co_write({nullptr,0}, error) or
					sleep_for(get_executor(), timeout)
				);
				auto res = m_impl->check_time_out(var, error);

				coro::check_error(remove_const(ntoken),
					error, "libgs::http::server_response::redirect"
				);
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
					coro::check_error(remove_const(token),
						error, "libgs::http::server_response::redirect"
					);
				co_return res;
			},
			token);
		}
	}
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::redirect
(core_concepts::text_p<char> auto &&url, Token &&token)
{
	return redirect (
		std::forward<decltype(url)>(url),
		protocol::redirect_enum::moved_permanently,
		std::forward<Token>(token)
	);
}

template <concepts::stream Stream>
template <typename T, core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::send_file(T &&opt, Token &&token)
	requires file_opt_token<T>
{
	using opt_t = decltype(opt);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_error_code_token_v<Token> )
		return token ? 0 : m_impl->send_file(std::forward<opt_t>(opt), token);

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
	else if constexpr( is_redirect_time_v<token_t> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(), [
			this, ntoken, opt = std::forward<opt_t>(opt), timeout = get_associated_redirect_time(token)
		]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_send_file(std::move(opt), error) or
				sleep_for(get_executor(), timeout)
			);
			auto res = m_impl->check_time_out(var, error);

			coro::check_error(remove_const(ntoken),
				error, "libgs::http::server_response::send_file"
			);
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
			coro::check_error(remove_const(token),
				error, "libgs::http::server_response::send_file"
			);
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::chunk_end(const headers_t &headers, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code&> )
		return token ? 0 : m_impl->chunk_end(headers, token, "chunk_end");

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
				sleep_for(get_executor(), timeout)
			);
			auto res = m_impl->check_time_out(var, error);
			coro::check_error(remove_const(ntoken),
				error, "libgs::http::server_response::chunk_end"
			);
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
				coro::check_error(remove_const(token),
					error, "libgs::http::server_response::chunk_end"
				);
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_response<Stream>::chunk_end(Token &&token)
{
	return chunk_end({}, std::forward<Token>(token));
}

template <concepts::stream Stream>
bool basic_response<Stream>::is_finished() const noexcept
{
	return m_impl->pro_state() == protocol::generator_state::finish;
}

template <concepts::stream Stream>
typename basic_response<Stream>::executor_t
basic_response<Stream>::get_executor() noexcept
{
	return m_impl->m_next_layer.get_executor();
}

template <concepts::stream Stream>
basic_response<Stream> &basic_response<Stream>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return *this;
}

template <concepts::stream Stream>
const typename basic_response<Stream>::next_layer_t&
basic_response<Stream>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <concepts::stream Stream>
typename basic_response<Stream>::next_layer_t&
basic_response<Stream>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
