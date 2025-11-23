
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REPLY_H
#define LIBGS_HTTP_CLIENT_DETAIL_REPLY_H

namespace libgs::http
{

template <concepts::connection Session>
class LIBGS_HTTP_TAPI basic_reply<Session>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using reply_ptr = std::shared_ptr<basic_reply>;

public:
	impl(session_t &&session, parser_t &&parser) :
		m_session(std::move(session)), m_parser(std::move(parser)) {}

	impl(impl &&other) noexcept :
		m_session(std::move(other.m_session)) {}

	impl& operator=(impl &&other) noexcept
	{
		m_session = std::move(other.m_session);
		return *this;
	}

public:
	[[nodiscard]] static sys_expected<reply_ptr> make(session_t &&session) noexcept
	{
		using namespace libgs::operators;
		auto &sock = session.opt_helper();

		parser_t parser;
		std::error_code error;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];
		for(;;)
		{
			auto sum = sock.read(buffer(buf, buf_size), error);
			if( error )
				return sys_unexpected(error);

			auto expected = parser.append({buf, sum});
			if( not expected )
				return sys_unexpected(expected.error());

			else if( *expected )
				break;
		}
		return std::make_shared<basic_reply> (
			std::move(session), std::move(parser)
		);
	}

	[[nodiscard]] static awaitable<sys_expected<reply_ptr>> co_make(session_t &&session,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		std::error_code error;

		auto &sock = session.opt_helper();
		sock.non_blocking(true, error);
		if( error )
			co_return io_unexpected(error);

		parser_t parser;
		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];

		auto task = libgs::dispatch(session.get_executor(),
		[&]() mutable -> awaitable<sys_expected<reply_ptr>>
		{
			for(;;)
			{
				auto sum = co_await sock.read (
					buffer(buf, buf_size), use_awaitable | error
				);
				if( error )
					co_return sys_unexpected(error);

				auto expected = parser.append({buf, sum});
				if( not expected )
					co_return sys_unexpected(expected.error());

				else if( *expected )
					break;
			}
			co_return std::make_shared<basic_reply> (
				std::move(session), std::move(parser)
			);
		},
		use_awaitable);

		using namespace std::chrono_literals;
		sys_expected<reply_ptr> expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(session.get_executor(), timeout)
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

	[[nodiscard]] static awaitable<sys_expected<reply_ptr>> co_read(std::error_code &error, session_t &&session,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_make(std::move(session),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] io_expected read(const mutable_buffer &buf) noexcept
	{
		size_t sum = 0;
		if( buf.size() == 0 )
			return sum;

		auto &sock = m_session.opt_helper();
		asio::socket_base::receive_buffer_size op;

		std::error_code error;
		sock.get_option(op, error);

		if( error )
			return sys_unexpected(error);

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
					return sum;

				auto expected = m_parser.append({body.data(), tmp_sum});
				if( not expected )
					return expected;
				else if( *expected )
					break;
			}
		}
		if( sum == 0 )
		{
			auto it = m_parser.headers().find("Connection");
			if( it == m_parser.headers().end() )
			{
				if( m_parser.version() < protocol::version::v11 )
					m_session.opt_helper().close();
			}
			else if( it->second == "close" )
				m_session.opt_helper().close();

			return sys_unexpected (
				make_error_code(errc::eof)
			);
		}
		return sum;
	}

	[[nodiscard]] awaitable<io_expected> co_read(const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		size_t sum = 0;
		if( buf.size() == 0 )
			co_return sum;

		auto &sock = m_session.opt_helper();
		asio::socket_base::receive_buffer_size op;

		std::error_code error;
		sock.get_option(op, error);

		if( error )
			co_return sys_unexpected(error);

		using namespace libgs::operators;
		auto dst_buf = reinterpret_cast<char*>(buf.data());
		auto task = libgs::dispatch(m_session.get_executor(), [&]() mutable -> awaitable<io_expected>
		{
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
						co_return sys_unexpected(error);

					auto expected = m_parser.append({body.data(), tmp_sum});
					if( not expected )
						co_return expected;
					else if( *expected )
						break;
				}
			}
			if( sum == 0 )
			{
				co_return sys_unexpected (
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

			auto expected = read(buffer(buf,buf_size)).transform([&](size_t s) {
				sum += std::string(buf, s);
			});
			if( expected )
				continue;

			else if( expected.error() == errc::eof )
				break;
			return expected;
		}
		return sum;
	}

	[[nodiscard]] awaitable<sys_expected<std::string>> co_read
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		auto task = libgs::dispatch(m_session.get_executor(),
		[&]() mutable -> awaitable<sys_expected<std::string>>
		{
			std::string sum;
			for(;;)
			{
				constexpr size_t buf_size = 0xFFFF;
				char buf[buf_size] {0};

				auto expected = co_await co_read (
					buffer(buf,buf_size), cancel_slot, 0ns
				);
				expected.transform([&](size_t s) {
					sum += std::string(buf, s);
				});
				if( expected )
					continue;

				else if( expected.error() == errc::eof )
					break;
				co_return expected;
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

private:

public:
	session_t m_session;
	parser_t m_parser;
};

template <concepts::connection Session>
basic_reply<Session>::basic_reply(session_t &&session, parser_t &&parser) :
	m_impl(std::make_shared<impl>(std::move(session), std::move(parser)))
{

}

template <concepts::connection Session>
basic_reply<Session>::~basic_reply() = default;

template <concepts::connection Session>
basic_reply<Session>::basic_reply(basic_reply &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::connection Session>
basic_reply<Session> &basic_reply<Session>::operator=(basic_reply &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::connection Session>
template <typename Token>
auto basic_reply<Session>::make(session_t &&session, Token &&token) noexcept
	requires task_token_v<Token,std::shared_ptr<basic_reply>>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return impl::make(std::move(session))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return impl::make(std::move(session));

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
				return impl::co_make(ntoken.ec_, std::move(session),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return impl::co_make(std::move(session),
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
				libgs::dispatch(session.get_executor(), [promise = std::move(promise),
					ntoken, session = std::make_shared<session_t>(std::move(session)),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl::co_make (
						ntoken.ec_, std::move(*session), cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(session.get_executor(), [promise = std::move(promise),
					session = std::make_shared<session_t>(std::move(session)),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl::co_make (
						std::move(*session), cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(session.get_executor(), [
				ntoken, nntoken, timeout = get_associated_redirect_time(token),
				session = std::make_shared<session_t>(std::move(session)),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl::co_make (
					ntoken.ec_, std::move(*session), cancel_slot, timeout
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
			libgs::dispatch(session.get_executor(), [
				nntoken, timeout = get_associated_redirect_time(token),
				session = std::make_shared<session_t>(std::move(session)),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl::co_make (
					std::move(*session), cancel_slot, timeout
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
		return make(std::move(session), token | 0ns);
	}
}

template <concepts::connection Session>
protocol::version_enum basic_reply<Session>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <concepts::connection Session>
protocol::status_enum basic_reply<Session>::status() const noexcept
{
	return m_impl->m_parser.status();
}

template <concepts::connection Session>
optional<typename basic_reply<Session>::value_t>
basic_reply<Session>::header(const core_concepts::text_p<char> auto &key) const noexcept
{
	return m_impl->m_parser.header(key);
}

template <concepts::connection Session>
const basic_reply<Session>::headers_t &basic_reply<Session>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

template <concepts::connection Session>
optional<typename basic_reply<Session>::cookie_t>
basic_reply<Session>::cookie(const core_concepts::text_p<char> auto &key) const noexcept
{
	return m_impl->m_parser.cookie(key);
}

template <concepts::connection Session>
const protocol::cookies &basic_reply<Session>::cookies() const noexcept
{
	return m_impl->m_parser.cookies();
}

template <concepts::connection Session>
template <typename Token>
auto basic_reply<Session>::read(const mutable_buffer &buf, Token &&token) noexcept
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
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						ntoken.ec_, buf, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
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
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
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
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, buf, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
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

template <concepts::connection Session>
template <typename Token>
auto basic_reply<Session>::read(Token &&token) noexcept
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
				return m_impl->co_read(
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
					impl = m_impl->shared_from_this(), ntoken, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await impl->co_read (
						ntoken.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [
					impl = m_impl->shared_from_this(), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
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
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				ntoken, nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
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
			libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(),
				nntoken, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
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

template <concepts::connection Session>
bool basic_reply<Session>::is_chunked() const noexcept
{
	if( version() < protocol::version::v11 )
		return false;
	auto it = m_impl->m_headers.find(protocol::header::transfer_encoding);
	return it != m_impl->m_headers.end() and str_to_lower(it->second) == "chunked";
}

template <concepts::connection Session>
bool basic_reply<Session>::is_eof() const noexcept
{
	return m_impl->m_parser.stage() == parser_t::stage_t::finished;
}

template <concepts::connection Session>
const basic_reply<Session>::session_t &basic_reply<Session>::session() const noexcept
{
	return m_impl->m_session;
}

template <concepts::connection Session>
basic_reply<Session>::session_t &basic_reply<Session>::session() noexcept
{
	return m_impl->m_session;
}

template <concepts::connection Session>
basic_reply<Session>::executor_t basic_reply<Session>::get_executor() noexcept
{
	return session().get_executor();
}

template <concepts::connection Session>
basic_reply<Session> &basic_reply<Session>::cancel() noexcept
{
	session().opt_helper().cancel();
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REPLY_H
