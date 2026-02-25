
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
	explicit impl(connection_t &&connection) :
		m_connection(std::move(connection))
	{
		auto &sock = m_connection.opt_helper();
		char buffer[0xFFFF] {0};
		size_t sum = 0;
		for(;;)
		{
			auto expected = sock.try_read({buffer, sizeof(buffer)});
			if( expected or (expected.error() == errc::try_again or
				expected.error() == errc::would_block) )
				break;
			/* else if( m_first_error == errc::eof ) */
			m_first_error = expected.error();
			sock.close();
			return ;
		}
		auto expected = m_parser.append({buffer, sum});
		if( not expected )
		{
			m_first_error = expected.error();
			sock.close();
		}
	}

public:
	[[nodiscard]] sys_expected<status_enum> wait() noexcept
	{
		if( m_first_error )
			return sys_unexpected(m_first_error);

		auto &sock = m_connection.opt_helper();
		if( not sock.is_open() )
		{
			return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		else if( m_parser.stage() != parser_t::stage_t::header )
			return m_parser.status();

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
		return m_parser.status();
	}

	[[nodiscard]] awaitable<sys_expected<status_enum>> co_wait
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( m_first_error )
			co_return sys_unexpected(m_first_error);

		auto &sock = m_connection.opt_helper();
		if( not sock.is_open() )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		else if( m_parser.stage() != parser_t::stage_t::header )
			co_return m_parser.status();

		using namespace libgs::operators;
		std::error_code error;

		sock.non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];

		auto task = libgs::dispatch(m_connection.get_executor(),
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

		auto &sock = m_connection.opt_helper();
		if( not sock.is_open() )
		{
			return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		else if( m_parser.stage() == parser_t::stage_t::finished )
		{
			return sys_unexpected (
				make_error_code(errc::eof)
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

		auto dst_buf = reinterpret_cast<char*>(buf.data());
		for(;;)
		{
			auto body = m_parser.take_partial_body(buf.size() - sum);
			std::memcpy(dst_buf + sum, body.c_str(), body.size());

			sum += body.size();
			if( m_parser.stage() == parser_t::stage_t::finished )
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
					return sum;
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

		auto &sock = m_connection.opt_helper();
		if( not sock.is_open() )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_connected)
			);
		}
		else if( m_parser.stage() == parser_t::stage_t::finished )
		{
			co_return sys_unexpected (
				make_error_code(errc::eof)
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
		auto task = libgs::dispatch(m_connection.get_executor(),
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
				if( m_parser.stage() == parser_t::stage_t::finished )
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

public:
	[[nodiscard]] sys_expected<std::string> read() noexcept
	{
		std::string sum;
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
		auto task = libgs::dispatch(m_connection.get_executor(),
		[&]() mutable noexcept -> awaitable<sys_expected<std::string>>
		{
			std::string sum;
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
	connection_t m_connection;
	error_code m_first_error {};
	parser_t m_parser {};
};

template <concepts::connection Connection>
basic_reply<Connection>::basic_reply(connection_t &&connection) :
	const_headers<basic_reply>(nullptr),
	const_cookies<cookie,basic_reply>(nullptr),
	m_impl(std::make_shared<impl>(std::move(connection)))
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
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_wait(ntoken.ec_,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_wait (
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
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					ntoken, cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_wait (
						ntoken.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
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
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_wait (
					ntoken.ec_, cancel_slot, timeout
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
				nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_wait (
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
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_read(ntoken.ec_, buf,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_read(buf,
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
					impl = m_impl->shared_from_this(), ntoken, buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						ntoken.ec_, buf, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
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
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
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
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
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
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_read(ntoken.ec_,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_read (
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
					impl = m_impl->shared_from_this(), ntoken, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						ntoken.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
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
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
					ntoken.ec_, cancel_slot, timeout
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
				nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await impl->co_read (
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
		return read(token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename T, typename Token>
auto basic_reply<Connection>::save_file(T &&opt, Token &&token) noexcept
	requires file_opt_token<T> and task_token_v<Token,size_t>
{
	// TODO ... ...
	return 0;
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
const basic_reply<Connection>::connection_t &basic_reply<Connection>::connection() const noexcept
{
	return m_impl->m_connection;
}

template <concepts::connection Connection>
basic_reply<Connection>::connection_t &basic_reply<Connection>::connection() noexcept
{
	return m_impl->m_connection;
}

template <concepts::connection Connection>
basic_reply<Connection>::executor_t basic_reply<Connection>::get_executor() noexcept
{
	return connection().get_executor();
}

template <concepts::connection Connection>
basic_reply<Connection> &basic_reply<Connection>::cancel() noexcept
{
	connection().cancel();
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_REPLY_H