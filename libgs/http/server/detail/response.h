
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#include <libgs/http/protocol/utils/server/generator.h>
#include <libgs/http/protocol/utils/core/compression.h>
#include <libgs/http/protocol/utils/core/conditional.h>
#include <libgs/http/protocol/utils/core/upgrade.h>
#include <libgs/http/protocol/utils/core/range.h>

namespace libgs::http
{

template <core_concepts::exec Exec>
class LIBGS_HTTP_TAPI basic_response<Exec>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using generator_t = server_generator;

	struct range_value : file_range {
		std::string cr_line {};
	};

public:
	explicit impl(connection_ptr connection) :
		m_connection(std::move(connection)) {}

public:
	[[nodiscard]] size_t write(const_buffer body, error_code &error) noexcept
	{
		error.clear();
		size_t sum = 0;

		auto pro_state = m_generator.pro_state();
		std::string compressed {};
		const_buffer wire_body = body;

		if( pro_state == generator_state::finish )
		{
			error = make_error_code(errc::eof);
			return sum;
		}
		else if( pro_state == generator_state::header )
		{
			auto content_type = m_generator.header(header::content_type)
				.transform([](const value &item) { return item.to_string(); })
				.value_or("application/octet-stream");

			if( m_auto_compression and
				gzip_candidate(content_type, body.size(), false) )
				add_vary_accept_encoding();

			if( should_gzip(content_type, body.size(), false) )
			{
				auto encoded = gzip_compress({
					static_cast<const char*>(body.data()), body.size()
				});
				if( not encoded )
				{
					error = encoded.error();
					return sum;
				}
				if( encoded->size() < body.size() or m_req_method == method::head )
				{
					compressed = std::move(*encoded);
					wire_body = buffer(compressed);
					prepare_gzip_headers(false);
				}
			}
			auto header = m_generator.header_data(wire_body.size(), m_req_method);
			if( wire_body.size() > 0 and m_generator.pro_state() != generator_state::finish )
			{
				if( m_generator.pro_state() == generator_state::content_length )
				{
					auto content = m_generator.body_buffer(wire_body);
					return base_write(std::move(header), content, error);
				}
				auto content = m_generator.body_data(wire_body);
				return base_write(std::move(header), std::move(content), error);
			}
			return base_write(std::move(header), error);
		}
		if( wire_body.size() > 0 and m_generator.pro_state() != generator_state::finish )
		{
			auto bytes = write_body(wire_body, error);
			if( error )
				return sum;
			sum += bytes;
		}
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_write
	(const_buffer body, error_code &error, std::chrono::nanoseconds timeout) noexcept
	{
		error.clear();
		size_t sum = 0;

		auto pro_state = m_generator.pro_state();
		std::string compressed {};
		const_buffer wire_body = body;

		if( pro_state == generator_state::finish )
		{
			error = make_error_code(errc::eof);
			co_return sum;
		}
		if( pro_state == generator_state::header )
		{
			auto content_type = m_generator.header(header::content_type)
				.transform([](const value &item) { return item.to_string(); })
				.value_or("application/octet-stream");

			if( m_auto_compression and
				gzip_candidate(content_type, body.size(), false) )
				add_vary_accept_encoding();

			if( should_gzip(content_type, body.size(), false) )
			{
				auto encoded = gzip_compress({
					static_cast<const char*>(body.data()), body.size()
				});
				if( not encoded )
				{
					error = encoded.error();
					co_return sum;
				}
				if( encoded->size() < body.size() or m_req_method == method::head )
				{
					compressed = std::move(*encoded);
					wire_body = buffer(compressed);
					prepare_gzip_headers(false);
				}
			}
		}
		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			if( pro_state == generator_state::header )
			{
				auto header = m_generator.header_data(wire_body.size(), m_req_method);
				if( wire_body.size() > 0 and
					m_generator.pro_state() != generator_state::finish )
				{
					if( m_generator.pro_state() == generator_state::content_length )
					{
						auto content = m_generator.body_buffer(wire_body);
						sum = co_await co_base_write(
							std::move(header), content, error
						);
					}
					else
					{
						auto content = m_generator.body_data(wire_body);
						sum = co_await co_base_write(
							std::move(header), std::move(content), error
						);
					}
				}
				else
				{
					sum = co_await co_base_write(std::move(header), error);
				}
				co_return ;
			}
			if( wire_body.size() > 0 and m_generator.pro_state() != generator_state::finish )
			{
				auto bytes = co_await co_write_body(wire_body, error);
				if( error )
					co_return ;
				sum += bytes;
			}
			co_return ;
		},
		use_awaitable);

		using namespace std::chrono_literals;
		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_connection->get_executor(), timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					error = make_error_code(errc::timed_out);
				else
					error = std::get<1>(var);
			}
		}
		co_return sum;
	}

private:
	[[nodiscard]] bool status_allows_representation() const noexcept
	{
		auto response_status = m_generator.status();
		auto code = static_cast<uint16_t>(response_status);

		if( response_status == status::none )
			code = static_cast<uint16_t>(status::ok);

		return not (code >= 100 and code < 200) and
			   response_status != status::no_content and
			   response_status != status::reset_content and
			   response_status != status::not_modified and
			   response_status != status::partial_content and
			   not (m_req_method == method::connect and code >= 200 and code < 300);
	}

	[[nodiscard]] bool gzip_candidate
	(std::string_view mime, size_t size, bool file) const noexcept
	{
		if( size < 256 or
			not status_allows_representation() or
			not is_compressible_mime_type(mime) or
			m_generator.contains_header(header::content_encoding) or
			header_has_token(m_generator.headers(), header::cache_control, "no-transform") )
			return false;
		if( file )
			return m_req_version >= version::v11;
		return not m_generator.contains_header(header::transfer_encoding);
	}

	[[nodiscard]] bool should_gzip
	(std::string_view mime, size_t size, bool file) const noexcept
	{
		return m_auto_compression and m_client_accepts_gzip and
			gzip_candidate(mime, size, file) and (not file or m_req_range.empty());
	}

	void set_gzip_variant_etag() noexcept
	{
		auto it = m_generator.headers().find(header::etag);
		if( it == m_generator.headers().end() )
			return ;

		if( auto tag = parse_entity_tag(it->second.to_string()) )
		{
			m_generator.set_header (
				header::etag, "W/\"" + tag->opaque + "-gzip\""
			);
		}
		else
			m_generator.unset_header(header::etag);
	}

	void add_vary_accept_encoding() noexcept
	{
		auto it = m_generator.headers().find(header::vary);
		if( it == m_generator.headers().end() )
		{
			m_generator.set_header(header::vary, header::accept_encoding);
			return ;
		}
		if( not header_has_token(m_generator.headers(), header::vary, "*") and
			not header_has_token(m_generator.headers(), header::vary, header::accept_encoding) )
		{
			m_generator.set_header(header::vary,
				it->second.to_string() + ", " + header::accept_encoding
			);
		}
	}

	void prepare_gzip_headers(bool streaming) noexcept
	{
		m_generator
		.set_header(header::content_encoding, "gzip")
		.unset_header(header::content_length);

		if( streaming )
			m_generator.set_header(header::transfer_encoding, "chunked");
		else
			m_generator.unset_header(header::transfer_encoding);

		add_vary_accept_encoding();
		set_gzip_variant_etag();
	}

	template <typename Opt>
	[[nodiscard]] static bool precompressed_file(const Opt &token) noexcept
	{
		if( is_precompressed_mime_type(token.mime_type) )
			return true;

		if constexpr( requires { token.file_name; } )
		{
			auto extension = strtls::to_lower(token.file_name.extension().string());
			return extension == ".svgz";
		}
		return false;
	}

	template <typename Opt>
	void prepare_file_encoding(const Opt &token) noexcept
	{
		m_generator.set_header(header::content_type, token.mime_type);
		auto candidate = not precompressed_file(token) and
			gzip_candidate(token.mime_type, token.file_size, true);

		if( m_auto_compression and candidate )
			add_vary_accept_encoding();

		m_file_gzip = candidate and should_gzip(
			token.mime_type, token.file_size, true
		);
		if( m_file_gzip )
			prepare_gzip_headers(true);
	}

	void cancel_file_content_encoding() noexcept
	{
		if( not m_file_gzip )
			return ;

		m_file_gzip = false;
		m_generator
		.unset_header(header::content_encoding)
		.unset_header(header::transfer_encoding)
		.unset_header(header::content_length);
	}

public:
	template <typename Opt>
	[[nodiscard]] size_t send_file(Opt &&opt, error_code &error) noexcept
	{
		if( m_generator.pro_state() != generator_state::header )
			return 0;

		auto f_token = make_file_opt_token(std::forward<Opt>(opt));
		if( not f_token )
		{
			error = f_token.error();
			return 0;
		}
		set_file_validators(*f_token);
		prepare_file_encoding(*f_token);

		if( auto result = precondition_status(); result != status::none )
		{
			if( result == status::precondition_failed )
				cancel_file_content_encoding();

			auto length = result == status::not_modified and not m_file_gzip ?
				f_token->file_size : 0;

			m_generator.set_status(result);
			if( result == status::not_modified and m_file_gzip )
				m_generator.unset_header(header::content_length);
			else
				m_generator.set_header(header::content_length, length);

			return write_header(length, error);
		}
		if( m_req_method != method::get or m_req_range.empty() or
			not if_range_matches() or f_token->file_size == 0 )
			return default_transfer(*f_token, error);

		auto specifier = parse_range_header(m_req_range);
		if( not specifier or specifier->unit != "bytes" or specifier->ranges.size() > 16 )
			return default_transfer(*f_token, error);

		auto resolved = resolve_byte_ranges(*specifier, f_token->file_size);
		if( resolved.empty() )
		{
			m_generator
			.set_status(status::range_not_satisfiable)
			.set_header(header::accept_ranges, "bytes")
			.set_header(header::content_length, 0)
			.set_header(header::content_range,
				format_unsatisfied_content_range(f_token->file_size)
			);
			return write_header(0, error);
		}
		if( excessive_range_set(resolved, f_token->file_size) )
			return default_transfer(*f_token, error);

		return range_transfer(*f_token,
			make_range_values(resolved, f_token->file_size), error
		);
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_send_file
	(Opt &&opt, error_code &error, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() != generator_state::header )
			co_return 0;

		auto f_token = make_file_opt_token(std::forward<Opt>(opt));
		if( not f_token )
		{
			error = f_token.error();
			co_return 0;
		}
		set_file_validators(*f_token);
		prepare_file_encoding(*f_token);

		using namespace std::chrono_literals;
		using namespace libgs::operators;
		size_t sum = 0;

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			if( auto result = precondition_status(); result != status::none )
			{
				if( result == status::precondition_failed )
					cancel_file_content_encoding();

				auto length = result == status::not_modified and not m_file_gzip ?
					f_token->file_size : 0;

				m_generator.set_status(result);
				if( result == status::not_modified and m_file_gzip )
					m_generator.unset_header(header::content_length);
				else
					m_generator.set_header(header::content_length, length);

				sum = co_await co_write_header(length, error);
				co_return ;
			}
			if( m_req_method != method::get or m_req_range.empty() or
				not if_range_matches() or f_token->file_size == 0 )
			{
				sum = co_await co_default_transfer(*f_token, error);
				co_return ;
			}
			auto specifier = parse_range_header(m_req_range);
			if( not specifier or specifier->unit != "bytes" or specifier->ranges.size() > 16 )
			{
				sum = co_await co_default_transfer(*f_token, error);
				co_return ;
			}
			auto resolved = resolve_byte_ranges(*specifier, f_token->file_size);
			if( resolved.empty() )
			{
				m_generator
				.set_status(status::range_not_satisfiable)
				.set_header(header::accept_ranges, "bytes")
				.set_header(header::content_length, 0)
				.set_header(header::content_range,
					format_unsatisfied_content_range(f_token->file_size)
				);
				sum = co_await co_write_header(0, error);
			}
			else if( excessive_range_set(resolved, f_token->file_size) )
			{
				sum = co_await co_default_transfer(*f_token, error);
			}
			else
			{
				sum = co_await co_range_transfer (
					*f_token, make_range_values(resolved, f_token->file_size), error
				);
			}
			co_return ;
		},
		use_awaitable);

		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_connection->get_executor(), timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					error = make_error_code(errc::timed_out);
				else
					error = std::get<1>(var);
			}
		}
		co_return sum;
	}

public:
	[[nodiscard]] size_t chunk_end(const headers_t &headers, error_code &error) noexcept
	{
		if( m_generator.pro_state() != generator_state::chunk )
			return 0;
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return base_write(std::move(buf), error);
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers, error_code &error) noexcept
	{
		if( m_generator.pro_state() != generator_state::chunk )
			co_return 0;
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;
		co_return co_await co_base_write(std::move(buf), error);
	}

private:
	template <typename Opt>
	[[nodiscard]] sys_expected<size_t> gzip_file_size(Opt &token) noexcept
	{
		gzip_encoder encoder;
		constexpr size_t buf_size = 64 * 1024;

		char data[buf_size] {};
		size_t result = 0;

		token.stream->clear();
		token.stream->seekg(0);

		if( not *token.stream )
			return sys_unexpected(make_error_code(std::errc::io_error));
		for(;;)
		{
			token.stream->read(data, buf_size);
			auto count = token.stream->gcount();

			if( count == 0 )
			{
				if( token.stream->eof() )
					break;
				return sys_unexpected(make_error_code(std::errc::io_error));
			}
			auto encoded = encoder.append({
				data, static_cast<size_t>(count)
			});
			if( not encoded )
				return sys_unexpected(encoded.error());

			if( encoded->size() > std::numeric_limits<size_t>::max() - result )
				return sys_unexpected(make_error_code(std::errc::value_too_large));

			result += encoded->size();
			if( token.stream->eof() )
				break;

			if( not *token.stream )
				return sys_unexpected(make_error_code(std::errc::io_error));
		}
		auto encoded = encoder.append({}, true);
		if( not encoded )
			return sys_unexpected(encoded.error());

		if( encoded->size() > std::numeric_limits<size_t>::max() - result )
			return sys_unexpected(make_error_code(std::errc::value_too_large));
		result += encoded->size();

		token.stream->clear();
		token.stream->seekg(0);

		if( not *token.stream )
			return sys_unexpected(make_error_code(std::errc::io_error));
		return result;
	}

	template <typename Opt>
	[[nodiscard]] size_t gzip_transfer(Opt &token, error_code &error) noexcept
	{
		m_generator
		.set_status(status::ok)
		.unset_header(header::content_range)
		.set_header(header::accept_ranges, "none")
		.set_header(header::content_type, token.mime_type);

		size_t body_size = 0;
		if( m_req_method == method::head )
		{
			auto expected = gzip_file_size(token);
			if( not expected )
			{
				error = expected.error();
				return 0;
			}
			body_size = *expected;
		}
		auto sum = write_header(body_size, error);
		if( error or m_generator.pro_state() == generator_state::finish )
			return sum;

		gzip_encoder encoder;
		constexpr size_t buf_size = 64 * 1024;
		char data[buf_size] {};

		token.stream->clear();
		token.stream->seekg(0);

		while( not token.stream->eof() )
		{
			token.stream->read(data, buf_size);
			auto size = static_cast<size_t>(token.stream->gcount());

			if( size == 0 )
			{
				if( not token.stream->eof() )
					error = make_error_code(std::errc::io_error);
				break;
			}
			auto encoded = encoder.append({data, size});
			if( not encoded )
			{
				error = encoded.error();
				return sum;
			}
			if( not encoded->empty() )
				sum += write_body(buffer(*encoded), error);
			if( error )
				return sum;
		}
		if( error )
			return sum;

		auto encoded = encoder.append({}, true);
		if( not encoded )
		{
			error = encoded.error();
			return sum;
		}
		if( not encoded->empty() )
			sum += write_body(buffer(*encoded), error);

		if( error )
			return sum;

		if( auto end = m_generator.chunk_end_data({}); not end.empty() )
			sum += base_write(std::move(end), error);
		return sum;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_gzip_transfer
	(Opt &token, error_code &error) noexcept
	{
		m_generator
		.set_status(status::ok)
		.unset_header(header::content_range)
		.set_header(header::accept_ranges, "none")
		.set_header(header::content_type, token.mime_type);

		size_t body_size = 0;
		if( m_req_method == method::head )
		{
			auto expected = gzip_file_size(token);
			if( not expected )
			{
				error = expected.error();
				co_return 0;
			}
			body_size = *expected;
		}
		auto sum = co_await co_write_header(body_size, error);
		if( error or m_generator.pro_state() == generator_state::finish )
			co_return sum;

		gzip_encoder encoder;
		constexpr size_t buf_size = 64 * 1024;
		char data[buf_size] {};

		token.stream->clear();
		token.stream->seekg(0);

		while( not token.stream->eof() )
		{
			token.stream->read(data, buf_size);
			auto size = static_cast<size_t>(token.stream->gcount());

			if( size == 0 )
			{
				if( not token.stream->eof() )
					error = make_error_code(std::errc::io_error);
				break;
			}
			auto encoded = encoder.append({data, size});
			if( not encoded )
			{
				error = encoded.error();
				co_return sum;
			}
			if( not encoded->empty() )
				sum += co_await co_write_body(buffer(*encoded), error);

			if( error )
				co_return sum;
		}
		if( error )
			co_return sum;

		auto encoded = encoder.append({}, true);
		if( not encoded )
		{
			error = encoded.error();
			co_return sum;
		}
		if( not encoded->empty() )
			sum += co_await co_write_body(buffer(*encoded), error);

		if( error )
			co_return sum;

		if( auto end = m_generator.chunk_end_data({}); not end.empty() )
			sum += co_await co_base_write(std::move(end), error);
		co_return sum;
	}

	template <typename Opt>
	[[nodiscard]] size_t default_transfer(Opt &token, error_code &error) noexcept
	{
		if( m_file_gzip )
			return gzip_transfer(token, error);

		m_generator
		.set_status(status::ok)
		.unset_header(header::content_range)
		.set_header(header::accept_ranges, "bytes")
		.set_header(header::content_type, token.mime_type);

		auto sum = write_header(token.file_size, error);
		if( error or token.file_size == 0 or
			m_generator.pro_state() == generator_state::finish )
			return sum;

		token.stream->seekg(0);
		while( not token.stream->eof() )
		{
			constexpr size_t buf_size = 0xFFFF;
			char fr_buf[buf_size] {0};

			token.stream->read(fr_buf, buf_size);
			auto size = token.stream->gcount();

			if( size == 0 )
				break;

			sum += write_body(buffer(fr_buf, size), error);
			if( error )
				break;
		}
		return sum;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_default_transfer
	(Opt &token, error_code &error) noexcept
	{
		if( m_file_gzip )
			co_return co_await co_gzip_transfer(token, error);

		m_generator
		.set_status(status::ok)
		.unset_header(header::content_range)
		.set_header(header::accept_ranges, "bytes")
		.set_header(header::content_type, token.mime_type);

		auto sum = co_await co_write_header(token.file_size, error);
		if( error or token.file_size == 0 or
			m_generator.pro_state() == generator_state::finish )
			co_return sum;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		token.stream->seekg(0);
		while( not token.stream->eof() )
		{
			token.stream->read(fr_buf, buf_size);
			auto size = static_cast<size_t>(token.stream->gcount());

			if( size == 0 )
				break;

			sum += co_await co_write_body(buffer(fr_buf, size), error);
			if( error )
				break;
		}
		co_return sum;
	}

private:
	[[nodiscard]] size_t range_transfer
	(auto &token, const std::vector<range_value> &ranges, error_code &error)
	{
		m_generator.set_status(status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_generator
			.set_header(header::accept_ranges , "bytes"        )
			.set_header(header::content_type  , token.mime_type)
			.set_header(header::content_length, range.total    )
			.set_header(header::content_range,
				format_content_range(range, token.file_size)
			);
			return send_range(token.stream, "", "", ranges, error);
		}
		using namespace std::chrono;

		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()
			).count()
		);
		m_generator.set_header(header::content_type,
			"multipart/byteranges; boundary=" + boundary
		);
		m_generator.unset_header(header::content_range);

		auto ct_line = std::format("{}: {}",
			header::content_type, token.mime_type
		);
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

		m_generator
		.set_header(header::content_length, content_length)
		.set_header(header::accept_ranges , "bytes");

		return send_range (
			token.stream, boundary, ct_line, ranges, error
		);
	}

	[[nodiscard]] awaitable<size_t> co_range_transfer
	(auto &token, const std::vector<range_value> &ranges, error_code &error) noexcept
	{
		m_generator.set_status(status::partial_content);
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_generator
			.set_header(header::accept_ranges , "bytes"          )
			.set_header(header::content_type  , token.mime_type  )
			.set_header(header::content_length, range.total      )
			.set_header(header::content_range,
				format_content_range(range, token.file_size)
			);
			co_return co_await co_send_range (
				token.stream, "", "", ranges, error
			);
		}
		using namespace std::chrono;

		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
		m_generator.set_header(header::content_type,
			"multipart/byteranges; boundary=" + boundary
		);
		m_generator.unset_header(header::content_range);

		auto ct_line = std::format (
			"{}: {}", header::content_type, token.mime_type
		);
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

		m_generator
		.set_header(header::content_length, content_length)
		.set_header(header::accept_ranges , "bytes");

		co_return co_await co_send_range (
			token.stream, boundary, ct_line, ranges, error
		);
	}

private:
	template <typename FS>
	[[nodiscard]] size_t send_range (
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::vector<range_value> ranges, error_code &error
	) noexcept
	{
		assert(not ranges.empty());
		auto sum = write_header(0, error);
		if( error )
			return sum;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size + 2] {0};

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
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					break;
				value.total -= static_cast<size_t>(size);
			}
			return sum;
		}
		for(auto &value: ranges)
		{
			auto body = std::format (
				"--{}\r\n"
				"{}\r\n"
				"{}\r\n"
				"\r\n",
				boundary,
				ct_line,
				value.cr_line
			);
			sum += write_body(buffer(body, body.size()), error);
			if( error )
				return sum;

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

					sum += write_body(buffer(buf, size + 2), error);
					if( error )
						return sum;
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				sum += write_body(buffer(buf,size), error);
				if( error )
					return sum;
				value.total -= static_cast<size_t>(size);
			}
		}
		auto abuf = std::format("--{}--\r\n", boundary);
		sum += write_body(buffer(abuf, abuf.size()), error);
		return sum;
	}

	template <typename FS>
	[[nodiscard]] awaitable<size_t> co_send_range (
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::vector<range_value> ranges, error_code &error
	) noexcept
	{
		assert(not ranges.empty());
		auto sum = co_await co_write_header(0, error);
		if( error )
			co_return sum;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size + 2] {0};

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
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					break;
				value.total -= size;
			}
			co_return sum;
		}
		for(auto &value : ranges)
		{
			auto body = std::format (
				"--{}\r\n"
				"{}\r\n"
				"{}\r\n"
				"\r\n",
				boundary,
				ct_line,
				value.cr_line
			);
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
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				sum += co_await co_write_body(buffer(buf,size), error);
				if( error )
					co_return sum;
				value.total -= size;
			}
		}
		auto abuf = std::format("--{}--\r\n", boundary);
		sum += co_await co_write_body(buffer(abuf, abuf.size()), error);
		co_return sum;
	}

private:
	[[nodiscard]] size_t write_header(size_t size, error_code &error) noexcept {
		return base_write(m_generator.header_data(size, m_req_method), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_header
	(size_t size, error_code &error) noexcept
	{
		co_return co_await co_base_write (
			m_generator.header_data(size, m_req_method), error
		);
	}

	[[nodiscard]] size_t write_body(const const_buffer &body, error_code &error) noexcept {
		if( m_generator.pro_state() == generator_state::content_length )
			return base_write(m_generator.body_buffer(body), error);
		return base_write(m_generator.body_data(body), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_body
	(const const_buffer &body, error_code &error) noexcept
	{
		if( m_generator.pro_state() == generator_state::content_length )
		{
			co_return co_await co_base_write(m_generator.body_buffer(body), error);
		}
		co_return co_await co_base_write(m_generator.body_data(body), error);
	}

private:
	[[nodiscard]] size_t base_write(const_buffer data, error_code &error) noexcept
	{
		error.clear();
		auto sum = m_connection->write(data, error);
		if( error )
			ignore_unused(m_connection->close());
		return sum;
	}

	[[nodiscard]] size_t base_write(std::string &&data, error_code &error) noexcept
	{
		error.clear();
		auto sum = m_connection->write(data, error);
		if( error )
			ignore_unused(m_connection->close());
		return sum;
	}

	[[nodiscard]] size_t base_write
	(std::string &&header, const_buffer body, error_code &error) noexcept
	{
		error.clear();
		const const_buffer buffers[] {
			const_buffer(header), body
		};
		auto sum = m_connection->write(buffers, error);
		if( error )
			ignore_unused(m_connection->close());
		return sum;
	}

	[[nodiscard]] size_t base_write
	(std::string &&header, std::string &&body, error_code &error) noexcept
	{
		error.clear();
		const const_buffer buffers[] {
			const_buffer(header), const_buffer(body)
		};
		auto sum = m_connection->write(buffers, error);
		if( error )
			ignore_unused(m_connection->close());
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_base_write
	(const_buffer data, error_code &error) noexcept
	{
		error.clear();
		using namespace libgs::operators;
		auto sum = co_await m_connection->write (
			data, use_awaitable | error
		);
		if( error )
			ignore_unused(m_connection->close());
		co_return sum;
	}

	[[nodiscard]] awaitable<size_t> co_base_write
	(std::string data, error_code &error) noexcept
	{
		error.clear();
		auto data_buffer = buffer(data);
		using namespace libgs::operators;
		auto sum = co_await m_connection->write (
			data_buffer, use_awaitable | error
		);

		if( error )
			ignore_unused(m_connection->close());
		co_return sum;
	}

	[[nodiscard]] awaitable<size_t> co_base_write(std::string header,
		const_buffer body, error_code &error) noexcept
	{
		error.clear();
		const const_buffer buffers[] {
			const_buffer(header), body
		};
		using namespace libgs::operators;
		auto sum = co_await m_connection->write (
			buffers, use_awaitable | error
		);

		if( error )
			ignore_unused(m_connection->close());
		co_return sum;
	}

	[[nodiscard]] awaitable<size_t> co_base_write(std::string header,
		std::string body, error_code &error) noexcept
	{
		error.clear();
		const const_buffer buffers[] {
			const_buffer(header), const_buffer(body)
		};
		using namespace libgs::operators;
		auto sum = co_await m_connection->write (
			buffers, use_awaitable | error
		);

		if( error )
			ignore_unused(m_connection->close());
		co_return sum;
	}

private:
	template <typename Opt>
	void set_file_validators(const Opt &token) noexcept
	{
		if constexpr( requires { token.file_name; } )
		{
			if( token.file_name.empty() )
				return ;

			std::error_code error {};
			auto file_time = std::filesystem::last_write_time(token.file_name, error);
			if( error )
				return ;

			auto modified = std::chrono::time_point_cast<std::chrono::system_clock::duration> (
				file_time - decltype(file_time)::clock::now() +
				std::chrono::system_clock::now()
			);
			auto seconds = std::chrono::duration_cast<std::chrono::seconds> (
				modified.time_since_epoch()
			).count();

			if( not m_generator.contains_header(header::last_modified) )
				m_generator.set_header(header::last_modified, format_http_date(modified));

			if( not m_generator.contains_header(header::etag) )
			{
				m_generator.set_header(header::etag,
					std::format("W/\"{:x}-{:x}\"", token.file_size, seconds)
				);
			}
		}
	}

	[[nodiscard]] status_enum precondition_status() const noexcept
	{
		switch( evaluate_preconditions
			(m_req_method, m_req_headers, m_generator.headers(), true) )
		{
		case precondition_result::not_modified:
			return status::not_modified;
		case precondition_result::precondition_failed:
			return status::precondition_failed;
		default:
			break;
		}
		return status::none;
	}

	[[nodiscard]] bool if_range_matches() const noexcept
	{
		if( m_req_if_range.empty() )
			return true;

		const auto &headers = m_generator.headers();
		if( m_req_if_range.starts_with("W/") )
			return false;

		if( m_req_if_range.starts_with('"') )
		{
			auto it = headers.find(header::etag);
			return it != headers.end() and
				strong_entity_tag_equal(it->second.to_string(), m_req_if_range);
		}
		auto it = headers.find(header::last_modified);
		if( it == headers.end() )
			return false;

		auto validator = parse_http_date(m_req_if_range);
		auto modified = parse_http_date(it->second.to_string());

		return validator and modified and
			   std::chrono::floor<std::chrono::seconds>(*modified) <=
			   std::chrono::floor<std::chrono::seconds>(*validator);
	}

	[[nodiscard]] static bool excessive_range_set
	(const file_ranges &ranges, size_t complete_length) noexcept
	{
		size_t total = 0;
		for(auto &range : ranges)
		{
			if( range.total > complete_length - total )
				return true;
			total += range.total;
		}
		return false;
	}

	[[nodiscard]] std::vector<range_value> make_range_values
	(const file_ranges &ranges, size_t complete_length) const
	{
		std::vector<range_value> result {};
		result.reserve(ranges.size());

		for(auto &range : ranges)
		{
			range_value value {};
			value.begin = range.begin;
			value.total = range.total;

			value.cr_line = std::format("{}: {}", header::content_range,
				format_content_range(range, complete_length)
			);
			result.emplace_back(std::move(value));
		}
		return result;
	}

	template <typename Opt>
	auto make_file_opt_token(Opt &&opt) noexcept
	{
		using opt_t = std::remove_cvref_t<Opt>;
		if constexpr( is_any_string_v<opt_t> or std::same_as<opt_t,std::filesystem::path> or
			is_fstream_v<opt_t,char> or is_ifstream_v<opt_t,char> )
		{
			auto token = http::make_file_opt_token(std::forward<Opt>(opt));
			using token_t = decltype(token);

			auto expected = token.init(std::ios::in | std::ios::binary);
			if( expected )
				return sys_expected<token_t>(std::move(token));
			return sys_expected<token_t>(sys_unexpected(expected.error()));
		}
		else
		{
			if( opt.stream->is_open() )
				return sys_expected<opt_t>(std::forward<Opt>(opt));

			auto expected = opt.init(std::ios::in | std::ios::binary);
			if( expected )
				return sys_expected<opt_t>(std::forward<Opt>(opt));
			return sys_expected<opt_t>(sys_unexpected(expected.error()));
		}
	}

public:
	connection_ptr m_connection {};
	generator_t m_generator {};

	method_enum m_req_method = method::get;
	version_enum m_req_version = version::v11;

	headers_t m_req_headers {};
	std::string m_req_range {};
	std::string m_req_if_range {};

	bool m_auto_compression = false;
	bool m_client_accepts_gzip = false;
	bool m_file_gzip = false;
};

template <core_concepts::exec Exec>
basic_response<Exec>::basic_response(connection_ptr connection) :
	mutable_headers<basic_response>(nullptr),
	mutable_cookies<cookie,basic_response>(nullptr),
	mutable_chunk_attributes<basic_response>(nullptr),
	m_impl(new impl(std::move(connection)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <core_concepts::exec Exec>
basic_response<Exec>::~basic_response()
{
	delete m_impl;
}

template <core_concepts::exec Exec>
version_enum basic_response<Exec>::version() const noexcept
{
	return m_impl->m_generator.version();
}

template <core_concepts::exec Exec>
basic_response<Exec> &basic_response<Exec>::set_status(status_enum status)
{
	m_impl->m_generator.set_status(status);
	return *this;
}

template <core_concepts::exec Exec>
basic_response<Exec> &basic_response<Exec>::auto_set(request_t &request)
{
	m_impl->m_req_method = request.method();
	m_impl->m_req_version = request.version();
	m_impl->m_req_headers = request.headers();

	m_impl->m_client_accepts_gzip = request.support_gzip();
	m_impl->m_auto_compression = gzip_available_v;

	m_impl->m_req_range.clear();
	m_impl->m_req_if_range.clear();
	m_impl->m_file_gzip = false;

	if( version() < http::version::v11 )
		return *this;

	auto it = request.headers().find(header::range);
	if( it != request.headers().end() )
		m_impl->m_req_range = it->second.to_string();

	it = request.headers().find(header::if_range);
	if( it != request.headers().end() )
		m_impl->m_req_if_range = it->second.to_string();
	return *this;
}

template <core_concepts::exec Exec>
basic_response<Exec>&
basic_response<Exec>::set_auto_compression(bool enabled) noexcept
{
	m_impl->m_auto_compression = gzip_available_v and enabled;
	return *this;
}

template <core_concepts::exec Exec>
bool basic_response<Exec>::auto_compression() const noexcept
{
	return m_impl->m_auto_compression;
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::write(const const_buffer &body, Token &&token)
	requires task_token_v<Token,size_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->write(body, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->write(body, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http::basic_response::write"
			);
		}
		return sum;
	}
	else if constexpr( is_detached_v<token_unbound_t<token_t>> )
	{
		auto data = std::make_shared<std::string>();
		if( body.size() > 0 )
		{
			data->assign (
				static_cast<const char*>(body.data()), body.size()
			);
		}
		return detail::initiate_expected<size_t>(get_executor(),
		[this, data = std::move(data)]() mutable -> awaitable<sys_expected<size_t>>
		{
			error_code error {};

			auto sum = co_await m_impl->co_write (
				buffer(*data), error, std::chrono::nanoseconds::zero()
			);
			if( error )
				co_return sys_unexpected(error);
			co_return sum;
		},
		std::forward<Token>(token));
	}
	else
	{
		return detail::initiate_expected<size_t>(get_executor(),
		[this, body]() mutable -> awaitable<sys_expected<size_t>>
		{
			error_code error {};

			auto sum = co_await m_impl->co_write (
				body, error, std::chrono::nanoseconds::zero()
			);
			if( error )
				co_return sys_unexpected(error);
			co_return sum;
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::write(Token &&token)
	requires task_token_v<Token,size_t>
{
	return write({nullptr,0}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename T, typename Token>
auto basic_response<Exec>::send_file(T &&opt, Token &&token)
	requires file_task_token_v<T,Token>
{
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->send_file(std::forward<T>(opt), token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->send_file (
			std::forward<T>(opt), error
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http::basic_response::send_file"
			);
		}
		return sum;
	}
	else
	{
		return detail::initiate_expected<size_t>(get_executor(),
		[this, opt = detail::capture_async_argument(std::forward<T>(opt))]
		() mutable -> awaitable<sys_expected<size_t>>
		{
			error_code error {};

			auto sum = co_await m_impl->co_send_file(
				detail::unwrap_async_argument(opt), error,
				std::chrono::nanoseconds::zero()
			);
			if( error )
				co_return sys_unexpected(error);
			co_return sum;
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::redirect
(core_concepts::text_p<char> auto &&url, redirect_enum redi, Token &&token)
	requires task_token_v<Token,size_t>
{
	m_impl->m_generator.set_redirect(std::forward<decltype(url)>(url), redi);
	return write({nullptr,0}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::redirect
(core_concepts::text_p<char> auto &&url, Token &&token)
	requires task_token_v<Token,size_t>
{
	return redirect (
		std::forward<decltype(url)>(url),
		redirect_enum::moved_permanently,
		std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::continues(Token &&token)
	requires task_token_v<Token,size_t>
{
	this->set_status(http::status::continue_upload);
	return write({nullptr,0}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::chunk_end(const headers_t &headers, Token &&token)
	requires task_token_v<Token,size_t>
{
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->chunk_end(headers, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->chunk_end(headers, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http::basic_response::chunk_end"
			);
		}
		return sum;
	}
	else
	{
		return detail::initiate_expected<size_t>(get_executor(),
		[this, headers]() mutable -> awaitable<sys_expected<size_t>>
		{
			error_code error {};

			auto sum = co_await m_impl->co_chunk_end(headers, error);
			if( error )
				co_return sys_unexpected(error);
			co_return sum;
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_response<Exec>::chunk_end(Token &&token)
	requires task_token_v<Token,size_t>
{
	return chunk_end({}, std::forward<Token>(token));
}

template <core_concepts::exec Exec>
status_enum basic_response<Exec>::status() const noexcept
{
	return m_impl->m_generator.status();
}

template <core_concepts::exec Exec>
bool basic_response<Exec>::is_finished() const noexcept
{
	return m_impl->m_generator.pro_state() == generator_state::finish;
}

template <core_concepts::exec Exec>
basic_response<Exec>::executor_t basic_response<Exec>::get_executor() noexcept
{
	return m_impl->m_connection->get_executor();
}

template <core_concepts::exec Exec>
basic_response<Exec> &basic_response<Exec>::cancel() noexcept
{
	ignore_unused(m_impl->m_connection->cancel());
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
