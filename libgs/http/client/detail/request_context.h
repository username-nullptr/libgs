
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_CONTEXT_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_CONTEXT_H

namespace libgs::http
{

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
class LIBGS_HTTP_TAPI basic_request_context<Method,Exec,Version>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl(lease_ptr &&lease, url_t url, options opt) :
		m_lease(std::move(lease)),
		m_exec(lease_executor(m_lease)),
		m_generator(std::move(url), std::move(opt.arg), opt.target_form),
		m_reply(new reply_t(m_lease)),
		m_cookie_store(std::move(opt.cookie_store))
	{
		m_reply->parser().set_request_method(Method);
		m_reply->parser().set_automatic_decompression(opt.auto_decompression);
		m_reply->bind_cookie_jar(m_cookie_store, m_generator.url());
	}

private:
	[[nodiscard]] static executor_t lease_executor(const lease_ptr &lease)
	{
		if( not lease or not lease->is_valid() )
			invalid_argument::loc_throw("request context connection lease is invalid");
		return lease->get().get_executor();
	}

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token>
	[[nodiscard]] auto write(const const_buffer &body, Token &&token)
	{
		if constexpr( is_error_code_token_v<Token> )
			return detail::expected_value_or_error(_write(body), token);

		else if constexpr( is_sync_opt_token_v<Token> )
			return detail::expected_value_or_throw(_write(body));

		else if constexpr( is_detached_v<token_unbound_t<Token>> )
		{
			using namespace std::chrono_literals;
			auto data = std::make_shared<std::string>();

			if( body.size() > 0 )
				data->assign(static_cast<const char*>(body.data()), body.size());

			return detail::initiate_expected<size_t>(m_exec,
			[self = this->shared_from_this(), data = std::move(data)]() mutable -> awaitable<io_expected>
			{
				auto state = co_await asio::this_coro::cancellation_state;
				co_return co_await self->_co_write (
					{data->data(), data->size()}, state.slot(), 0ns
				);
			},
			std::forward<Token>(token));
		}
		else
		{
			using namespace std::chrono_literals;
			return detail::initiate_expected<size_t>(m_exec,
			[self = this->shared_from_this(), body]() mutable -> awaitable<io_expected>
			{
				auto state = co_await asio::this_coro::cancellation_state;
				co_return co_await self->_co_write (
					body, state.slot(), 0ns
				);
			},
			std::forward<Token>(token));
		}
	}

public:
	[[nodiscard]] io_expected upload_file
	(const body_norms_t &norms, auto &&opt, auto &&progress) noexcept
	{
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
			return io_unexpected(token.error());

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
			auto error = do_transfer(0, token->file_size);
			token->stream->close();
			if( error )
			{
				close_connection();
				return io_unexpected(error);
			}
		}
		else if( norms.index() == 1 )
		{
			auto &range_norms = std::get<range_body_norms>(norms);
			total = range_norms.total;

			auto error = do_transfer(range_norms.begin, range_norms.total);
			token->stream->close();
			if( error )
			{
				close_connection();
				return io_unexpected(error);
			}
		}
		else if( norms.index() == 2 )
		{
			const auto &[boundary, packages] = std::get<multipart_body_norms>(norms);
			for(auto &package : packages)
				total += package.range.total;

			for(const auto &[headers, range] : packages)
			{
				auto prefix = std::format("--{}\r\n", boundary);
				for(auto &header : headers)
					prefix += std::format("{}\r\n", header);
				prefix += "\r\n";

				if( auto expected = _write(prefix); not expected )
				{
					token->stream->close();
					return io_unexpected(expected.error());
				}
				else if( auto error = do_transfer(range.begin, range.total) )
				{
					token->stream->close();
					close_connection();
					return io_unexpected(error);
				}
			}
			token->stream->close();
			if( auto expected = _write(std::format("--{}--\r\n", boundary)); not expected )
				return io_unexpected(expected.error());
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
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_upload_file(body_norms_t norms,
		auto &&opt, auto &&progress, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
			co_return io_unexpected(token.error());

		auto task = libgs::dispatch(m_exec, [&]() mutable noexcept -> awaitable<io_expected>
		{
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
				auto error = co_await do_transfer(0, token->file_size);
				token->stream->close();
				if( error )
				{
					close_connection();
					co_return io_unexpected(error);
				}
			}
			else if( norms.index() == 1 )
			{
				auto &range_norms = std::get<range_body_norms>(norms);
				total = range_norms.total;

				auto error = co_await do_transfer(range_norms.begin, range_norms.total);
				token->stream->close();
				if( error )
				{
					close_connection();
					co_return io_unexpected(error);
				}
			}
			else if( norms.index() == 2 )
			{
				auto &[boundary, packages] = std::get<multipart_body_norms>(norms);
				for(auto &package : packages)
					total += package.range.total;

				for(auto &[headers, range] : packages)
				{
					auto prefix = std::format("--{}\r\n", boundary);
					for(auto &header : headers)
						prefix += std::format("{}\r\n", header);
					prefix += "\r\n";

					auto expected = co_await _co_write(prefix, cancel_slot, 0ns);
					if( not expected )
					{
						token->stream->close();
						co_return io_unexpected(expected.error());
					}
					auto error = co_await do_transfer (
						range.begin, range.total
					);
					if( error )
					{
						token->stream->close();
						close_connection();
						co_return io_unexpected(error);
					}
				}
				token->stream->close();
				auto expected = co_await _co_write (
					std::format("--{}--\r\n", boundary),
					cancel_slot, 0ns
				);
				if( not expected )
					co_return io_unexpected(expected.error());
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
			co_return sum;
		},
		use_awaitable);

		io_expected expected;
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
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
		std::error_code &error, body_norms_t norms, auto &&opt, auto &&progress,
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
		if( m_generator.pro_state() != generator_state::chunk )
		{
			return io_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return base_write(std::move(buf));
	}

	[[nodiscard]] awaitable<io_expected> co_chunk_end(const headers_t &headers,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_generator.pro_state() != generator_state::chunk )
		{
			co_return io_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		auto buf = m_generator.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;

		using namespace std::chrono_literals;
		auto task = co_base_write(std::move(buf), std::move(cancel_slot));
		io_expected expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
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
		libgs::dispatch(m_exec, co_chunk_end(headers), detached);
	}

private:
	[[nodiscard]] io_expected _write(const_buffer body) noexcept
	{
		auto pro_state = m_generator.pro_state();
		if( pro_state == generator_state::finish )
		{
			return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		size_t sum = 0;
		if( pro_state == generator_state::header )
		{
			auto header = m_generator.header_data(method_v, body.size());
			if( body.size() > 0 )
			{
				auto content = m_generator.body_data(body);
				return base_write(std::move(header), std::move(content));
			}
			return base_write(std::move(header));
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
		if( pro_state == generator_state::finish )
		{
			co_return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		auto task = libgs::dispatch(m_exec, [&]() mutable noexcept -> awaitable<io_expected>
		{
			size_t sum = 0;
			if( pro_state == generator_state::header )
			{
				auto header = m_generator.header_data(method_v, body.size());
				if( body.size() > 0 )
				{
					auto content = m_generator.body_data(body);
					co_return co_await co_base_write(std::move(header),
						std::move(content), std::move(cancel_slot)
					);
				}
				co_return co_await co_base_write(std::move(header),
					std::move(cancel_slot)
				);
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
				coro::sleep_for(m_exec, timeout)
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
		libgs::dispatch(m_exec, _co_write(body), detached);
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
		if( not has_connection() )
		{
			return io_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		auto expected = connection().write(const_buffer(data), use_sync);
		if( not expected )
		{
			close_connection();
			return io_unexpected(expected.error());
		}
		return *expected;
	}

	[[nodiscard]] io_expected base_write
	(std::string &&header, std::string &&body) noexcept
	{
		if( not has_connection() )
		{
			return io_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		const const_buffer buffers[] {
			const_buffer(header), const_buffer(body)
		};
		auto expected = connection().write(buffers, use_sync);
		if( not expected )
		{
			close_connection();
			return io_unexpected(expected.error());
		}
		return *expected;
	}

	[[nodiscard]] awaitable<io_expected>
	co_base_write(std::string data, asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		error_code error {};

		if( not has_connection() )
		{
			co_return io_unexpected(
				make_error_code(std::errc::not_connected)
			);
		}
		auto sum = co_await connection().write(const_buffer(data),
			use_awaitable | error | cancel_slot
		);
		if( error )
		{
			close_connection();
			co_return io_unexpected(error);
		}
		co_return sum;
	}

	[[nodiscard]] awaitable<io_expected>
	co_base_write(std::string header, std::string body,
		asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		error_code error {};

		if( not has_connection() )
		{
			co_return io_unexpected(
				make_error_code(std::errc::not_connected)
			);
		}
		const const_buffer buffers[] {
			const_buffer(header), const_buffer(body)
		};
		auto sum = co_await connection().write(buffers,
			use_awaitable | error | cancel_slot
		);
		if( error )
		{
			close_connection();
			co_return io_unexpected(error);
		}
		co_return sum;
	}

	[[nodiscard]] connection_t &connection() noexcept
	{
		return m_lease->get();
	}

	[[nodiscard]] bool has_connection() const noexcept
	{
		return m_lease and m_lease->is_valid();
	}

	void close_connection() noexcept
	{
		if( not has_connection() )
			return ;

		auto connection = m_lease->take();
		if( connection )
			ignore_unused(connection->close());
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
	lease_ptr m_lease {};
	executor_t m_exec {};

	generator_t m_generator;
	reply_ptr m_reply {};

	std::shared_ptr<cookie_jar> m_cookie_store {};
};

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::basic_request_context(lease_ptr &&lease, url_t url, options opt) :
	mutable_headers<basic_request_context>(nullptr),
	mutable_cookies<value,basic_request_context>(nullptr),
	mutable_chunk_attributes<basic_request_context>(nullptr),
	m_impl(std::make_shared<impl>(std::move(lease), std::move(url), std::move(opt)))
{
	this->m_headers = &m_impl->m_generator.headers();
	this->m_cookies = &m_impl->m_generator.cookies();
	this->m_chunk_attributes = &m_impl->m_generator.chunk_attributes();
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::~basic_request_context() = default;

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request_context<Method,Exec,Version>::write(Token &&token)
{
	return m_impl->write({}, std::forward<Token>(token));
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request_context<Method,Exec,Version>::write(const const_buffer &body, Token &&token)
	requires put_or_post
{
	return m_impl->write(body, std::forward<Token>(token));
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <typename T, typename Token>
auto basic_request_context<Method,Exec,Version>::upload_file(body_norms_t norms, T &&opt, Token &&token)
	requires file_task_token_v<T,Token>
{
	return upload_file(std::move(norms),
		std::forward<T>(opt), [](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <typename T, typename Progress, typename Token>
auto basic_request_context<Method,Exec,Version>::upload_file
(body_norms_t norms, T &&opt, Progress &&progress, Token &&token)
	requires file_task_token_v<T,Token> and concepts::progress_callback<Progress,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error(m_impl->upload_file (
			std::move(norms), std::forward<T>(opt),
			std::forward<Progress>(progress)
		), token);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw(m_impl->upload_file (
			std::move(norms), std::forward<T>(opt),
			std::forward<Progress>(progress)
		));
	}
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<size_t>(get_executor(), [
			impl = m_impl, norms = std::move(norms),
			opt = detail::capture_async_argument(std::forward<T>(opt)),
			progress = detail::capture_async_argument(std::forward<Progress>(progress))
		]()mutable -> awaitable<io_expected>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->co_upload_file (
				std::move(norms),
				detail::unwrap_async_argument(opt),
				detail::unwrap_async_argument(progress),
				state.slot(), 0ns
			);
		},
		std::forward<Token>(token));
	}
}
template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request_context<Method,Exec,Version>::chunk_end(const headers_t &headers, Token &&token)
	requires put_or_post
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error (
			m_impl->chunk_end(headers), token
		);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw (
			m_impl->chunk_end(headers)
		);
	}
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<size_t>(get_executor(),
		[impl = m_impl, headers]() mutable -> awaitable<io_expected>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->co_chunk_end (
				headers, state.slot(), 0ns
			);
		},
		std::forward<Token>(token));
	}
}
template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_request_context<Method,Exec,Version>::chunk_end(Token &&token)
	requires put_or_post
{
	return chunk_end({}, std::forward<Token>(token));
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
template <typename Token>
auto basic_request_context<Method,Exec,Version>::wait_reply(Token &&token)
	requires task_token_v<Token,status_enum>
{
	return reply()->wait(std::forward<Token>(token));
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::const_reply_ptr
basic_request_context<Method,Exec,Version>::reply() const noexcept
{
	return m_impl->m_reply;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::reply_ptr
basic_request_context<Method,Exec,Version>::reply() noexcept
{
	return m_impl->m_reply;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
bool basic_request_context<Method,Exec,Version>::responded() const noexcept
{
	return reply()->valid();
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>&
basic_request_context<Method,Exec,Version>::cancel() noexcept
{
	if( m_impl->m_lease and m_impl->m_lease->is_valid() )
	{
		ignore_unused(m_impl->m_lease->get().cancel());
		auto connection = m_impl->m_lease->take();
		if( connection )
			ignore_unused(connection->close());
	}
	return *this;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
const basic_request_context<Method,Exec,Version>::url_t&
basic_request_context<Method,Exec,Version>::url() const noexcept
{
	return m_impl->m_generator.url();
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::request_arg_t
basic_request_context<Method,Exec,Version>::arg() const noexcept
{
	return m_impl->m_generator.arg();
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::operator request_arg_t() const noexcept
{
	return arg();
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
consteval method_enum basic_request_context<Method,Exec,Version>::method() noexcept
{
	return method_v;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
consteval version_enum basic_request_context<Method,Exec,Version>::version() noexcept
{
	return version_v;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
const basic_request_context<Method,Exec,Version>::lease_t&
basic_request_context<Method,Exec,Version>::lease() const noexcept
{
	return *m_impl->m_lease;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::lease_t&
basic_request_context<Method,Exec,Version>::lease() noexcept
{
	return *m_impl->m_lease;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
const basic_request_context<Method,Exec,Version>::generator_t&
basic_request_context<Method,Exec,Version>::generator() const noexcept
{
	return m_impl->m_generator;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::generator_t&
basic_request_context<Method,Exec,Version>::generator() noexcept
{
	return m_impl->m_generator;
}

template <method_enum Method, core_concepts::exec Exec, version_enum Version>
basic_request_context<Method,Exec,Version>::executor_t
basic_request_context<Method,Exec,Version>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_CONTEXT_H
