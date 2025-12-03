
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

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
class LIBGS_HTTP_TAPI basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)

public:
	using sock_helper_t = socket_operation_helper<typename connection_t::socket_t>;

public:
	impl(connection_t &&connection, url_t url, request_arg_t arg) :
		m_connection(std::move(connection)), m_generator(std::move(url), std::move(arg)) {}

	impl(impl &&other) noexcept :
		m_connection(std::move(other.m_connection)),
		m_generator(std::move(other.m_generator)) {}

	impl& operator=(impl &&other) noexcept
	{
		m_connection = std::move(other.m_connection);
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
					libgs::dispatch(m_connection.get_executor(), [self = this->shared_from_this(),
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
					libgs::dispatch(m_connection.get_executor(), [self = this->shared_from_this(),
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
					libgs::dispatch(m_connection.get_executor(), [self = this->shared_from_this(), ntoken, nntoken,
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
					libgs::dispatch(m_connection.get_executor(), [self = this->shared_from_this(), nntoken,
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

public:
	[[nodiscard]] io_expected upload_file
	(const protocol::body_norms_t &norms, auto &&opt, auto &&progress) noexcept
	{
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
			return io_unexpected(token.error());

		auto before = m_connection.set_transfer_file_option();
		if( not before )
			return io_unexpected(before.error());

		char buffer[128 * 1024] {0};
		size_t sum = 0, total = 0;

		auto do_transfer = [&](size_t begin, size_t loc_total)
		{
			token->stream->seekg(begin, std::ios::beg);
			size_t loc_sum = 0;
			do {
				token->stream->read(buffer, sizeof(buffer));
				size_t gcount = token->stream->gcount();
				if( gcount == 0 )
					break;

				auto expected = _write({buffer, gcount});
				if( not expected )
					return expected.error();

				loc_sum += *expected;
				sum += *expected;

				if( auto error = invoke_progress(progress, sum, total) )
					return error;
			}
			while( not token->stream->eof() and loc_sum < loc_total );
			return error_code();
		};
		if( norms.index() == 0 or norms.index() == std::variant_npos )
		{
			total = token->file_size;
			if( auto error = do_transfer(0, token->file_size) )
				return io_unexpected(error);
		}
		else if( norms.index() == 1 )
		{
			auto &range_norms = std::get<protocol::range_body_norms>(norms);
			total = range_norms.total;
			if( auto error = do_transfer(range_norms.begin, range_norms.total) )
				return io_unexpected(error);
		}
		else if( norms.index() == 2 )
		{
			auto &multipart_norms = std::get<protocol::multipart_body_norms>(norms);
			for(auto &package : multipart_norms.packages)
				total += package.range.total;

			for(auto &package : multipart_norms.packages)
			{
				auto prefix = std::format("--{}\r\n", multipart_norms.boundary);
				for(auto &header : package.headers)
					prefix += std::format("{}\r\n", header);
				prefix += "\r\n";

				if( auto expected = _write(prefix); not expected )
					return io_unexpected(expected.error());

				if( auto error = do_transfer(package.range.begin, package.range.total) )
					return io_unexpected(error);
			}
			if( auto expected = _write(std::format("--{}--\r\n", multipart_norms.boundary)); not expected )
				return io_unexpected(expected.error());
		}
		else
		{
			logic_error::loc_throw (
				"There is a bug in the implementation of the library:"
				" theoretically, this conditional branch should never be true."
				" Please contact the author."
			);
		}
		auto expected = m_connection.unset_transfer_file_option(*before);
		if( not expected )
			return io_unexpected(expected.error());
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_upload_file(protocol::body_norms_t norms,
		auto &&opt, auto &&progress, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
			co_return io_unexpected(token.error());

		auto task = libgs::dispatch(m_connection.get_executor(),
		[&]() mutable -> awaitable<io_expected>
		{
			auto before = m_connection.set_transfer_file_option();
			if( not before )
				co_return io_unexpected(before.error());

			char buffer[128 * 1024] {0};
			size_t sum = 0, total = 0;

			auto do_transfer = [&](size_t begin, size_t loc_total) -> awaitable<error_code>
			{
				token->stream->seekg(begin, std::ios::beg);
				size_t loc_sum = 0;
				do {
					token->stream->read(buffer, sizeof(buffer));
					size_t gcount = token->stream->gcount();
					if( gcount == 0 )
						break;

					auto expected = co_await _co_write (
						{buffer, gcount}, cancel_slot, 0ns
					);
					if( not expected )
						co_return expected.error();

					loc_sum += *expected;
					sum += *expected;

					if( auto error = co_await co_invoke_progress(progress, sum, total) )
						co_return error;
				}
				while( not token->stream->eof() and loc_sum < loc_total );
				co_return error_code();
			};
			if( norms.index() == 0 )
			{
				total = token->file_size;
				if( auto error = co_await do_transfer(0, token->file_size) )
					co_return io_unexpected(error);
			}
			else if( norms.index() == 1 )
			{
				auto &range_norms = std::get<protocol::range_body_norms>(norms);
				total = range_norms.total;
				if( auto error = co_await do_transfer(range_norms.begin, range_norms.total) )
					co_return io_unexpected(error);
			}
			else if( norms.index() == 2 )
			{
				auto &multipart_norms = std::get<protocol::multipart_body_norms>(norms);
				for(auto &package : multipart_norms.packages)
					total += package.range.total;

				for(auto &package : multipart_norms.packages)
				{
					auto prefix = std::format("--{}\r\n", multipart_norms.boundary);
					for(auto &header : package.headers)
						prefix += std::format("{}\r\n", header);
					prefix += "\r\n";

					auto expected = co_await _co_write(prefix, cancel_slot, 0ns);
					if( not expected )
						co_return io_unexpected(expected.error());

					auto error = co_await do_transfer (
						package.range.begin, package.range.total
					);
					if( error )
						co_return io_unexpected(error);
				}
				auto expected = co_await _co_write (
					std::format("--{}--\r\n", multipart_norms.boundary),
					cancel_slot, 0ns
				);
				if( not expected )
					co_return io_unexpected(expected.error());
			}
			else
			{
				logic_error::loc_throw (
					"There is a bug in the implementation of the library:"
					" theoretically, this conditional branch should never be true."
					" Please contact the author."
				);
			}
			auto expected = m_connection.unset_transfer_file_option(*before);
			if( not expected )
				co_return io_unexpected(expected.error());
			co_return sum;
		},
		use_awaitable);

		io_expected expected;
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_connection.get_executor(), timeout)
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

	[[nodiscard]] awaitable<io_expected> co_upload_file(
		std::error_code &error, protocol::body_norms_t norms, auto &&opt, auto &&progress,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_upload_file(std::move(norms),
			std::forward<decltype(opt)>(opt), std::forward<decltype(progress)>(progress),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] io_expected chunk_end(const headers_t &headers) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::chunk )
		{
			return io_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return write_body(buffer(buf));
	}

	[[nodiscard]] awaitable<io_expected> co_chunk_end(const headers_t &headers,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() != protocol::generator_state::chunk )
		{
			co_return io_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
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
				coro::sleep_for(m_connection.get_executor(), timeout)
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
		libgs::dispatch(m_connection.get_executor(), co_chunk_end(headers), detached);
	}

private:
	[[nodiscard]] io_expected _write(const_buffer body) noexcept
	{
		auto pro_state = m_generator.pro_state();
		if( pro_state == protocol::generator_state::finish )
		{
			return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		size_t sum = 0;
		if( pro_state == protocol::generator_state::header )
		{
			auto expected = write_header(body.size());
			if( not expected )
				return expected;
			sum += *expected;
		}
		if( body.size() > 0 )
		{
			auto expected = write_body(body);
			if( not expected )
				return expected;
			sum += *expected;
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> _co_write(const_buffer body,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto pro_state = m_generator.pro_state();
		if( pro_state == protocol::generator_state::finish )
		{
			co_return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		auto task = libgs::dispatch(m_connection.get_executor(), [&]() mutable -> awaitable<io_expected>
		{
			size_t sum = 0;
			if( pro_state == protocol::generator_state::header )
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
				coro::sleep_for(m_connection.get_executor(), timeout)
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

	[[nodiscard]] awaitable<io_expected> _co_write(std::error_code &error, const_buffer body,
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
		libgs::dispatch(m_connection.get_executor(), _co_write(body), detached);
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
		auto &sock_helper = m_connection.opt_helper();
		error_code error;

		sock_helper.non_blocking(false, error);
		if( error )
		{
			sock_helper.close();
			return io_unexpected(error);
		}
		auto sum = sock_helper.write(data, error);
		if( error )
		{
			sock_helper.close();
			return io_unexpected(error);
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected>
	co_base_write(std::string &&data, asio::cancellation_slot cancel_slot) noexcept
	{
		auto &sock_helper = m_connection.opt_helper();
		error_code error;

		sock_helper.non_blocking(true, error);
		if( error )
		{
			sock_helper.close();
			co_return io_unexpected(error);
		}
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto sum = co_await sock_helper.write(data,
			use_awaitable | error | cancel_slot
		);
		if( error )
		{
			sock_helper.close();
			co_return io_unexpected(error);
		}
		co_return sum;
	}

private:
	[[nodiscard]] error_code invoke_progress(auto &progress, size_t sum, size_t total) noexcept
	{
		using pro_ret_t = decltype(progress(0, 0));
		if constexpr( std::is_same_v<pro_ret_t, bool> )
		{
			if( progress(sum, total) )
				return {};
			return make_error_code(errc::operation_aborted);
		}
		else
			progress(sum, total);
		return {};
	}

	[[nodiscard]] awaitable<error_code> co_invoke_progress(auto &progress, size_t sum, size_t total) noexcept
	{
		using pro_ret_t = decltype(progress(0,0));
		if constexpr( is_awaitable_v<pro_ret_t> )
		{
			using co_pro_ret_t = pro_ret_t::value_t;
			if constexpr( std::is_same_v<co_pro_ret_t,bool> )
			{
				if( co_await progress(sum, total) )
					co_return ;
				co_return make_error_code(errc::operation_aborted);
			}
			else
				co_await progress(sum, total);
		}
		else
			co_return invoke_progress(progress, sum, total);
	}

	template <typename Opt>
	auto make_file_opt_token(Opt &&opt) noexcept
	{
		using opt_t = std::remove_cvref_t<Opt>;
		if constexpr( is_any_string_v<opt_t> or is_fstream_v<opt_t,char> or is_ifstream_v<opt_t,char> )
		{
			using token_t = file_opt_token<void,file_optype::single> ;
			token_t token(std::forward<Opt>(opt));

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
	connection_t m_connection;
	generator_t m_generator;
};

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
basic_request(connection_t &&connection, url_t url, request_arg_t arg) :
	protocol::mutable_headers<basic_request>(nullptr),
	protocol::mutable_cookies<value, basic_request>(nullptr),
	protocol::mutable_chunk_attributes<basic_request>(nullptr),
	m_impl(std::make_shared<impl>(std::move(connection), std::move(url), std::move(arg)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
~basic_request() = default;

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
basic_request(basic_request &&other) noexcept :
	protocol::mutable_headers<basic_request>(nullptr),
	protocol::mutable_cookies<value, basic_request>(nullptr),
	protocol::mutable_chunk_attributes<basic_request>(nullptr),
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
operator=(basic_request &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
write(Token &&token) noexcept
{
	return m_impl->write({}, token);
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
write(const const_buffer &body, Token &&token) noexcept requires put_or_post
{
	return m_impl->write(body, token);
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <typename T, typename Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
upload_file(protocol::body_norms_t norms, T &&opt, Token &&token)
	noexcept requires file_opt_token_v<T>
{
	return upload_file(std::move(norms),
		std::forward<T>(opt), [](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <typename T, typename Progress, typename Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
upload_file(protocol::body_norms_t norms, T &&opt, Progress &&progress, Token &&token)
	noexcept requires file_opt_token_v<T> and concepts::progress_callback<Progress,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->upload_file(std::move(norms),
			std::forward<T>(opt), std::forward<Progress>(progress)
		)
		.or_else([&token](const error_code &error) {
			token = error;
		});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return m_impl->upload_file(std::move(norms),
			std::forward<T>(opt), std::forward<Progress>(progress)
		);
	}
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
				return m_impl->co_upload_file(ntoken.ec_, std::move(norms),
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_upload_file(std::move(norms),
					std::forward<T>(opt), std::forward<Progress>(progress),
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
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					ntoken, promise = std::move(promise), norms = std::move(norms),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_upload_file(ntoken.ec_,
						std::move(norms), std::move(opt), std::move(progress),
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
					promise = std::move(promise), norms = std::move(norms),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_upload_file (
						std::move(norms), std::move(opt), std::move(progress),
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, norms = std::move(norms), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file(ntoken.ec_,
					std::move(norms), std::move(opt), std::move(progress),
					cancel_slot, timeout
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, norms = std::move(norms), opt = std::forward<T>(opt),
				progress = std::forward<Progress>(progress), timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_upload_file (
					std::move(norms), std::move(opt), std::move(progress),
					cancel_slot, timeout
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
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return upload_file(std::move(norms),
			std::forward<T>(opt), std::forward<Progress>(progress),
			token | 0ns
		);
	}
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
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
				libgs::dispatch(get_executor(), [
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
				libgs::dispatch(get_executor(), [
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
			libgs::dispatch(get_executor(), [
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
			libgs::dispatch(get_executor(), [
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

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
chunk_end(Token &&token) noexcept requires put_or_post
{
	return chunk_end({}, token);
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
emplace(connection_t &&connection, url_t url)
{
	m_impl->m_connection = std::move(connection);
	m_impl->m_generator.reset();
	m_impl->m_generator.set_url(std::move(url));
	return *this;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
emplace(request_arg_t arg)
{
	m_impl->m_generator.set_arg(std::move(arg));
	return *this;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
const basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::url_t&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
url() const noexcept
{
	return m_impl->m_generator.url();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::request_arg_t
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
arg() const noexcept
{
	return m_impl->m_generator.arg();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
operator request_arg_t() const noexcept
{
	return arg();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
consteval protocol::method_enum
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
method() noexcept
{
	return method_v;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
consteval protocol::version_enum
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
version() noexcept
{
	return version_v;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
bool basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
is_finished() const noexcept
{
	return m_impl->m_generator.pro_state() == protocol::generator_state::finish;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
const basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::connection_t&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
connection() const noexcept
{
	return m_impl->m_connection;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::connection_t&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
connection() noexcept
{
	return m_impl->m_connection;
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::executor_t
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
get_executor() noexcept
{
	return connection().get_executor();
}

template <protocol::method_enum Method, concepts::connection Connection, protocol::version_enum Version>
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>&
basic_request<protocol::model::client,client_request_targ<Method,Connection,Version>>::
cancel() noexcept
{
	connection().opt_helper().cancel();
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H