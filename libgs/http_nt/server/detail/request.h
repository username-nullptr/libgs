
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_request<Connection>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	explicit impl(connection_ptr connection) :
		m_connection(std::move(connection)) {}

	impl(connection_ptr connection, parser_t &&parser) :
		m_connection(std::move(connection)),
		m_parser(std::move(parser)) {}

public:
	void wait(error_code &error) noexcept
	{
		error.clear();
		if( m_parser.stage() != parser_t::stage_t::header )
			return ;

		auto &sock = m_connection->opt_helper();
		if( not sock.is_open() )
		{
			error = make_error_code(std::errc::not_connected);
			return ;
		}
		using namespace libgs::operators;
		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];
		for(;;)
		{
			auto sum = sock.read(buffer(buf, buf_size), error);
			if( error )
			{
				sock.close();
				return ;
			}
			auto expected = m_parser.append({buf, sum});
			if( not expected )
			{
				sock.close();
				error = expected.error();
				return ;
			}
			else if( *expected )
				break;
		}
	}

	[[nodiscard]] awaitable<void> co_wait(error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		error.clear();
		if( m_parser.stage() != parser_t::stage_t::header )
			co_return ;

		auto &sock = m_connection->opt_helper();
		if( not sock.is_open() )
		{
			error = make_error_code(std::errc::not_connected);
			co_return ;
		}
		using namespace libgs::operators;
		sock.non_blocking(true, error);
		if( error )
			co_return ;

		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size];

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			for(;;)
			{
				auto sum = co_await sock.read (
					buffer(buf, buf_size), use_awaitable | cancel_slot | error
				);
				if( error )
				{
					sock.close();
					co_return ;
				}
				auto expected = m_parser.append({buf, sum});
				if( not expected )
				{
					error = expected.error();
					co_return ;
				}
				else if( *expected )
					break;
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
		co_return ;
	}

	[[nodiscard]] awaitable<void> co_wait
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		co_await co_wait (
			error, std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::wait"
			);
		}
		co_return ;
	}

public:
	[[nodiscard]] size_t read(const mutable_buffer &buf, error_code &error) noexcept
	{
		error.clear();
		const size_t buf_size = buf.size();
		if( buf_size == 0 )
			return 0;

		else if( m_parser.stage() != stage::body )
		{
			error = std::make_error_code (
				static_cast<std::errc>(errc::eof)
			);
			return 0;
		}
		auto &sock_helper = m_connection.opt_helper();
		asio::socket_base::receive_buffer_size op;

		sock_helper.get_option(op, error);
		if( error )
			return 0;

		auto dst_buf = static_cast<char*>(buf.data());
		size_t sum = 0;
		do {
			auto body = m_parser.take_partial_body(buf_size);
			sum += body.size();

			memcpy(dst_buf + sum, body.c_str(), body.size());
			if( sum == buf_size or m_parser.stage() == stage::finished )
				break;

			body = std::string(op.value(),'\0');
			for(;;)
			{
				auto tmp_sum = sock_helper.read (
					{body.data(), body.size()}, error
				);
				if( error )
					return sum;

				auto expected = m_parser.append({body.data(), tmp_sum});
				if( not expected )
					return sum;
				else if( *expected )
					break;
			}
		}
		while(true);
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_read(const mutable_buffer &buf, error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		error.clear();
		size_t sum = 0;
		const size_t buf_size = buf.size();
		if( buf_size == 0 )
			co_return sum;

		else if( m_parser.stage() != stage::body )
		{
			error = std::make_error_code (
				static_cast<std::errc>(errc::eof)
			);
			co_return 0;
		}
		auto &sock_helper = m_connection->opt_helper();
		asio::socket_base::receive_buffer_size op;

		sock_helper.get_option(op, error);
		if( error )
			co_return 0;

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			auto dst_buf = static_cast<char*>(buf.data());
			do {
				auto body = m_parser.take_partial_body(buf_size);
				sum += body.size();

				memcpy(dst_buf + sum, body.c_str(), body.size());
				if( sum == buf_size or m_parser.stage() == stage::finished )
					break;

				using namespace libgs::operators;
				body = std::string(op.value(),'\0');
				for(;;)
				{
					auto tmp_sum = co_await sock_helper.read (
						{body.data(), body.size()}, use_awaitable | cancel_slot | error
					);
					if( error )
						co_return ;

					auto expected = m_parser.append({body.data(), tmp_sum});
					if( not expected )
						co_return ;
					else if( *expected )
						break;
				}
			}
			while(true);
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

	[[nodiscard]] awaitable<size_t> co_read(const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		auto sum = co_await co_read (
			buf, error, std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::read"
			);
		}
		co_return sum;
	}

public:
	[[nodiscard]] std::vector<std::byte> read(error_code &error) noexcept
	{
		error.clear();
		std::vector<std::byte> sum {};
		if( m_parser.stage() != stage::body )
			return sum;

		asio::socket_base::receive_buffer_size op {};
		m_connection.opt_helper().get_option(op, error);
		if( error )
			return sum;

		auto buf_size = static_cast<size_t>(op.value());
		auto buffer = std::make_shared<char[]>(buf_size);
		do {
			auto bytes = read({buffer.get(), buf_size}, error);
			if( error )
				return sum;

			auto ptr = reinterpret_cast<std::byte*>(buffer.get());
			sum.insert(sum.end(), ptr, ptr + bytes);
		}
		while( m_parser.stage() == stage::body );
		return sum;
	}

	[[nodiscard]] awaitable<std::vector<std::byte>> co_read(error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		error.clear();
		std::vector<std::byte> sum {};
		if( m_parser.stage() != stage::body )
			co_return sum;

		asio::socket_base::receive_buffer_size op {};
		m_connection->opt_helper().get_option(op, error);
		if( error )
			co_return sum;

		using namespace std::chrono_literals;
		using namespace libgs::operators;

		auto task = libgs::dispatch(m_connection->get_executor(),
		[&]() mutable noexcept -> awaitable<void>
		{
			auto buf_size = static_cast<size_t>(op.value());
			auto buffer = std::make_shared<char[]>(buf_size);
			do {
				auto bytes = co_await co_read (
					{buffer.get(), buf_size}, error, cancel_slot, 0ns
				);
				if( error )
					co_return ;

				auto ptr = reinterpret_cast<std::byte*>(buffer.get());
				sum.insert(sum.end(), ptr, ptr + bytes);
			}
			while( m_parser.stage() == stage::body );
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

	[[nodiscard]] awaitable<std::vector<std::byte>> co_read
	(asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		auto sum = co_await co_read (
			error, std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::read"
			);
		}
		co_return sum;
	}

public:
	[[nodiscard]] size_t save_file(auto &&opt, error_code &error) noexcept
	{
		error.clear();
		size_t sum = 0;
		auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
		if( not token )
		{
			error = token.error();
			return sum;
		}
		auto before = m_connection->set_receive_file_option();
		if( not before )
		{
			error = before.error();
			return sum;
		}
		constexpr size_t buf_size = 128 * 1024;
		char buffer[buf_size] {0};
		for(;;)
		{
			auto bytes = read({buffer, buf_size}, error);
			if( error )
				break;

			token->stream->write(buffer, bytes);
			sum += bytes;
		}
		token->stream->close();
		if( error and error != errc::eof )
			return sum;

		if( auto expected = m_connection->unset_transfer_file_option(*before); not expected )
			error = expected.error();
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_save_file(auto &&opt, error_code &error,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
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
			auto before = m_connection->set_receive_file_option();
			if( not before )
			{
				error = before.error();
				co_return ;
			}
			constexpr size_t buf_size = 128 * 1024;
			char buffer[buf_size] {0};
			for(;;)
			{
				auto bytes = co_await co_read (
					{buffer, buf_size}, error, cancel_slot
				);
				if( error )
					break;

				token->stream->write(buffer, bytes);
				sum += bytes;
			}
			token->stream->close();
			if( error and error != errc::eof )
				co_return ;

			if( auto expected = m_connection->unset_transfer_file_option(*before); not expected )
				error = expected.error();
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

	[[nodiscard]] awaitable<size_t> co_save_file(auto &&opt,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout)
	{
		error_code error;
		auto sum = co_await co_save_file (
			std::forward<decltype(opt)>(opt), error,
			std::move(cancel_slot), std::move(timeout)
		);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::save_file"
			);
		}
		co_return sum;
	}

public:
	connection_ptr m_connection;
	parser_t m_parser {};
};

template <concepts::connection Connection>
basic_request<Connection>::basic_request(connection_ptr connection) :
	const_headers<basic_request>(nullptr),
	const_cookies<value_t,basic_request>(nullptr),
	const_parameters<basic_request>(nullptr),
	m_impl(new impl(std::move(connection)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
	this->m_parameters = &m_impl->m_parser.parameters();
}

template <concepts::connection Connection>
basic_request<Connection>::basic_request(connection_ptr connection, parser_t &&parser) :
	const_headers<basic_request>(nullptr),
	const_cookies<value_t,basic_request>(nullptr),
	const_parameters<basic_request>(nullptr),
	m_impl(new impl(std::move(connection), std::move(parser)))
{
	this->m_headers = &m_impl->m_parser.headers();
	this->m_cookies = &m_impl->m_parser.cookies();
	this->m_parameters = &m_impl->m_parser.parameters();
}

template <concepts::connection Connection>
basic_request<Connection>::~basic_request()
{
	delete m_impl;
}

template <concepts::connection Connection>
template <typename Token>
auto basic_request<Connection>::wait(Token &&token)
	requires task_token_v<Token,status_enum>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		m_impl->wait(token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		m_impl->wait(error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::wait"
			);
		}
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
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [this, promise = std::move(promise),
					no_time_token, cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_wait (
						no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [this, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_wait (
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [this, no_time_token,
				original_token,  timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await m_impl->co_wait (
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
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return wait(token | 0ns);
	}
}

template <concepts::connection Connection>
int32_t basic_request<Connection>::path_match(std::string_view rule)
{
	return m_impl->m_parser.path_match(rule);
}

template <concepts::connection Connection>
method_enum basic_request<Connection>::method() const noexcept
{
	return m_impl->m_parser.method();
}

template <concepts::connection Connection>
version_enum basic_request<Connection>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <concepts::connection Connection>
std::string_view basic_request<Connection>::path() const noexcept
{
	return m_impl->m_parser.path();
}

template <concepts::connection Connection>
optional<typename basic_request<Connection>::value_t>
basic_request<Connection>::path_arg(const core_concepts::text_p<char> auto &key) const noexcept
{
	return m_impl->m_parser.path_arg(key);
}

template <concepts::connection Connection>
bool basic_request<Connection>::contains_path_arg(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto &args = m_impl->m_parser.path_args();
	return args.find(key) != args.end();
}

template <concepts::connection Connection>
optional<typename basic_request<Connection>::value_t>
basic_request<Connection>::path_arg(size_t index) const
{
	return m_impl->m_parser.path_arg(index);
}

template <concepts::connection Connection>
bool basic_request<Connection>::contains_path_arg(size_t index) const noexcept
{
	return index < m_impl->m_parser.path_args().size();
}

template <concepts::connection Connection>
const basic_request<Connection>::parameters_t&
basic_request<Connection>::path_args() const noexcept
{
	return m_impl->m_parser.path_args();
}

template <concepts::connection Connection>
template <typename Token>
auto basic_request<Connection>::read(const mutable_buffer &buf, Token &&token)
	requires task_token_v<Token,size_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->read(buf, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->read(buf, error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::read"
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
				return m_impl->co_read(buf, no_time_token.ec_,
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
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [this, buf, promise = std::move(promise),
					no_time_token, cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_read (
						buf, no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [this, buf, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_read (
						buf, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [this, buf, no_time_token,
				original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await m_impl->co_read (
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
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return read(buf, token | 0ns);
	}
}

template <concepts::connection Connection>
template <typename Token>
auto basic_request<Connection>::read(Token &&token)
	requires task_token_v<Token,std::string>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->read(token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->read(error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::read"
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
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [this, promise = std::move(promise),
					no_time_token, cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_read (
						no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [this, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_read (
						cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [this, no_time_token,
				original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await m_impl->co_read (
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
auto basic_request<Connection>::save_file(T &&opt, Token &&token)
	requires file_task_token_v<T,Token>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->save_file(std::forward<T>(opt), token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto sum = m_impl->save_file(std::forward<T>(opt), error);
		if( error )
		{
			system_error::loc_throw (
				error, "libgs::http_nt::basic_request::save_file"
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
				return m_impl->co_save_file(std::forward<T>(opt), no_time_token.ec_,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_save_file(std::forward<T>(opt),
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(get_executor(), [this, opt = std::forward<T>(opt), promise = std::move(promise),
					no_time_token, cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_save_file (
						opt, no_time_token.ec_, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(get_executor(), [this,
					opt = std::forward<T>(opt), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable noexcept -> awaitable<void>
				{
					promise->set_value(co_await m_impl->co_save_file (
						opt, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(get_executor(), [this, opt = std::forward<T>(opt),
				no_time_token, original_token, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable noexcept -> awaitable<void>
			{
				auto expected = co_await m_impl->co_save_file (
					opt, no_time_token.ec_, cancel_slot, timeout
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
		return save_file(std::forward<T>(opt), token | 0ns);
	}
}

template <concepts::connection Connection>
bool basic_request<Connection>::keep_alive() const noexcept
{
	return m_impl->m_parser.keep_alive();
}

template <concepts::connection Connection>
bool basic_request<Connection>::support_gzip() const noexcept
{
	return m_impl->m_parser.support_gzip();
}

template <concepts::connection Connection>
bool basic_request<Connection>::is_chunked() const noexcept
{
	if( version() < http_nt::version::v11 )
		return false;

	auto value = this->header(http_nt::header::transfer_encoding);
	return value and strtls::to_lower(**value) == "chunked";
}

template <concepts::connection Connection>
bool basic_request<Connection>::can_read_body() const noexcept
{
	return m_impl->m_parser.stage() == stage::body;
}

template <concepts::connection Connection>
bool basic_request<Connection>::is_eof() const noexcept
{
	return m_impl->m_parser.stage() == stage::finished;
}

template <concepts::connection Connection>
basic_request<Connection>::endpoint_t
basic_request<Connection>::remote_endpoint() const
{
	return connection().opt_helper().remote_endpoint();
}

template <concepts::connection Connection>
basic_request<Connection>::endpoint_t
basic_request<Connection>::local_endpoint() const
{
	return connection().opt_helper().local_endpoint();
}

template <concepts::connection Connection>
basic_request<Connection>::executor_t
basic_request<Connection>::get_executor() noexcept
{
	return connection().get_executor();
}

template <concepts::connection Connection>
basic_request<Connection> &basic_request<Connection>::cancel() noexcept
{
	connection().cancel();
	return *this;
}

template <concepts::connection Connection>
const basic_request<Connection>::connection_t&
basic_request<Connection>::connection() const noexcept
{
	return *m_impl->m_connection;
}

template <concepts::connection Connection>
basic_request<Connection>::connection_t&
basic_request<Connection>::connection() noexcept
{
	return *m_impl->m_connection;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_REQUEST_H
