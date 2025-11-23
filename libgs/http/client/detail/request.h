
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H

#include <libgs/core/algorithm/uuid.h>
#include <libgs/core/string_vector.h>
#include <libgs/coro/utils.h>

namespace libgs::http
{

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)

public:
	using sock_helper_t = socket_operation_helper<typename session_t::socket_t>;

public:
	impl(session_t &&session, url_t url, request_arg_t arg) :
		m_session(std::move(session)), m_generator(std::move(url), std::move(arg)) {}

	impl(impl &&other) noexcept :
		m_session(std::move(other.m_session)),
		m_generator(std::move(other.m_generator)) {}

	impl& operator=(impl &&other) noexcept
	{
		m_session = std::move(other.m_session);
		m_generator = std::move(other.m_generator);
		return *this;
	}

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token>
	[[nodiscard]] auto write(const const_buffer &body, Token &&token)
	{
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_error_code_token_v<Token> )
		{
			return _write(body)
				.or_else([&token](const error_code &error) {
					token = error;
				});
		}
		else if constexpr( is_sync_opt_token_v<Token> )
			return _write(body);

		else if constexpr( is_redirect_time_v<token_t> )
		{
			decltype(auto) ntoken = unbound_redirect_time(token);
			using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

			decltype(auto) nntoken = unbound_token(ntoken);
			using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

			if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
			{
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					return _co_write(ntoken.ec_, body,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
				else
				{
					return _co_write(body,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
			}
			else if constexpr( is_use_future_v<nntoken_t> )
			{
				auto buf_ptr = std::make_shared<std::string>(
					static_cast<const char*>(body.data()), body.size()
				);
				auto promise = std::make_shared<std::promise<io_expected>>();
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					libgs::dispatch(m_session.get_executor(), [self = this->shared_from_this(),
						ntoken, buf = std::move(buf_ptr), promise = std::move(promise),
						cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						promise->set_value(co_await self->_co_write (
							ntoken.ec_, {buf->data(), buf->size()}, cancel_slot, timeout
						));
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(m_session.get_executor(), [self = this->shared_from_this(),
						buf = std::move(buf_ptr), promise = std::move(promise),
						cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						promise->set_value(co_await self->_co_write (
							{buf->data(), buf->size()}, cancel_slot, timeout
						));
						co_return ;
					});
				}
				return promise->get_future();
			}
			else if constexpr( is_detached_v<nntoken_t> )
				_write_detach(body);
			else
			{
				auto buf_ptr = std::make_shared<std::string>(
					static_cast<const char*>(body.data()), body.size()
				);
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					libgs::dispatch(m_session.get_executor(), [self = this->shared_from_this(), ntoken, nntoken,
						buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
						cancel_slot = asio::get_associated_cancellation_slot(ntoken)
					]() mutable -> awaitable<void>
					{
						auto expected = co_await self->_co_write (
							ntoken.ec_, buf, cancel_slot, timeout
						);
						expected
						.transform([&callback = nntoken](int code) {
							callback(error_code(), code);
						})
						.or_else([&callback = nntoken](const error_code &error) {
							callback(error, 255);
						});
					});
				}
				else
				{
					libgs::dispatch(m_session.get_executor(), [self = this->shared_from_this(), nntoken,
						buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
						cancel_slot = asio::get_associated_cancellation_slot(ntoken)
					]() mutable -> awaitable<void>
					{
						auto expected = co_await self->_co_write (
							buf, cancel_slot, timeout
						);
						expected
						.transform([&callback = nntoken](int code) {
							callback(error_code(), code);
						})
						.or_else([&callback = nntoken](const error_code &error) {
							callback(error, 255);
						});
					});
				}
			}
		}
		else
		{
			using namespace libgs::operators;
			using namespace std::chrono_literals;
			return write(body, token | 0ns);
		}
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
	[[nodiscard]] io_expected send_file(Opt &&opt) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::header )
			return 0;

		fot_data data = 0;
		error_code error;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			return io_unexpected(error);

		if( not token.ranges.empty() )
		{
			auto ranges = from_file_range(token.ranges, data.fsize, error);
			if( error )
				return io_unexpected(error);
			return range_transfer(token, ranges, data);
		}
		auto it = m_generator.arg().headers().find(protocol::header::range);
		if( it == m_generator.arg().headers().end() )
			return default_transfer(token, data);

		std::vector<range_value> ranges;
		return range_text_parsing(it->second.to_string(), data.fsize, ranges) ?
			range_transfer(token, ranges, data) : default_transfer(token, data);
	}

	template <typename Opt>
	[[nodiscard]] awaitable<io_expected> co_send_file(Opt &&opt,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::header )
			co_return 0;

		fot_data data = 0;
		error_code error;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), data, error);
		if( error )
			co_return io_unexpected(error);

		auto task = libgs::dispatch(m_session.get_executor(), [&]() mutable -> awaitable<io_expected>
		{
			if( not token.ranges.empty() )
			{
				auto ranges = from_file_range(token.ranges, data.fsize, error);
				if( error )
					co_return io_unexpected(error);
				co_return co_await co_range_transfer(token, ranges, data, cancel_slot);
			}
			auto it = m_generator.arg().headers().find(protocol::header::range);
			if( it == m_generator.arg().headers().end() )
				co_return co_await co_default_transfer(token, data, cancel_slot);

			std::vector<range_value> ranges;
			co_return range_text_parsing(it->second.to_string(), data.fsize, ranges) ?
				co_await co_range_transfer(token, ranges, data, std::move(cancel_slot)) :
				co_await co_default_transfer(token, data, std::move(cancel_slot));
		},
		use_awaitable);

		using namespace std::chrono_literals;
		io_expected expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_session.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<io_expected> co_send_file(std::error_code &error,
		Opt &&opt, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_send_file(std::forward<Opt>(opt),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

	template <typename Opt>
	void send_file_detach(Opt &&opt) noexcept
	{
		libgs::dispatch(m_session.get_executor(),
			co_send_file(std::forward<Opt>(opt)), detached
		);
	}

public:
	[[nodiscard]] io_expected chunk_end(const headers_t &headers) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::chunk )
			return 0;
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return write_body(buffer(buf));
	}

	[[nodiscard]] awaitable<io_expected> co_chunk_end(const headers_t &headers,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::chunk )
			co_return 0;
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;

		using namespace std::chrono_literals;
		auto task = co_write_body(buffer(buf), std::move(cancel_slot));
		io_expected expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_session.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	[[nodiscard]] awaitable<io_expected> co_chunk_end(std::error_code &error, const headers_t &headers,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_chunk_end(headers,
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

	void chunk_end_detach(const headers_t &headers) noexcept {
		libgs::dispatch(m_session.get_executor(), co_chunk_end(headers), detached);
	}

private:
	[[nodiscard]] io_expected _write(const const_buffer &body) noexcept
	{
		if( m_generator.pro_state() == protocol::generator_state::finish )
			return 0;

		size_t sum = 0;
		if( m_generator.pro_state() == protocol::generator_state::header )
		{
			auto expected = write_header(body.size()).transform([&](size_t size) {
				sum += size;
			});
			if( not expected )
				return expected;
		}
		if( body.size() > 0 )
		{
			auto expected = write_body(body).transform([&](size_t size) {
				sum += size;
			});
			if( not expected )
				return expected;
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> _co_write(const const_buffer &body,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() == protocol::generator_state::finish )
			co_return 0;

		auto task = libgs::dispatch(m_session.get_executor(), [&]() mutable -> awaitable<io_expected>
		{
			size_t sum = 0;
			if( m_generator.pro_state() == protocol::generator_state::header )
			{
				auto expected = co_await co_write_header(body.size(), cancel_slot);
				if( expected )
					 sum += *expected;
				else
					co_return expected;
			}
			if( body.size() > 0 )
			{
				auto expected = co_await co_write_body(body, std::move(cancel_slot));
				if( expected )
					sum += *expected;
				else
					co_return expected;
			}
			co_return sum;
		},
		use_awaitable);

		using namespace std::chrono_literals;
		io_expected expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_session.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	[[nodiscard]] awaitable<io_expected> _co_write(std::error_code &error, const const_buffer &body,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await _co_write(body,
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

	void _write_detach(const const_buffer &body) noexcept {
		libgs::dispatch(m_session.get_executor(), _co_write(body), detached);
	}

private:
	template <typename Opt>
	[[nodiscard]] io_expected default_transfer(Opt &&opt, const fot_data &data) noexcept
	{
		size_t sum = 0;
		if( data.fsize == 0 )
			return sum;

		m_generator.arg().set_header(protocol::header::content_type, data.mtype);
		auto expected = write_header(data.fsize).transform([&](size_t size) {
			sum += size;
		});
		if( not expected )
			return expected;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		opt.stream->seekg(0);
		while( not opt.stream->eof() )
		{
			opt.stream->read(fr_buf, buf_size);
			auto size = opt.stream->gcount();
			if( size == 0 )
				break;

			expected = write_body(buffer(fr_buf, size)).transform([&](size_t s) {
				sum += s;
			});
			if( not expected )
				return expected;
//			sleep_for(512us);
		}
		return sum;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<io_expected> co_default_transfer
	(Opt &&opt, const fot_data &data, asio::cancellation_slot cancel_slot) noexcept
	{
		size_t sum = 0;
		if( data.fsize == 0 )
			co_return sum;

		m_generator.arg().set_header(protocol::header::content_type, data.mtype);
		auto expected = co_await co_write_header(data.fsize, cancel_slot);
		if( expected )
			sum += *expected;
		else
			co_return expected;

		constexpr size_t buf_size = 0xFFFF;
		char fr_buf[buf_size] {0};

		opt.stream->seekg(0);
		while( not opt.stream->eof() )
		{
			opt.stream->read(fr_buf, buf_size);
			auto size = static_cast<size_t>(opt.stream->gcount());
			if( size == 0 )
				break;

			expected = co_await co_write_body(buffer(fr_buf, size), cancel_slot);
			if( expected )
				sum += *expected;
			else
				co_return expected;
//			co_await sleep_for(get_executor(), 512us);
		}
		co_return sum;
	}

public:
	[[nodiscard]] io_expected range_transfer
	(auto &&opt, const std::vector<range_value> &ranges, const fot_data &data) noexcept
	{
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_generator.arg()
			.set_header(protocol::header::accept_ranges , "bytes"    )
			.set_header(protocol::header::content_type  , data.mtype )
			.set_header(protocol::header::content_length, range.total)

			.set_header(protocol::header::content_range , value {
				"{}-{}/{}", range.begin, range.end, range.total
			});
			return send_range(opt.stream, "", "", ranges);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()
			).count()
		);
		m_generator.arg().set_header(protocol::header::content_type,
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

		m_generator.arg()
		.set_header(protocol::header::content_length, content_length)
		.set_header(protocol::header::accept_ranges , "bytes");

		return send_range (
			opt.stream, boundary, ct_line, ranges
		);
	}

	[[nodiscard]] awaitable<io_expected> co_range_transfer(auto &&opt,
		const std::vector<range_value> &ranges, const fot_data &data, asio::cancellation_slot cancel_slot) noexcept
	{
		if( ranges.size() == 1 )
		{
			auto &range = ranges.back();
			m_generator.arg()
			.set_header(protocol::header::accept_ranges , "bytes"    )
			.set_header(protocol::header::content_type  , data.mtype )
			.set_header(protocol::header::content_length, range.total)

			.set_header(protocol::header::content_range, value {
				"{}-{}/{}", range.begin, range.end, range.total
			});
			co_return co_await co_send_range (
				opt.stream, "", "", ranges, std::move(cancel_slot)
			);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}",
			uuid::generate().to_string(),
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
		m_generator.arg().set_header(protocol::header::content_type,
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

		m_generator.arg()
		.set_header(protocol::header::content_length, content_length)
		.set_header(protocol::header::accept_ranges , "bytes");

		co_return co_await co_send_range (
			opt.stream, boundary, ct_line, ranges, std::move(cancel_slot)
		);
	}

private:
	template <typename FS>
	[[nodiscard]] io_expected send_range(
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::vector<range_value> ranges, asio::cancellation_slot cancel_slot
	) noexcept
	{
		assert(not ranges.empty());
		auto expected = write_header(0);
		if( not expected )
			return expected;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] {0};
		size_t sum = 0;

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

					expected = write_body(buffer(buf,size)).transform([&](size_t s) {
						sum += s;
					});
					if( not expected )
						return expected;

//					sleep_for(512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				expected = write_body(buffer(buf,size)).transform([&](size_t s) {
					sum += s;
				});
				if( not expected )
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

			expected = write_body(buffer(body, body.size())).transform([&](size_t s) {
				sum += s;
			});
			if( not expected )
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

					expected = write_body(buffer(buf, size + 2)).transform([&](size_t s) {
						sum += s;
					});
					if( not expected )
						return sum;

//					sleep_for(512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = stream->gcount();

				expected = write_body(buffer(buf,size)).transform([&](size_t s) {
					sum += s;
				});
				if( not expected )
					return sum;

				value.size -= buf_size;
//				sleep_for(512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		expected = write_body(buffer(abuf, abuf.size())).transform([&](size_t s) {
			sum += s;
		});
		if( expected )
			return sum;
		return expected;
	}

	template <typename FS>
	[[nodiscard]] awaitable<io_expected> co_send_range(
		FS &stream, std::string_view boundary, std::string_view ct_line,
		std::vector<range_value> ranges, asio::cancellation_slot cancel_slot
	) noexcept
	{
		assert(not ranges.empty());
		size_t sum = 0;
		auto expected = co_await co_write_header(0, cancel_slot);
		if( not expected )
			sum += *expected;
		else
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

					expected = co_await co_write_body(buffer(buf,size), cancel_slot);
					if( expected )
						sum += *expected;
					else
						co_return expected;

//					co_await sleep_for(get_executor(), 512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				expected = co_await co_write_body(buffer(buf,size), cancel_slot);
				if( expected )
					sum += *expected;
				else
					co_return expected;

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

			expected = co_await co_write_body(buffer(body, body.size()), cancel_slot);
			if( expected )
				sum += *expected;
			else
				co_return expected;

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

					expected = co_await co_write_body(buffer(buf, size + 2), cancel_slot);
					if( expected )
						sum += *expected;
					else
						co_return sum;

//					co_await sleep_for(get_executor(), 512us);
					break;
				}
				stream->read(buf, buf_size);
				auto size = static_cast<size_t>(stream->gcount());

				expected = co_await co_write_body(buffer(buf,size), cancel_slot);
				if( expected )
					sum += *expected;
				else
					co_return sum;

				value.total -= buf_size;
//				co_await sleep_for(get_executor(), 512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		expected = co_await co_write_body(buffer(abuf, abuf.size()), cancel_slot);
		if( expected )
			co_return sum + *expected;
		co_return expected;
	}

private:
	[[nodiscard]] bool range_text_parsing
	(std::string_view range_str_view, size_t file_size, std::vector<range_value> &ranges) noexcept
	{
		std::string range_str(range_str_view.data(), range_str_view.size());
		for(auto i=range_str.size(); i>0; i--)
		{
			if( range_str[i] == 0x20/*SPACE*/ )
				range_str.erase(i,1);
		}
		// bytes=x-y, m-n, i-j ...
		if( range_str.empty() or range_str.substr(0,6) != "bytes=" )
			return false;

		// x-y, m-n, i-j ...
		auto cl_range_str = range_str.substr(6);
		if( cl_range_str.empty() )
			return false;

		// (x-y) ( m-n) ( i-j) ...
		for(auto &sub_range_str : string_vector::from_string(cl_range_str, ','))
		{
			range_value range;
			range.total = 0;

			if( auto str_vector = string_vector::from_string(sub_range_str, '-', false);
				str_vector.size() != 2 )
				return false;

			else if( str_vector[0].empty() )
			{
				if( str_vector[1].empty() )
					return false;

				range.total = *strtls::to_arith<size_t>(str_vector[1]).or_else();
				if( range.total == 0 or range.total > file_size )
					return false;

				range.begin = file_size - range.total;
				range.end   = file_size - 1;
			}
			else if( str_vector[1].empty() )
			{
				if( str_vector[0].empty() )
					return false;
				range.begin = *strtls::to_arith<size_t>(str_vector[0]).or_else();
				range.end   = file_size - 1;

				if( range.begin > range.end )
					return false;
				range.total = file_size - range.begin;
			}
			else
			{
				range.begin = *strtls::to_arith<size_t>(str_vector[0]).or_else();
				range.end   = *strtls::to_arith<size_t>(str_vector[1]).or_else();

				if( range.begin > range.end or range.end >= file_size )
					return false;
				range.total = range.end - range.begin + 1;
			}
			range.cr_line = std::format("{}: bytes {}-{}/{}",
				protocol::header::content_range, range.begin, range.end, file_size
			);
			ranges.emplace_back(std::move(range));
		}
		return true;
	}

	[[nodiscard]] std::vector<range_value> from_file_range
	(const file_ranges &ranges, size_t file_size, error_code &error) noexcept
	{
		std::vector<range_value> vector;
		for(auto &[begin, total] : ranges)
		{
			auto end = begin + total - 1;
			if( total == 0 or end >= file_size )
			{
				error = std::make_error_code(std::errc::invalid_seek);
				break;
			}
			range_value value;
			value.begin = begin;
			value.total = total;
			value.end   = end;

			value.cr_line = std::format("{}: bytes {}-{}/{}",
				protocol::header::content_range, value.begin, value.end, file_size
			);
			vector.emplace_back(std::move(value));
		}
		return vector;
	}


private:
	[[nodiscard]] io_expected write_header(size_t size) noexcept {
		return base_write(m_generator.header_data(method_v, size));
	}

	[[nodiscard]] awaitable<io_expected>
	co_write_header(size_t size, asio::cancellation_slot cancel_slot) noexcept
	{
		co_return co_await co_base_write (
			m_generator.header_data(method_v, size), std::move(cancel_slot)
		);
	}

	[[nodiscard]] io_expected write_body(const const_buffer &body) noexcept {
		return base_write(m_generator.body_data(body));
	}

	[[nodiscard]] awaitable<io_expected>
	co_write_body(const const_buffer &body, asio::cancellation_slot cancel_slot) noexcept
	{
		co_return co_await co_base_write (
			m_generator.body_data(body), std::move(cancel_slot)
		);
	}

private:
	[[nodiscard]] io_expected base_write(std::string &&data) noexcept
	{
		auto &sock_helper = m_session.opt_helper();
		error_code error;

		sock_helper.non_blocking(false, error);
		if( error )
			return io_unexpected(error);

		auto sum = sock_helper.write(data, error);
		if( error )
			return io_unexpected(error);
		return sum;
	}

	[[nodiscard]] awaitable<io_expected>
	co_base_write(std::string &&data, asio::cancellation_slot cancel_slot) noexcept
	{
		auto &sock_helper = m_session.opt_helper();
		error_code error;

		sock_helper.non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto sum = co_await sock_helper.write(data,
			use_awaitable | error | cancel_slot
		);
		if( error )
			co_return io_unexpected(error);
		co_return sum;
	}

private:
	template <typename Opt>
	[[nodiscard]] auto file_opt_token_helper(Opt &&opt, fot_data &data, error_code &error) noexcept
	{
		if constexpr( is_any_string_v<Opt> or is_fstream_v<Opt,char> or is_ofstream_v<Opt,char> )
		{
			using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
			using type = token_t::type;
			return _file_opt_token_helper (
				http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
				data, error
			);
		}
		else if constexpr( Opt::optype == file_optype::single )
		{
			using type = std::remove_cvref_t<Opt>::type;
			return _file_opt_token_helper (
				http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt)),
				data, error
			);
		}
		else
			return _file_opt_token_helper(std::forward<Opt>(opt), data, error);
	}

	template <typename Opt>
	[[nodiscard]] auto _file_opt_token_helper(Opt &&opt, fot_data &data, error_code &error) noexcept
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

public:
	session_t m_session;
	generator_t m_generator;
};

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
basic_request(session_t &&session, url_t url, request_arg_t arg) :
	m_impl(std::make_shared<impl>(std::move(session), std::move(url), std::move(arg)))
{

}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
~basic_request() = default;

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
basic_request(basic_request &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
operator=(basic_request &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
write(Token &&token) noexcept
{
	return m_impl->write({}, token);
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
write(const const_buffer &body, Token &&token) noexcept requires put_or_post
{
	return m_impl->write(body, token);
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
template <typename T, core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
send_file(T &&opt, Token &&token) noexcept requires file_opt_token<T> and put_or_post
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->send_file(std::forward<T>(opt))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->send_file(std::forward<T>(opt));

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_send_file(ntoken.ec_, std::forward<T>(opt),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_send_file(std::forward<T>(opt),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, opt = std::forward<T>(opt), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_send_file (
						ntoken.ec_, std::move(opt), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
					opt = std::forward<T>(opt), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_send_file (
						std::move(opt), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_detached_v<nntoken_t> )
			m_impl->send_file_detach(std::forward<T>(opt));

		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken,
				opt = std::forward<T>(opt), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_send_file (
					ntoken.ec_, std::move(opt), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken,
				opt = std::forward<T>(opt), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_send_file (
					std::move(opt), cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace operators;
		using namespace std::chrono_literals;
		return send_file(std::forward<T>(opt), token | 0ns);
	}
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
chunk_end(const headers_t &headers, Token &&token) noexcept requires put_or_post
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->chunk_end(headers)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->chunk_end(headers);

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_chunk_end(ntoken.ec_, headers,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_chunk_end(headers,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, headers, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_chunk_end (
						ntoken.ec_, headers, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), headers, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_chunk_end (
						headers, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_detached_v<nntoken_t> )
			m_impl->chunk_end_detach(headers);

		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken,
				headers, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_chunk_end (
					ntoken.ec_, headers, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken,
				headers, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_chunk_end (
					headers, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace operators;
		using namespace std::chrono_literals;
		return chunk_end(headers, token | 0ns);
	}
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
chunk_end(Token &&token) noexcept requires put_or_post
{
	return chunk_end({}, token);
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
set_context(session_t &&session, url_t url)
{
	m_impl->m_session = std::move(session);
	m_impl->m_generator.reset();
	m_impl->m_generator.set_url(std::move(url));
	return *this;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
set_arg(request_arg_t arg)
{
	m_impl->m_generator.set_arg(std::move(arg));
	return *this;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
const basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::request_arg_t&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
arg() const noexcept
{
	return m_impl->m_generator.arg();
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::request_arg_t&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
arg() noexcept
{
	return m_impl->m_generator.arg();
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
const basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::url_t&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
url() const noexcept
{
	return m_impl->m_generator.url();
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
bool basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
is_finished() const noexcept
{
	return m_impl->m_generator.pro_state() == protocol::generator_state::finish;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
consteval protocol::method_enum
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
method() noexcept
{
	return method_v;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
consteval protocol::version_enum
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
version() noexcept
{
	return version_v;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
const basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::session_t&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
session() const noexcept
{
	return m_impl->m_session;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::session_t&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
session() noexcept
{
	return m_impl->m_session;
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::executor_t
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
get_executor() noexcept
{
	return session().get_executor();
}

template <protocol::method_enum Method, concepts::connection Session, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Session,Version>>::
cancel() noexcept
{
	session().opt_helper().cancel();
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H