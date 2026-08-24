
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H

#include <libgs/http_nt/protocol/utils/server/generator.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_response<Connection>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)
	using generator_t = server_generator;

public:
	explicit impl(connection_ptr connection) :
		m_connection(std::move(connection)) {}

public:
	[[nodiscard]] size_t write(const_buffer body, error_code &error) noexcept
	{
		error.clear();
		size_t sum = 0;

		auto pro_state = m_generator.pro_state();
		if( pro_state == generator_state::finish )
		{
			error = make_error_code(errc::eof);
			return sum;
		}
		else if( pro_state == generator_state::header )
		{
			auto bytes = write_header(body.size(), error);
			if( error )
				return sum;
			sum += bytes;
		}
		if( body.size() > 0 )
		{
			auto bytes = write_body(body, error);
			if( error )
				return sum;
			sum += bytes;
		}
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_write(const_buffer body, error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		error.clear();
		size_t sum = 0;

		auto pro_state = m_generator.pro_state();
		if( pro_state == generator_state::finish )
		{
			error = make_error_code(errc::eof);
			co_return sum;
		}
		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			if( pro_state == generator_state::header )
			{
				auto bytes = co_await co_write_header (
					body.size(), error, cancel_slot
				);
				if( error )
					co_return ;
				sum += bytes;
			}
			if( body.size() > 0 )
			{
				auto bytes = co_await co_write_body (
					body, error, std::move(cancel_slot)
				);
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

	[[nodiscard]] awaitable<size_t> co_write(const_buffer body,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		auto sum = co_await co_write(body,
			error, std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::write"
			);
		}
		co_return sum;
	}

public:
	[[nodiscard]] size_t send_file
	(const body_norms_t &norms, auto &&opt, error_code &error) noexcept
	{
		error.clear();
		size_t sum = 0;

		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
		{
			error = token.error();
			return sum;
		}
		auto before = m_connection->set_send_file_option();
		if( not before )
		{
			error = before.error();
			return sum;
		}
		char buffer[128 * 1024] {0};
		auto do_transfer = [&](size_t begin, size_t loc_total)
		{
			token->stream->seekg(begin, std::ios::beg);
			size_t loc_sum = 0;
			do {
				token->stream->read(buffer, sizeof(buffer));
				size_t gcount = token->stream->gcount();
				if( gcount == 0 )
					break;

				auto bytes = write({buffer, gcount}, error);
				if( error )
					return ;

				loc_sum += bytes;
				sum += bytes;
			}
			while( not token->stream->eof() and loc_sum < loc_total );
		};
		if( norms.index() == 0 or norms.index() == std::variant_npos )
		{
			do_transfer(0, token->file_size);
			token->stream->close();
			if( error )
				return sum;
		}
		else if( norms.index() == 1 )
		{
			auto &range_norms = std::get<range_body_norms>(norms);
			do_transfer(range_norms.begin, range_norms.total);
			token->stream->close();
			if( error )
				return sum;
		}
		else if( norms.index() == 2 )
		{
			auto &multipart_norms = std::get<multipart_body_norms>(norms);
			for(auto &package : multipart_norms.packages)
			{
				auto prefix = std::format("--{}\r\n", multipart_norms.boundary);
				for(auto &header : package.headers)
					prefix += std::format("{}\r\n", header);
				prefix += "\r\n";

				sum += write(prefix, error);
				if( error )
				{
					token->stream->close();
					return sum;
				}
				do_transfer(package.range.begin, package.range.total);
				if( error )
				{
					token->stream->close();
					return sum;
				}
			}
			token->stream->close();
			sum += write(std::format("--{}--\r\n", multipart_norms.boundary), error);
			if( error )
				return sum;
		}
		else
		{
			token->stream->close();
			logic_error::loc_throw (
				"There is a bug in the implementation of the library:"
				" theoretically, this conditional branch should never be true."
				" Please contact the author."
			);
		}
		auto expected = m_connection->unset_transfer_file_option(*before);
		if( not expected )
			error = expected.error();
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_send_file(body_norms_t norms, auto &&opt,
		error_code &error, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;
		error.clear();
		size_t sum = 0;

		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
		{
			error = token.error();
			co_return sum;
		}
		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			auto before = m_connection->set_send_file_option();
			if( not before )
			{
				error = before.error();
				co_return ;
			}
			char buffer[128 * 1024] {0};
			auto do_transfer = [&](size_t begin, size_t loc_total) -> awaitable<void>
			{
				token->stream->seekg(begin, std::ios::beg);
				size_t loc_sum = 0;
				do {
					token->stream->read(buffer, sizeof(buffer));
					size_t gcount = token->stream->gcount();
					if( gcount == 0 )
						break;

					auto bytes = co_await co_write (
						{buffer, gcount}, error, cancel_slot, 0ns
					);
					if( error )
						co_return ;

					loc_sum += bytes;
					sum += bytes;
				}
				while( not token->stream->eof() and loc_sum < loc_total );
			};
			if( norms.index() == 0 )
			{
				co_await do_transfer(0, token->file_size);
				token->stream->close();
				if( error )
					co_return io_unexpected(error);
			}
			else if( norms.index() == 1 )
			{
				auto &range_norms = std::get<range_body_norms>(norms);
				co_await do_transfer(range_norms.begin, range_norms.total);
				token->stream->close();
				if( error )
					co_return io_unexpected(error);
			}
			else if( norms.index() == 2 )
			{
				auto &multipart_norms = std::get<multipart_body_norms>(norms);
				for(auto &package : multipart_norms.packages)
				{
					auto prefix = std::format("--{}\r\n", multipart_norms.boundary);
					for(auto &header : package.headers)
						prefix += std::format("{}\r\n", header);
					prefix += "\r\n";

					sum += co_await co_write(prefix, error, cancel_slot, 0ns);
					if( error )
					{
						token->stream->close();
						co_return sum;
					}
					co_await do_transfer(package.range.begin, package.range.total);
					if( error )
					{
						token->stream->close();
						co_return sum;
					}
				}
				token->stream->close();
				sum += co_await co_write (
					std::format("--{}--\r\n", multipart_norms.boundary),
					error, cancel_slot, 0ns
				);
				if( error )
					co_return sum;
			}
			else
			{
				token->stream->close();
				logic_error::loc_throw (
					"There is a bug in the implementation of the library:"
					" theoretically, this conditional branch should never be true."
					" Please contact the author."
				);
			}
			auto expected = m_connection->unset_transfer_file_option(*before);
			if( not expected )
				error = expected.error();
			co_return sum;
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

	[[nodiscard]] awaitable<size_t> co_send_file
	(body_norms_t norms, auto &&opt, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		auto sum = co_await co_send_file (
			std::move(norms), std::forward<decltype(opt)>(opt),
			error, std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::send_file"
			);
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
		return write_body(buffer(buf), error);
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers, error_code &error) noexcept
	{
		if( m_generator.pro_state() != generator_state::chunk )
			co_return 0;
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;
		co_return co_await co_write_body(buffer(buf), error);
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers)
	{
		error_code error;
		auto sum = co_await co_chunk_end(headers, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::chunk_end"
			);
		}
		co_return sum;
	}

private:
	[[nodiscard]] size_t write_header(size_t size, error_code &error) noexcept {
		return base_write(m_generator.header_data(size), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_header
	(size_t size, error_code &error, asio::cancellation_slot cancel_slot) noexcept
	{
		co_return co_await co_base_write (
			m_generator.header_data(size), error, std::move(cancel_slot)
		);
	}

	[[nodiscard]] size_t write_body(const const_buffer &body, error_code &error) noexcept {
		return base_write(m_generator.body_data(body), error);
	}

	[[nodiscard]] awaitable<size_t> co_write_body
	(const const_buffer &body, error_code &error, asio::cancellation_slot cancel_slot) noexcept
	{
		co_return co_await co_base_write (
			m_generator.body_data(body), error, std::move(cancel_slot)
		);
	}

private:
	[[nodiscard]] size_t base_write(std::string &&data, error_code &error) noexcept
	{
		error.clear();
		auto &sock_helper = m_connection->opt_helper();

		size_t sum = 0;
		sock_helper.non_blocking(false, error);
		if( error )
		{
			sock_helper.close();
			return sum;
		}
		sum = sock_helper.write(data, error);
		if( error )
			sock_helper.close();
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_base_write
	(std::string &&data, error_code &error, asio::cancellation_slot cancel_slot) noexcept
	{
		error.clear();
		auto &sock_helper = m_connection->opt_helper();

		size_t sum = 0;
		sock_helper.non_blocking(true, error);
		if( error )
		{
			sock_helper.close();
			co_return sum;
		}
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		sum = co_await sock_helper.write(data,
			use_awaitable | error | cancel_slot
		);
		if( error )
			sock_helper.close();
		co_return sum;
	}

public:
	connection_ptr m_connection;
	generator_t m_generator {};
};

template <concepts::connection Connection>
basic_response<Connection>::basic_response(connection_ptr connection) :
	mutable_headers<basic_response>(nullptr),
	mutable_cookies<value_t,basic_response>(nullptr),
	mutable_chunk_attributes<basic_response>(nullptr),
	m_impl(new impl(std::move(connection)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <concepts::connection Connection>
basic_response<Connection>::~basic_response()
{
	delete m_impl;
}

template <concepts::connection Connection>
std::string_view basic_response<Connection>::version() const noexcept
{
	return m_impl->m_generator.version();
}

template <concepts::connection Connection>
basic_response<Connection> &basic_response<Connection>::set_status(status_enum status)
{
	return *this;
}

template <concepts::connection Connection>
basic_response<Connection> &basic_response<Connection>::auto_set(request_t &request)
{
	if( version() >= http_nt::version::v11 )
	{
		auto value = request.header(http_nt::header::transfer_encoding);
		if( value and strtls::to_lower(**value) == "chunked" )
			this->set_header(http_nt::header::transfer_encoding, "chunked");
	}
	return *this;
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::write(const const_buffer &body, Token &&token)
	requires task_token_v<Token,size_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->write(body, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = write(body, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::write"
			);
		}
		return sum;
	}
	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_write(body, no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_write(body,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_write(body, no_time_token.ec_,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
			else
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_write(body,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto buf_ptr = std::make_shared<std::string>(
				static_cast<const char*>(body.data()), body.size()
			);
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					no_time_token, buf = std::move(buf_ptr), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->m_impl->co_write (
						{buf->data(), buf->size()}, no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					buf = std::move(buf_ptr), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->m_impl->co_write (
						{buf->data(), buf->size()}, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else
		{
			auto buf_ptr = std::make_shared<std::string>(
				static_cast<const char*>(body.data()), body.size()
			);
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [
					self = this->shared_from_this(), no_time_token, original_token,
					buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->m_impl->co_write (
						buf, no_time_token.ec_, cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
						callback(error, 255);
					});
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					original_token, buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->m_impl->co_write (
						buf, cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
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

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::write(Token &&token)
	requires task_token_v<Token,size_t>
{
	return write({nullptr,0}, std::forward<Token>(token));
}

template <concepts::connection Connection>
template <typename T, typename Token>
auto basic_response<Connection>::send_file(T &&opt, Token &&token)
	requires file_opt_token<T,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->send_file(std::forward<T>(opt), token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = send_file(std::forward<T>(opt), error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::send_file"
			);
		}
		return sum;
	}
	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_send_file(std::forward<T>(opt), no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_send_file(std::forward<T>(opt),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_send_file(std::forward<T>(opt), no_time_token.ec_,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
			else
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_send_file(std::forward<T>(opt),
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					no_time_token, opt = std::forward<T>(opt), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->co_send_file (
						std::move(opt), no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					opt = std::forward<T>(opt), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->co_send_file (
						std::move(opt), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [
					self = this->shared_from_this(), no_time_token, original_token,
					opt = std::forward<T>(opt), timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->co_send_file (
						std::move(opt), no_time_token.ec_, cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
						callback(error, 255);
					});
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					original_token, opt = std::forward<T>(opt), timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->co_send_file (
						std::move(opt), cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
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
		return send_file(std::forward<T>(opt), token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::redirect
(core_concepts::text_p<char> auto &&url, redirect_enum redi, Token &&token)
	requires task_token_v<Token,size_t>
{
	m_impl->m_generator.set_redirect(std::forward<decltype(url)>(url), redi);
	return write({nullptr,0}, token);
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::redirect
(core_concepts::text_p<char> auto &&url, Token &&token)
	requires task_token_v<Token,size_t>
{
	return redirect (
		std::forward<decltype(url)>(url),
		redirect_enum::moved_permanently,
		std::forward<Token>(token)
	);
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::continues(Token &&token)
	requires task_token_v<Token,size_t>
{
	this->set_status(http_nt::status::continue_upload);
	return write({nullptr,0});
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::chunk_end(const headers_t &headers, Token &&token)
	requires task_token_v<Token,size_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->chunk_end(headers, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = chunk_end(headers, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_response::chunk_end"
			);
		}
		return sum;
	}
	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_chunk_end(headers, no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_chunk_end(headers,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_chunk_end(headers, no_time_token.ec_,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
			else
			{
				return libgs::dispatch(m_impl->m_connection->get_executor(),
					m_impl->co_chunk_end(headers,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					),
					deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					no_time_token, headers, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->m_impl->co_chunk_end (
						headers, no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					headers, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await self->m_impl->co_chunk_end (
						headers, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [
					self = this->shared_from_this(), no_time_token, original_token,
					headers, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->m_impl->co_chunk_end (
						headers, no_time_token.ec_, cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
						callback(error, 255);
					});
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection->get_executor(), [self = this->shared_from_this(),
					original_token, headers, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
				]() mutable noexcept -> awaitable<void>
				{
					auto expected = co_await self->m_impl->co_chunk_end (
						headers, cancel_slot, timeout
					);
					expected
					.transform([&callback = original_token](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = original_token](const error_code &error) {
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
		return chunk_end(headers, token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename Token>
auto basic_response<Connection>::chunk_end(Token &&token)
	requires task_token_v<Token,size_t>
{
	return chunk_end({}, std::forward<Token>(token));
}

template <concepts::connection Connection>
status_enum basic_response<Connection>::status() const noexcept
{
	return m_impl->m_generator.status();
}

template <concepts::connection Connection>
bool basic_response<Connection>::is_finished() const noexcept
{
	return m_impl->m_generator.pro_state() == generator_state::finish;
}

template <concepts::connection Connection>
basic_response<Connection>::executor_t basic_response<Connection>::get_executor() noexcept
{
	return m_impl->m_connection->get_executor();
}

template <concepts::connection Connection>
basic_response<Connection> &basic_response<Connection>::cancel() noexcept
{
	return m_impl->m_connection->cancel();
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_RESPONSE_H
