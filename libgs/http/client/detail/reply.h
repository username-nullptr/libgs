
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REPLY_H
#define LIBGS_HTTP_CLIENT_DETAIL_REPLY_H

namespace libgs::http
{

template <core_concepts::exec Exec>
class LIBGS_HTTP_TAPI basic_reply<Exec>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(lease_ptr lease) :
		m_lease(std::move(lease)),
		m_exec(lease_executor(m_lease)) {}

	impl(lease_ptr lease, parser_t &&parser) :
		m_lease(std::move(lease)),
		m_exec(lease_executor(m_lease)),
		m_parser(std::move(parser)) {}

private:
	[[nodiscard]] static executor_t lease_executor(const lease_ptr &lease)
	{
		if( not lease or not lease->is_valid() )
			invalid_argument::loc_throw("reply connection lease is invalid");
		return lease->get().get_executor();
	}

public:
	void capture_cookies()
	{
		if( m_cookies_captured or not m_cookie_jar or
			m_parser.status() == status::none or
			m_parser.stage() == parser_t::stage_t::header or
			m_parser.is_informational() )
			return ;

		m_cookie_jar->store(m_origin, m_parser.set_cookies());
		m_cookies_captured = true;
	}

	[[nodiscard]] sys_expected<status_enum> wait() noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		else if( m_parser.stage() != parser_t::stage_t::header )
		{
			if( m_parser.is_informational() )
			{
				auto expected = m_parser.next_message();
				if( not expected )
				{
					close_connection();
					return sys_unexpected(expected.error());
				}
				if( *expected )
				{
					capture_cookies();
					finish_connection();
					return m_parser.status();
				}
			}
			else
			{
				capture_cookies();
				finish_connection();
				return m_parser.status();
			}
		}
		if( not has_connection() )
		{
			return sys_unexpected(
				make_error_code(std::errc::not_connected)
			);
		}
		auto &conn = connection();
		if( not conn.is_open() )
		{
			close_connection();
			return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		using namespace libgs::operators;
		std::error_code error;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];
		for(;;)
		{
			auto sum = conn.read(buffer(buf, buf_size), error);
			if( error )
			{
				close_connection();
				return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, sum});
			if( not expected )
			{
				close_connection();
				return sys_unexpected(expected.error());
			}
			else if( *expected )
				break;
		}
		capture_cookies();
		finish_connection();
		return m_parser.status();
	}

	[[nodiscard]] awaitable<sys_expected<status_enum>>
	co_wait(std::chrono::nanoseconds timeout) noexcept
	{
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		else if( m_parser.stage() != parser_t::stage_t::header )
		{
			if( m_parser.is_informational() )
			{
				auto expected = m_parser.next_message();
				if( not expected )
				{
					close_connection();
					co_return sys_unexpected(expected.error());
				}
				if( *expected )
				{
					capture_cookies();
					finish_connection();
					co_return m_parser.status();
				}
			}
			else
			{
				capture_cookies();
				finish_connection();
				co_return m_parser.status();
			}
		}
		if( not has_connection() )
		{
			co_return sys_unexpected(
				make_error_code(std::errc::not_connected)
			);
		}
		auto &conn = connection();
		if( not conn.is_open() )
		{
			close_connection();
			co_return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		using namespace libgs::operators;
		std::error_code error;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];

		auto task = libgs::dispatch(m_exec,
		[&]() mutable noexcept -> awaitable<sys_expected<status_enum>>
		{
			for(;;)
			{
				auto sum = co_await conn.read (
					buffer(buf, buf_size), use_awaitable | error
				);
				if( error )
				{
					close_connection();
					co_return sys_unexpected(error);
				}
				auto expected = m_parser.append({buf, sum});
				if( not expected )
				{
					close_connection();
					co_return sys_unexpected(expected.error());
				}
				else if( *expected )
					break;
			}
			co_return m_parser.status();
		},
		use_awaitable);

		using namespace std::chrono_literals;
		sys_expected<status_enum> expected;

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
		if( expected )
		{
			capture_cookies();
			finish_connection();
		}
		else
			close_connection();

		co_return expected;
	}

public:
	[[nodiscard]] io_expected read(const mutable_buffer &buf) noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			finish_connection();
			return sys_unexpected (
				make_error_code(errc::eof)
			);
		}
		if( m_parser.stage() == parser_t::stage_t::header )
		{
			if( auto expected = wait(); not expected )
				return io_unexpected(expected.error());
		}
		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			finish_connection();
			return io_unexpected(make_error_code(errc::eof));
		}
		if( not has_connection() )
		{
			return io_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		auto &conn = connection();
		if( not conn.is_open() )
		{
			close_connection();
			return io_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		size_t sum = 0;
		if( buf.size() == 0 )
			return sum;

		auto state = conn.options();
		if( not state )
		{
			close_connection();
			return io_unexpected(state.error());
		}
		auto read_size = state->receive_buffer_size;
		if( read_size == 0 )
			read_size = 0xFFFF;

		auto dst_buf = static_cast<char*>(buf.data());
		for(;;)
		{
			sum += m_parser.read_partial_body (
				{dst_buf + sum, buf.size() - sum}
			);
			if( sum == buf.size() or m_parser.stage() == parser_t::stage_t::finished )
				break;

			if( auto direct_size = m_parser.prepare_direct_body_read(buf.size() - sum) )
			{
				error_code error {};
				auto bytes = conn.read({dst_buf + sum, direct_size}, error);
				if( error )
				{
					close_connection();
					return sum > 0 ? io_expected(sum) : io_unexpected(error);
				}
				if( not m_parser.commit_direct_body_read(bytes) )
				{
					close_connection();
					return io_unexpected(make_error_code(std::errc::protocol_error));
				}
				sum += bytes;
				continue;
			}

			std::string body(read_size,'\0');
			for(;;)
			{
				error_code error {};
				auto tmp_sum = conn.read({body.data(), body.size()}, error);
				if( error )
				{
					close_connection();
					if( error == errc::eof and m_parser.finish_eof() )
					{
						error.clear();
						break;
					}
					return sum > 0 ? io_expected(sum) : io_unexpected(error);
				}
				auto expected = m_parser.append({body.data(), tmp_sum});
				if( not expected )
				{
					close_connection();
					return io_unexpected(expected.error());
				}
				else if( *expected )
					break;
			}
		}
		if( sum == 0 )
		{
			finish_connection();
			return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		finish_connection();
		return sum;
	}

	[[nodiscard]] awaitable<io_expected>
	co_read(const mutable_buffer &buf, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			finish_connection();
			co_return sys_unexpected (
				make_error_code(errc::eof)
			);
		}
		if( m_parser.stage() == parser_t::stage_t::header )
		{
			using namespace std::chrono_literals;
			auto expected = co_await co_wait(0ns);
			if( not expected )
				co_return io_unexpected(expected.error());
		}
		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			finish_connection();
			co_return io_unexpected(make_error_code(errc::eof));
		}
		if( not has_connection() )
		{
			co_return io_unexpected(
				make_error_code(std::errc::not_connected)
			);
		}
		auto &conn = connection();
		if( not conn.is_open() )
		{
			close_connection();
			co_return io_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		size_t sum = 0;
		if( buf.size() == 0 )
			co_return sum;

		auto state = conn.options();
		if( not state )
		{
			close_connection();
			co_return io_unexpected(state.error());
		}
		auto read_size = state->receive_buffer_size;
		if( read_size == 0 )
			read_size = 0xFFFF;

		using namespace libgs::operators;
		auto dst_buf = static_cast<char*>(buf.data());

		auto task = libgs::dispatch(m_exec, [&]() mutable noexcept -> awaitable<io_expected>
		{
			for(;;)
			{
				sum += m_parser.read_partial_body (
					{dst_buf + sum, buf.size() - sum}
				);
				if( sum == buf.size() or m_parser.stage() == parser_t::stage_t::finished )
					break;

				if( auto direct_size = m_parser.prepare_direct_body_read(buf.size() - sum) )
				{
					error_code error {};
					auto bytes = co_await conn.read({dst_buf + sum, direct_size},
						use_awaitable | error
					);
					if( error )
					{
						close_connection();
						co_return io_unexpected(error);
					}
					if( not m_parser.commit_direct_body_read(bytes) )
					{
						close_connection();
						co_return io_unexpected(
							make_error_code(std::errc::protocol_error)
						);
					}
					sum += bytes;
					continue;
				}

				std::string body(read_size,'\0');
				for(;;)
				{
					error_code error {};
					auto tmp_sum = co_await conn.read({body.data(), body.size()},
						use_awaitable | error
					);
					if( error )
					{
						close_connection();
						if( error == errc::eof and m_parser.finish_eof() )
						{
							error.clear();
							break;
						}
						co_return io_unexpected(error);
					}
					auto expected = m_parser.append({body.data(), tmp_sum});
					if( not expected )
					{
						close_connection();
						co_return io_unexpected(expected.error());
					}
					else if( *expected )
						break;
				}
			}
			if( sum == 0 )
			{
				finish_connection();
				co_return io_unexpected (
					make_error_code(errc::eof)
				);
			}
			finish_connection();
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
		if( not expected and expected.error() != errc::eof )
			close_connection();
		co_return expected;
	}

public:
	[[nodiscard]] sys_expected<byte_range_chunk> read_range_body() noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		if( m_parser.stage() == stage::header )
		{
			if( auto expected = wait(); not expected )
				return sys_unexpected(expected.error());
		}
		for(;;)
		{
			if( auto chunk = m_parser.take_range_body(128 * 1024) )
			{
				finish_connection();
				return std::move(*chunk);
			}
			if( m_parser.stage() == stage::finished )
			{
				finish_connection();
				return sys_unexpected(make_error_code(errc::eof));
			}
			if( not has_connection() )
				return sys_unexpected(make_error_code(std::errc::not_connected));

			auto &conn = connection();
			if( not conn.is_open() )
			{
				close_connection();
				return sys_unexpected(make_error_code(std::errc::not_connected));
			}
			char buf[128 * 1024] {};
			error_code error {};

			auto size = conn.read(buffer(buf), error);
			if( error )
			{
				close_connection();
				return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, size});
			if( not expected )
			{
				close_connection();
				return sys_unexpected(expected.error());
			}
		}
		return {};
	}

	[[nodiscard]] awaitable<sys_expected<byte_range_chunk>> co_read_range_body() noexcept
	{
		using namespace libgs::operators;
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		if( m_parser.stage() == stage::header )
		{
			using namespace std::chrono_literals;
			auto expected = co_await co_wait(0ns);
			if( not expected )
				co_return sys_unexpected(expected.error());
		}
		for(;;)
		{
			if( auto chunk = m_parser.take_range_body(128 * 1024) )
			{
				finish_connection();
				co_return std::move(*chunk);
			}
			if( m_parser.stage() == stage::finished )
			{
				finish_connection();
				co_return sys_unexpected(make_error_code(errc::eof));
			}
			if( not has_connection() )
				co_return sys_unexpected(make_error_code(std::errc::not_connected));

			auto &conn = connection();
			if( not conn.is_open() )
			{
				close_connection();
				co_return sys_unexpected(make_error_code(std::errc::not_connected));
			}
			char buf[128 * 1024] {};
			error_code error {};

			auto size = co_await conn.read (
				buffer(buf), use_awaitable | error
			);
			if( error )
			{
				close_connection();
				co_return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, size});
			if( not expected )
			{
				close_connection();
				co_return sys_unexpected(expected.error());
			}
		}
		co_return sys_expected<byte_range_chunk>();
	}

public:
	[[nodiscard]] sys_expected<std::vector<std::byte>> read_all() noexcept
	{
		std::vector<std::byte> sum {};
		for(;;)
		{
			constexpr size_t default_buf_size = 64 * 1024;
			auto offset = sum.size();
			auto buf_size = grow_read_all_buffer(sum, default_buf_size);
			if( not buf_size )
				return sys_unexpected(buf_size.error());

			auto expected = read({sum.data() + offset, *buf_size});
			if( expected )
			{
				sum.resize(offset + *expected);
				if( m_parser.stage() == stage::finished )
					break;
				continue;
			}
			sum.resize(offset);
			if( expected.error() == errc::eof )
				break;
			return sys_unexpected(expected.error());
		}
		return std::move(sum);
	}

	[[nodiscard]] awaitable<sys_expected<std::vector<std::byte>>>
	co_read_all(std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		auto task = libgs::dispatch(m_exec,
		[&]() mutable noexcept -> awaitable<sys_expected<std::vector<std::byte>>>
		{
			std::vector<std::byte> sum {};
			for(;;)
			{
				constexpr size_t default_buf_size = 64 * 1024;
				auto offset = sum.size();
				auto buf_size = grow_read_all_buffer(sum, default_buf_size);
				if( not buf_size )
					co_return sys_unexpected(buf_size.error());

				auto expected = co_await co_read (
					{sum.data() + offset, *buf_size}, 0ns
				);
				if( expected )
				{
					sum.resize(offset + *expected);
					if( m_parser.stage() == stage::finished )
						break;
					continue;
				}
				sum.resize(offset);
				if( expected.error() == errc::eof )
					break;
				co_return sys_unexpected(expected.error());
			}
			co_return std::move(sum);
		},
		use_awaitable);

		sys_expected<std::vector<std::byte>> expected;
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
		if( not expected )
			close_connection();
		co_return expected;
	}

public:
	[[nodiscard]] io_expected save_file(auto &&opt, auto &&progress) noexcept
	{
		io_expected expected {};
		if( m_parser.stage() == stage::header )
		{
			if( auto status = wait(); not status )
				return expected.despair(status.error());
		}
		if( m_parser.status() == status::range_not_satisfiable )
		{
			close_connection();
			return expected.despair (
				std::make_error_code(std::errc::result_out_of_range)
			);
		}
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt),
			m_parser.status() == status::partial_content
		);
		if( not token )
		{
			close_connection();
			return expected.despair(token.error());
		}
		size_t sum = 0, total = 0;
		if( auto complete = m_parser.complete_length() )
			total = *complete;

		else if( auto length = m_parser.header(header::content_length);
				 not m_parser.is_range_response() and length )
			total = *length->get<size_t>().or_else(0);

		for(;;)
		{
			auto chunk = read_range_body();
			if( not chunk )
			{
				expected.despair(chunk.error());
				break;
			}
			token->stream->seekp(chunk->offset, std::ios::beg);
			token->stream->write(chunk->data.data(), chunk->data.size());

			if( not *token->stream )
			{
				expected.despair(std::make_error_code(std::errc::io_error));
				break;
			}
			sum += chunk->data.size();
			if( auto complete = m_parser.complete_length() )
				total = *complete;

			if( auto error = invoke_progress(progress, sum, total) )
			{
				expected.despair(error);
				break;
			}
		}
		token->stream->close();
		if( not expected and expected.error() != errc::eof )
		{
			close_connection();
			return expected;
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_save_file
	(auto &&opt, auto &&progress, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		io_expected expected {};
		if( m_parser.stage() == stage::header )
		{
			auto status = co_await co_wait(0ns);
			if( not status )
				co_return expected.despair(status.error());
		}
		if( m_parser.status() == status::range_not_satisfiable )
		{
			close_connection();
			co_return expected.despair (
				std::make_error_code(std::errc::result_out_of_range)
			);
		}
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt),
			m_parser.status() == status::partial_content
		);
		if( not token )
		{
			close_connection();
			co_return expected.despair(token.error());
		}
		auto task = libgs::dispatch(m_exec,
		[&]() mutable noexcept -> awaitable<io_expected>
		{
			size_t sum = 0, total = 0;
			if( auto complete = m_parser.complete_length() )
				total = *complete;

			else if( auto length = m_parser.header(header::content_length);
					 not m_parser.is_range_response() and length )
				total = *length->get<size_t>().or_else(0);

			for(;;)
			{
				auto chunk = co_await co_read_range_body();
				if( not chunk )
				{
					expected.despair(chunk.error());
					break;
				}
				token->stream->seekp(chunk->offset, std::ios::beg);
				token->stream->write(chunk->data.data(), chunk->data.size());

				if( not *token->stream )
				{
					expected.despair(std::make_error_code(std::errc::io_error));
					break;
				}
				sum += chunk->data.size();
				if( auto complete = m_parser.complete_length() )
					total = *complete;

				if( auto error = co_await co_invoke_progress(progress, sum, total) )
				{
					expected.despair(error);
					break;
				}
			}
			token->stream->close();

			if( not expected and expected.error() != errc::eof )
				co_return expected;
			co_return sum;
		},
		use_awaitable);

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
		if( not expected )
			close_connection();
		co_return expected;
	}

private:
	[[nodiscard]] sys_expected<size_t>
	grow_read_all_buffer(std::vector<std::byte> &sum, size_t default_size) const noexcept
	{
		auto offset = sum.size();
		size_t direct_remaining = 0;

		auto read_size = m_parser.partial_body_size();
		if( read_size == 0 )
		{
			direct_remaining = m_parser.prepare_direct_body_read(
				std::numeric_limits<size_t>::max()
			);
			read_size = direct_remaining == 0 ? default_size :
				std::min(default_size, direct_remaining);
		}
		if( read_size > sum.max_size() - offset or
			(direct_remaining != 0 and direct_remaining > sum.max_size() - offset) )
		{
			return sys_unexpected(
				make_error_code(std::errc::value_too_large)
			);
		}
		try
		{
			if( constexpr size_t max_preallocated_body_size = 8 * 1024 * 1024;
				direct_remaining != 0 and direct_remaining <= max_preallocated_body_size and
				sum.capacity() < offset + direct_remaining )
				sum.reserve(offset + direct_remaining);
			sum.resize(offset + read_size);
		}
		catch(const std::length_error&)
		{
			return sys_unexpected (
				make_error_code(std::errc::value_too_large)
			);
		}
		catch(const std::bad_alloc&)
		{
			return sys_unexpected (
				make_error_code(std::errc::not_enough_memory)
			);
		}
		return read_size;
	}

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
	auto make_file_opt_token(Opt &&opt, bool preserve) noexcept
	{
		using opt_t = std::remove_cvref_t<Opt>;
		if constexpr( is_any_string_v<opt_t> or std::same_as<opt_t,std::filesystem::path> or
			is_fstream_v<opt_t,char> or is_ofstream_v<opt_t,char> )
		{
			auto token = http::make_file_opt_token(std::forward<Opt>(opt));
			using token_t = decltype(token);

			auto mode = std::ios::out | std::ios::binary | std::ios::trunc;
			if constexpr( requires { token.file_name; } )
			{
				if( preserve and std::filesystem::exists(token.file_name) )
					mode = std::ios::in | std::ios::out | std::ios::binary;
			}
			auto expected = token.init(mode);
			if( expected )
				return sys_expected<token_t>(std::move(token));

			return sys_expected<token_t>(sys_unexpected(expected.error()));
		}
		else
		{
			if( opt.stream->is_open() )
				return sys_expected<opt_t>(std::forward<Opt>(opt));

			auto mode = preserve ?
				std::ios::in | std::ios::out | std::ios::binary :
				std::ios::out | std::ios::binary | std::ios::trunc;

			auto expected = opt.init(mode);
			if( expected )
				return sys_expected<opt_t>(std::forward<Opt>(opt));

			return sys_expected<opt_t>(sys_unexpected(expected.error()));
		}
	}

	[[nodiscard]] bool has_connection() const noexcept
	{
		return m_lease and m_lease->is_valid();
	}

	void close_connection() noexcept
	{
		if( not has_connection() )
			return ;
		if( auto connection = m_lease->take() )
			ignore_unused(connection->close());
	}

	void finish_connection() noexcept
	{
		if( not has_connection() or
			m_parser.stage() != parser_t::stage_t::finished or
			m_parser.is_informational() or m_parser.is_upgrade() )
			return ;

		auto pending = m_parser.take_pending_data();
		if( not m_parser.keep_alive() or not pending.empty() )
		{
			close_connection();
			return ;
		}
		try {
			m_lease->release();
		}
		catch(...) {
			close_connection();
		}
	}

	[[nodiscard]] connection_t &connection() noexcept
	{
		return m_lease->get();
	}

public:
	lease_ptr m_lease {};
	executor_t m_exec {};

	error_code m_first_error {};
	parser_t m_parser {};

	std::shared_ptr<cookie_jar> m_cookie_jar {};
	url m_origin {};

	bool m_cookies_captured = false;
};

template <core_concepts::exec Exec>
basic_reply<Exec>::basic_reply(lease_ptr lease) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::make_shared<impl>(std::move(lease)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
}

template <core_concepts::exec Exec>
basic_reply<Exec>::basic_reply(lease_ptr lease, parser_t &&parser) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::make_shared<impl>(std::move(lease), std::move(parser)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
}

template <core_concepts::exec Exec>
basic_reply<Exec>::~basic_reply() = default;

template <core_concepts::exec Exec>
template <typename Token>
auto basic_reply<Exec>::wait(Token &&token)
	requires task_token_v<Token,status_enum>
{
	if constexpr( is_error_code_token_v<Token> )
		return detail::expected_value_or_error(m_impl->wait(), token);

	else if constexpr( is_sync_opt_token_v<Token> )
		return detail::expected_value_or_throw(m_impl->wait());
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<status_enum>(get_executor(),
		[impl = m_impl]() mutable -> awaitable<sys_expected<status_enum>> {
			co_return co_await impl->co_wait(0ns);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_reply<Exec>::read(const mutable_buffer &buf, Token &&token)
	requires task_token_v<Token,size_t>
{
	if constexpr( is_error_code_token_v<Token> )
		return detail::expected_value_or_error(m_impl->read(buf), token);

	else if constexpr( is_sync_opt_token_v<Token> )
		return detail::expected_value_or_throw(m_impl->read(buf));
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<size_t>(get_executor(),
		[impl = m_impl, buf]() mutable -> awaitable<io_expected> {
			co_return co_await impl->co_read(buf, 0ns);
		},
		std::forward<Token>(token));
	}
}
template <core_concepts::exec Exec>
template <concepts::buffer Buffer, typename Token>
auto basic_reply<Exec>::read(Token &&token)
	requires task_token_v<Token,Buffer>
{
	if constexpr( is_array_buffer_v<Buffer> )
	{
		if constexpr( is_error_code_token_v<Token> )
		{
			Buffer result {};
			ignore_unused(detail::expected_value_or_error (
				m_impl->read(buffer(result)), token
			));
			return result;
		}
		else if constexpr( is_sync_opt_token_v<Token> )
		{
			Buffer result {};
			ignore_unused(detail::expected_value_or_throw (
				m_impl->read(buffer(result))
			));
			return result;
		}
		else
		{
			using namespace std::chrono_literals;
			return detail::initiate_expected<Buffer>(get_executor(),
			[impl = m_impl]() mutable -> awaitable<sys_expected<Buffer>>
			{
				Buffer result {};
				auto expected = co_await impl->co_read (
					buffer(result), 0ns
				);
				if( not expected )
					co_return sys_unexpected(expected.error());
				co_return result;
			},
			std::forward<Token>(token));
		}
	}
	else if constexpr( is_error_code_token_v<Token> )
	{
		auto source = detail::expected_value_or_error(m_impl->read_all(), token);
		return detail::copy_buffer_data<Buffer>(std::move(source));
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		auto source = detail::expected_value_or_throw(m_impl->read_all());
		return detail::copy_buffer_data<Buffer>(std::move(source));
	}
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<Buffer>(get_executor(),
		[impl = m_impl]() mutable -> awaitable<sys_expected<Buffer>>
		{
			auto expected = co_await impl->co_read_all(0ns);
			if( not expected )
				co_return sys_unexpected(expected.error());
			co_return detail::copy_buffer_data<Buffer>(std::move(*expected));
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_reply<Exec>::read(Token &&token)
	requires task_token_v<Token,std::vector<std::byte>>
{
	return read<std::vector<std::byte>>(std::forward<Token>(token));
}
template <core_concepts::exec Exec>
template <typename T, typename Token>
auto basic_reply<Exec>::save_file(T &&opt, Token &&token)
	requires file_task_token<T,Token>
{
	return save_file(std::forward<T>(opt),
		[](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec>
template <typename T, typename Progress, typename Token>
auto basic_reply<Exec>::save_file(T &&opt, Progress &&progress, Token &&token)
	requires file_task_token<T,Token> and concepts::progress_callback<Progress,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error(m_impl->save_file(
			std::forward<T>(opt), std::forward<Progress>(progress)
		), token);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw(m_impl->save_file (
			std::forward<T>(opt), std::forward<Progress>(progress)
		));
	}
	else
	{
		using namespace std::chrono_literals;
		return detail::initiate_expected<size_t>(get_executor(), [
			impl = m_impl, opt = detail::capture_async_argument(std::forward<T>(opt)),
			progress = detail::capture_async_argument(std::forward<Progress>(progress))
		]() mutable -> awaitable<io_expected>
		{
			co_return co_await impl->co_save_file (
				detail::unwrap_async_argument(opt),
				detail::unwrap_async_argument(progress),
				0ns
			);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec>
version_enum basic_reply<Exec>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <core_concepts::exec Exec>
status_enum basic_reply<Exec>::status() const noexcept
{
	return m_impl->m_parser.status();
}

template <core_concepts::exec Exec>
bool basic_reply<Exec>::valid() const noexcept
{
	return m_impl->m_parser.stage() != stage::header;
}

template <core_concepts::exec Exec>
error_code basic_reply<Exec>::first_error() const noexcept
{
	return m_impl->m_first_error;
}

template <core_concepts::exec Exec>
bool basic_reply<Exec>::content_decoded() const noexcept
{
	return m_impl->m_parser.content_decoded();
}

template <core_concepts::exec Exec>
bool basic_reply<Exec>::is_chunked() const noexcept
{
	return m_impl->m_parser.is_chunked();
}

template <core_concepts::exec Exec>
bool basic_reply<Exec>::is_eof() const noexcept
{
	return m_impl->m_parser.stage() == stage::finished;
}

template <core_concepts::exec Exec>
bool basic_reply<Exec>::is_upgrade() const noexcept
{
	return m_impl->m_parser.is_upgrade();
}

template <core_concepts::exec Exec>
std::string basic_reply<Exec>::take_pending_data()
{
	return m_impl->m_parser.take_pending_data();
}

template <core_concepts::exec Exec>
const basic_reply<Exec>::lease_t &basic_reply<Exec>::lease() const noexcept
{
	return *m_impl->m_lease;
}

template <core_concepts::exec Exec>
basic_reply<Exec>::lease_t &basic_reply<Exec>::lease() noexcept
{
	return *m_impl->m_lease;
}

template <core_concepts::exec Exec>
const basic_reply<Exec>::parser_t &basic_reply<Exec>::parser() const noexcept
{
	return m_impl->m_parser;
}

template <core_concepts::exec Exec>
basic_reply<Exec>::parser_t &basic_reply<Exec>::parser() noexcept
{
	return m_impl->m_parser;
}

template <core_concepts::exec Exec>
basic_reply<Exec>::executor_t basic_reply<Exec>::get_executor() noexcept
{
	return m_impl->m_exec;
}

template <core_concepts::exec Exec>
basic_reply<Exec> &basic_reply<Exec>::bind_cookie_jar
(std::shared_ptr<cookie_jar> jar, url origin)
{
	m_impl->m_cookie_jar = std::move(jar);
	m_impl->m_origin = std::move(origin);
	m_impl->capture_cookies();
	return *this;
}

template <core_concepts::exec Exec>
basic_reply<Exec> &basic_reply<Exec>::cancel() noexcept
{
	if( m_impl->m_lease and m_impl->m_lease->is_valid() )
	{
		ignore_unused(m_impl->m_lease->get().cancel());
		if( auto connection = m_impl->m_lease->take() )
			ignore_unused(connection->close());
	}
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REPLY_H
