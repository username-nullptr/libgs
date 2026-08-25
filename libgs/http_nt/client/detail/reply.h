
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_reply<Connection>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(connection_ptr connection) :
		m_connection(std::move(connection)) {
		check_active();
	}
	impl(connection_ptr connection, parser_t &&parser) :
		m_connection(std::move(connection)),
		m_parser(std::move(parser)) {
		check_active();
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
					return sys_unexpected(expected.error());
				if( *expected )
				{
					capture_cookies();
					return m_parser.status();
				}
			}
			else
			{
				capture_cookies();
				return m_parser.status();
			}
		}
		auto &sock = m_connection->opt_helper();
		if( not sock.is_open() )
		{
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
			auto sum = sock.read(buffer(buf, buf_size), error);
			if( error )
			{
				sock.close();
				return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, sum});
			if( not expected )
			{
				sock.close();
				return sys_unexpected(expected.error());
			}
			else if( *expected )
				break;
		}
		capture_cookies();
		return m_parser.status();
	}

	[[nodiscard]] awaitable<sys_expected<status_enum>> co_wait
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		else if( m_parser.stage() != parser_t::stage_t::header )
		{
			if( m_parser.is_informational() )
			{
				auto expected = m_parser.next_message();
				if( not expected )
					co_return sys_unexpected(expected.error());

				if( *expected )
				{
					capture_cookies();
					co_return m_parser.status();
				}
			}
			else
			{
				capture_cookies();
				co_return m_parser.status();
			}
		}
		auto &sock = m_connection->opt_helper();
		if( not sock.is_open() )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		using namespace libgs::operators;
		std::error_code error;

		sock.non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<sys_expected<status_enum>>
		{
			for(;;)
			{
				auto sum = co_await sock.read (
					buffer(buf, buf_size), use_awaitable | cancel_slot | error
				);
				if( error )
				{
					sock.close();
					co_return sys_unexpected(error);
				}
				auto expected = m_parser.append({buf, sum});
				if( not expected )
					co_return sys_unexpected(expected.error());

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
				coro::sleep_for(m_connection->get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		if( expected )
			capture_cookies();
		co_return expected;
	}

	[[nodiscard]] awaitable<sys_expected<status_enum>> co_wait(std::error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_wait (
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] io_expected read(const mutable_buffer &buf) noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		auto &sock = m_connection->opt_helper();
		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			return sys_unexpected (
				make_error_code(errc::eof)
			);
		}
		else if( not sock.is_open() )
		{
			return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		else if( m_parser.stage() == parser_t::stage_t::header )
		{
			auto expected = wait();
			if( not expected )
				return io_unexpected(expected.error());
		}
		size_t sum = 0;
		if( buf.size() == 0 )
			return sum;

		asio::socket_base::receive_buffer_size op;
		std::error_code error;
		sock.get_option(op, error);

		if( error )
			return io_unexpected(error);

		auto dst_buf = static_cast<char*>(buf.data());
		for(;;)
		{
			auto body = m_parser.take_partial_body(buf.size() - sum);
			std::memcpy(dst_buf + sum, body.c_str(), body.size());

			sum += body.size();
			if( sum == buf.size() or m_parser.stage() == parser_t::stage_t::finished )
				break;

			body = std::string(op.value(),'\0');
			for(;;)
			{
				auto tmp_sum = sock.read (
					{body.data(), body.size()}, error
				);
				if( error )
				{
					sock.close();
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
					sock.close();
					return io_unexpected(expected.error());
				}
				else if( *expected )
					break;
			}
		}
		if( sum == 0 )
		{
			auto it = m_parser.headers().find("Connection");
			if( it == m_parser.headers().end() )
			{
				if( m_parser.version() < version::v11 )
					sock.close();
			}
			else if( it->second == "close" )
				sock.close();

			return io_unexpected (
				make_error_code(errc::eof)
			);
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_read(const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		auto &sock = m_connection->opt_helper();
		if( m_parser.stage() == parser_t::stage_t::finished )
		{
			co_return sys_unexpected (
				make_error_code(errc::eof)
			);
		}
		else if( not sock.is_open() )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		size_t sum = 0;
		if( buf.size() == 0 )
			co_return sum;

		asio::socket_base::receive_buffer_size op;
		std::error_code error;
		sock.get_option(op, error);

		if( error )
			co_return io_unexpected(error);

		using namespace libgs::operators;
		auto dst_buf = reinterpret_cast<char*>(buf.data());
		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<io_expected>
		{
			if( m_parser.stage() == parser_t::stage_t::header )
			{
				using namespace std::chrono_literals;
				auto expected = co_await co_wait(cancel_slot, 0ns);
				if( not expected )
					co_return io_unexpected(expected.error());
			}
			for(;;)
			{
				auto body = m_parser.take_partial_body(buf.size() - sum);
				std::memcpy(dst_buf + sum, body.c_str(), body.size());

				sum += body.size();
				if( sum == buf.size() or m_parser.stage() == parser_t::stage_t::finished )
					break;

				body = std::string(op.value(),'\0');
				for(;;)
				{
					auto tmp_sum = co_await sock.read({body.data(), body.size()},
						use_awaitable | cancel_slot | error
					);
					if( error )
					{
						sock.close();
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
						sock.close();
						co_return io_unexpected(expected.error());
					}
					else if( *expected )
						break;
				}
			}
			if( sum == 0 )
			{
				co_return io_unexpected (
					make_error_code(errc::eof)
				);
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
				coro::sleep_for(m_connection->get_executor(), timeout)
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

	[[nodiscard]] awaitable<io_expected> co_read(std::error_code &error, const mutable_buffer &body,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_read(body,
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

	[[nodiscard]] sys_expected<byte_range_chunk> read_range_body() noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		if( m_parser.stage() == stage::header )
		{
			auto expected = wait();
			if( not expected )
				return sys_unexpected(expected.error());
		}
		auto &sock = m_connection->opt_helper();
		for(;;)
		{
			if( auto chunk = m_parser.take_range_body(128 * 1024) )
				return std::move(*chunk);

			if( m_parser.stage() == stage::finished )
				return sys_unexpected(make_error_code(errc::eof));

			if( not sock.is_open() )
				return sys_unexpected(make_error_code(std::errc::not_connected));

			char buf[128 * 1024] {};
			error_code error {};
			auto size = sock.read(buffer(buf), error);

			if( error )
			{
				sock.close();
				return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, size});
			if( not expected )
			{
				sock.close();
				return sys_unexpected(expected.error());
			}
		}
	}

	[[nodiscard]] awaitable<sys_expected<byte_range_chunk>>
	co_read_range_body(asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		if( m_parser.stage() == stage::header )
		{
			using namespace std::chrono_literals;
			auto expected = co_await co_wait(cancel_slot, 0ns);
			if( not expected )
				co_return sys_unexpected(expected.error());
		}
		auto &sock = m_connection->opt_helper();
		for(;;)
		{
			if( auto chunk = m_parser.take_range_body(128 * 1024) )
				co_return std::move(*chunk);

			if( m_parser.stage() == stage::finished )
				co_return sys_unexpected(make_error_code(errc::eof));

			if( not sock.is_open() )
				co_return sys_unexpected(make_error_code(std::errc::not_connected));

			char buf[128 * 1024] {};
			error_code error {};

			auto size = co_await sock.read (
				buffer(buf), use_awaitable | cancel_slot | error
			);
			if( error )
			{
				sock.close();
				co_return sys_unexpected(error);
			}
			auto expected = m_parser.append({buf, size});
			if( not expected )
			{
				sock.close();
				co_return sys_unexpected(expected.error());
			}
		}
	}

public:
	[[nodiscard]] sys_expected<std::string> read() noexcept
	{
		std::string sum {};
		for(;;)
		{
			constexpr size_t buf_size = 0xFFFF;
			char buf[buf_size] {0};

			auto expected = read(buffer(buf,buf_size));
			if( expected )
			{
				sum += std::string(buf, *expected);
				continue;
			}
			else if( expected.error() == errc::eof )
				break;
			return sys_unexpected(expected.error());
		}
		return sum;
	}

	[[nodiscard]] awaitable<sys_expected<std::string>> co_read
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<sys_expected<std::string>>
		{
			std::string sum {};
			for(;;)
			{
				constexpr size_t buf_size = 0xFFFF;
				char buf[buf_size] {0};

				auto expected = co_await co_read (
					buffer(buf,buf_size), cancel_slot, 0ns
				);
				if( expected )
				{
					sum += std::string(buf, *expected);
					continue;
				}
				else if( expected.error() == errc::eof )
					break;
				co_return sys_unexpected(expected.error());
			}
			co_return sum;
		},
		use_awaitable);

		sys_expected<std::string> expected;
		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_connection->get_executor(), timeout)
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

	[[nodiscard]] awaitable<sys_expected<std::string>> co_read(std::error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_read (
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] io_expected save_file(auto &&opt, auto &&progress) noexcept
	{
		io_expected expected {};
		if( m_parser.stage() == stage::header )
		{
			auto status = wait();
			if( not status )
				return expected.despair(status.error());
		}
		if( m_parser.status() == status::range_not_satisfiable )
		{
			return expected.despair (
				std::make_error_code(std::errc::result_out_of_range)
			);
		}
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt),
			m_parser.status() == status::partial_content
		);
		if( not token )
			return expected.despair(token.error());

		auto before = m_connection->set_receive_file_option();
		if( not before )
			return expected.despair(before.error());

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
		auto restored = m_connection->unset_transfer_file_option(*before);

		if( not expected and expected.error() != errc::eof )
			return expected;

		if( not restored )
			return expected.despair(restored.error());
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_save_file(auto &&opt, auto &&progress,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		io_expected expected {};
		if( m_parser.stage() == stage::header )
		{
			auto status = co_await co_wait(cancel_slot, 0ns);
			if( not status )
				co_return expected.despair(status.error());
		}
		if( m_parser.status() == status::range_not_satisfiable )
		{
			co_return expected.despair (
				std::make_error_code(std::errc::result_out_of_range)
			);
		}
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt),
			m_parser.status() == status::partial_content
		);
		if( not token )
			co_return expected.despair(token.error());

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<io_expected>
		{
			auto before = m_connection->set_receive_file_option();
			if( not before )
				co_return expected.despair(before.error());

			size_t sum = 0, total = 0;
			if( auto complete = m_parser.complete_length() )
				total = *complete;

			else if( auto length = m_parser.header(header::content_length);
				not m_parser.is_range_response() and length )
				total = *length->get<size_t>().or_else(0);

			for(;;)
			{
				auto chunk = co_await co_read_range_body(cancel_slot);
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
			auto restored = m_connection->unset_transfer_file_option(*before);

			if( not expected and expected.error() != errc::eof )
				co_return expected;

			if( not restored )
				co_return expected.despair(restored.error());
			co_return sum;
		},
		use_awaitable | cancel_slot);

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_connection->get_executor(), timeout)
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

	[[nodiscard]] awaitable<io_expected> co_save_file(std::error_code &error,
		auto &&opt, auto &&progress, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_save_file (
			std::forward<decltype(opt)>(opt), std::forward<decltype(progress)>(progress),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
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
	auto make_file_opt_token(Opt &&opt, bool preserve) noexcept
	{
		using opt_t = std::remove_cvref_t<Opt>;
		if constexpr( is_any_string_v<opt_t> or std::same_as<opt_t,std::filesystem::path> or
			is_fstream_v<opt_t,char> or is_ofstream_v<opt_t,char> )
		{
			auto token = http_nt::make_file_opt_token(std::forward<Opt>(opt));
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

	void check_active() noexcept
	{
		if ( not m_connection->peek() )
		{
			m_first_error = errc::not_connected;
			return ;
		}
		auto &sock = m_connection->opt_helper();
		char buffer[0xFFFF] {0};

		auto sum_expected = sock.try_read({buffer, sizeof(buffer)});
		if( sum_expected )
		{
			if( *sum_expected == 0 )
				return ;

			auto expected = m_parser.append({buffer, *sum_expected});
			if( not expected )
			{
				m_first_error = expected.error();
				sock.close();
			}
		}
		else if( sum_expected.error() != errc::try_again and
				 sum_expected.error() != errc::would_block )
		{
			m_first_error = sum_expected.error();
			sock.close();
		}
	}

public:
	connection_ptr m_connection {};
	error_code m_first_error {};
	parser_t m_parser {};

	std::shared_ptr<cookie_jar> m_cookie_jar {};
	url m_origin {};

	bool m_cookies_captured = false;
};

template <concepts::connection Connection>
basic_reply<Connection>::basic_reply(connection_ptr connection) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::make_shared<impl>(std::move(connection)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
}

template <concepts::connection Connection>
basic_reply<Connection>::basic_reply(connection_ptr connection, parser_t &&parser) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::make_shared<impl>(std::move(connection), std::move(parser)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
}

template <concepts::connection Connection>
basic_reply<Connection>::~basic_reply() = default;

template <concepts::connection Connection>
template <typename Token>
auto basic_reply<Connection>::wait(Token &&token) noexcept
	requires task_token_v<Token,status_enum>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->wait()
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->wait();

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
				return m_impl->co_wait(no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_wait (
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_wait(no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_wait (
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					no_time_token, cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_wait (
						no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_wait (
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				no_time_token, original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_wait (
					no_time_token.ec_, cancel_slot, timeout
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_wait (
					cancel_slot, timeout
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
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return wait(token | 0ns);
	}
}

template <concepts::connection Connection>
version_enum basic_reply<Connection>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <concepts::connection Connection>
status_enum basic_reply<Connection>::status() const noexcept
{
	return m_impl->m_parser.status();
}

template <concepts::connection Connection>
template <typename Token>
auto basic_reply<Connection>::read(const mutable_buffer &buf, Token &&token) noexcept
	requires task_token_v<Token,size_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->read(buf)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->read(buf);

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
				return m_impl->co_read(no_time_token.ec_, buf,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_read(buf,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_read(no_time_token.ec_, buf,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_read(buf,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), no_time_token, buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						no_time_token.ec_, buf, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						buf, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				no_time_token, original_token, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
					no_time_token.ec_, buf, cancel_slot, timeout
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				original_token, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
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
	else
	{
		using namespace operators;
		using namespace std::chrono_literals;
		return read(buf, token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename Token>
auto basic_reply<Connection>::read(Token &&token) noexcept
	requires task_token_v<Token,std::string>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->read()
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->read();

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
				return m_impl->co_read(no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_read (
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_read(no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_read (
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), no_time_token, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				no_time_token, original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
					no_time_token.ec_, cancel_slot, timeout
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
					cancel_slot, timeout
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
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return read(token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename T, typename Token>
auto basic_reply<Connection>::save_file(T &&opt, Token &&token) noexcept
	requires file_task_token<T,Token>
{
	return save_file(std::forward<T>(opt),
		[](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <concepts::connection Connection>
template <typename T, typename Progress, typename Token>
auto basic_reply<Connection>::save_file(T &&opt, Progress &&progress, Token &&token) noexcept
	requires file_task_token<T,Token> and concepts::progress_callback<Progress,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->save_file(std::forward<T>(opt), std::forward<Progress>(progress))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->save_file(std::forward<T>(opt), std::forward<Progress>(progress));

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
				return m_impl->co_save_file(no_time_token.ec_,
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_save_file (
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_save_file(no_time_token.ec_,
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_save_file (
					std::forward<T>(opt), std::forward<Progress>(progress),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), no_time_token, promise = std::move(promise),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_save_file (
						std::forward<T>(opt), std::forward<Progress>(progress),
						no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_save_file (
						std::forward<T>(opt), std::forward<Progress>(progress),
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
				no_time_token, original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_save_file (
					std::forward<T>(opt), std::forward<Progress>(progress),
					no_time_token.ec_, cancel_slot, timeout
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				opt = std::forward<T>(opt), progress = std::forward<Progress>(progress),
				original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_save_file (
					std::forward<T>(opt), std::forward<Progress>(progress),
					cancel_slot, timeout
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
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return save_file (
			std::forward<T>(opt), std::forward<Progress>(progress),
			token | 0ns
		);
	}
}

template <concepts::connection Connection>
bool basic_reply<Connection>::valid() const noexcept
{
	return m_impl->m_parser.stage() != stage::header;
}

template <concepts::connection Connection>
error_code basic_reply<Connection>::first_error() const noexcept
{
	return m_impl->m_first_error;
}

template <concepts::connection Connection>
bool basic_reply<Connection>::content_decoded() const noexcept
{
	return m_impl->m_parser.content_decoded();
}

template <concepts::connection Connection>
bool basic_reply<Connection>::is_chunked() const noexcept
{
	return m_impl->m_parser.is_chunked();
}

template <concepts::connection Connection>
bool basic_reply<Connection>::is_eof() const noexcept
{
	return m_impl->m_parser.stage() == stage::finished;
}

template <concepts::connection Connection>
bool basic_reply<Connection>::is_upgrade() const noexcept
{
	return m_impl->m_parser.is_upgrade();
}

template <concepts::connection Connection>
std::string basic_reply<Connection>::take_pending_data()
{
	return m_impl->m_parser.take_pending_data();
}

template <concepts::connection Connection>
const basic_reply<Connection>::connection_t &basic_reply<Connection>::connection() const noexcept
{
	return *m_impl->m_connection;
}

template <concepts::connection Connection>
basic_reply<Connection>::connection_t &basic_reply<Connection>::connection() noexcept
{
	return *m_impl->m_connection;
}

template <concepts::connection Connection>
const basic_reply<Connection>::parser_t &basic_reply<Connection>::parser() const noexcept
{
	return m_impl->m_parser;
}

template <concepts::connection Connection>
basic_reply<Connection>::parser_t &basic_reply<Connection>::parser() noexcept
{
	return m_impl->m_parser;
}

template <concepts::connection Connection>
basic_reply<Connection>::executor_t basic_reply<Connection>::get_executor() noexcept
{
	return connection().get_executor();
}

template <concepts::connection Connection>
basic_reply<Connection> &basic_reply<Connection>::bind_cookie_jar
(std::shared_ptr<cookie_jar> jar, url origin)
{
	m_impl->m_cookie_jar = std::move(jar);
	m_impl->m_origin = std::move(origin);
	m_impl->capture_cookies();
	return *this;
}

template <concepts::connection Connection>
basic_reply<Connection> &basic_reply<Connection>::cancel() noexcept
{
	connection().cancel();
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H
