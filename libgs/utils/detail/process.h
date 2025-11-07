
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_UTILS_DETAIL_PROCESS_H
#define LIBGS_UTILS_DETAIL_PROCESS_H

namespace libgs::utils { namespace detail
{

class LIBGS_UTILS_API process
{
	LIBGS_DISABLE_COPY_MOVE(process)

public:
	using executor_t = asio::any_io_executor;
	explicit process(const executor_t &exec);
	~process();

public:
	using path_t = std::filesystem::path;
	void set(const path_t &cmd, const std::vector<path_t> &args) const noexcept;
	void add_arg(const path_t &arg) const noexcept;

public:
	void start(std::error_code &error) const noexcept;
	void terminate() const noexcept;
	void kill() const noexcept;
	void detach() const noexcept;
	void cancel() const noexcept;

public:


	[[nodiscard]] sys_expected<int> join (
		std::error_code &error, const std::chrono::nanoseconds &timeout = {}
	) const noexcept;

	[[nodiscard]] awaitable<sys_expected<int>> co_join (
		std::error_code &error, asio::cancellation_slot cancel_slot,
		std::chrono::nanoseconds timeout = {}
	) const noexcept;

	template <typename Clock, typename Duration>
	[[nodiscard]] sys_expected<int> join(std::error_code &error,
		const std::chrono::time_point<Clock,Duration> &timeout) noexcept
	{
		auto no_wait = [this]
		{
			auto state = this->state();
			if( state == process_state::idle )
				return sys_unexpected(make_error_code(std::errc::no_such_process));

			else if( state == process_state::running )
				return sys_unexpected(make_error_code(std::errc::timed_out));

			else if( state == process_state::crashed )
				return sys_unexpected(make_error_code(std::errc::io_error));

			return exit_code();
		};
		if constexpr( Clock::is_steady )
		{
			auto now = Clock::now();
			if( now < timeout )
				return join(timeout - now);
			return no_wait();
		}
		else
		{
			auto now = Clock::now();
			if( now < timeout )
			{
				do {
					auto expected = join(timeout - now);
					if( expected.error().value() != std::errc::timed_out )
						return expected;
					now = Clock::now();
				}
				while( now < timeout );
				return no_wait();
			}
			return no_wait();
		}
	}

public:
	[[nodiscard]] io_expected write (
		std::error_code &error, const const_buffer &buf
	) const noexcept;

	void write_detach(const const_buffer &buf) const noexcept;

	[[nodiscard]] awaitable<io_expected> co_write (
		std::error_code &error, const const_buffer &buf,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout = {}
	) const noexcept;

	enum class read_channel {
		stdout, stderr
	};
	[[nodiscard]] io_expected read(read_channel channel,
		std::error_code &error, const mutable_buffer &buf
	) const noexcept;

	void read_detach(read_channel channel, const mutable_buffer &buf) const noexcept;

	[[nodiscard]] awaitable<io_expected> co_read (
		read_channel channel, std::error_code &error, const mutable_buffer &buf,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout = {}
	) const noexcept;

public:
	void set_work_path(path_t path) noexcept;
	void setenv(std::string_view key, libgs::value value) noexcept;
	void unsetenv(std::string_view key) noexcept;

public:
	[[nodiscard]] process_state state() const noexcept;
	[[nodiscard]] int exit_code() const noexcept;
	[[nodiscard]] uint64_t pid() const noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace detail

template <concepts::character CharT, concepts::exec Exec>
class LIBGS_UTILS_TAPI basic_process<CharT,Exec>::impl : public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using path_t = std::filesystem::path;

	explicit impl(const executor_t &exec) :
		m_exec(exec), m_detail(m_exec) {}

	impl() : m_exec(libgs::get_executor()),
		m_detail(m_exec) {}

public:
	void set(string_t cmd, args_t args)
	{
		std::vector<path_t> paths;
		for(auto &arg : args)
			paths.emplace_back(std::move(*arg));
		m_detail.set(std::move(cmd), paths);
	}

	template <typename...Args>
	void set(string_t cmd, Args&&...args)
	{
		m_detail.set(std::move(cmd), {});
		(
			m_detail.add_arg(std::format (
				l_str(char_t,"{}"), std::forward<Args>(args)
			)), ...
		);
	}

public:
	template <typename...Args>
	[[nodiscard]] sys_expected<> start(const string_t &cmd, Args&&...args) noexcept
	{
		set(cmd, std::forward<Args>(args)...);
		std::error_code error;
		m_detail.start(error);
		if( error )
			return sys_unexpected(error);
		return {};
	}

	[[nodiscard]] sys_expected<> start(const string_t &cmd, const args_t &args) noexcept
	{
		if( not cmd.empty() )
			set(cmd, args);

		std::error_code error;
		m_detail.start(error);
		if( error )
			return sys_unexpected(error);
		return {};
	}

	void terminate() noexcept {
		m_detail.terminate();
	}
	void kill() noexcept {
		m_detail.kill();
	}
	void detach() noexcept {
		m_detail.detach();
	}
	void cancel() noexcept {
		m_detail.cancel();
	}

public:
	template <typename Token>
	[[nodiscard]] auto join(Token &&token) noexcept
	{
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_error_code_token_v<Token> )
		{
			using namespace std::chrono_literals;
			return m_detail.join(token, 0ns);
		}
		else if constexpr( is_sync_opt_token_v<Token> )
		{
			using namespace std::chrono_literals;
			std::error_code error; LIBGS_UNUSED(error);
			return m_detail.join(error, 0ns);
		}
		else if constexpr( is_time_v<token_t> )
		{
			std::error_code error; LIBGS_UNUSED(error);
			return m_detail.join(error, token);
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
					return m_detail.co_join(ntoken.ec_,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
				else
				{
					std::error_code error;
					return m_detail.co_join(error,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
			}
			else if constexpr( is_use_future_v<nntoken_t> )
			{
				auto promise = std::make_shared<std::promise<sys_expected<int>>>();
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(), promise = std::move(promise),
						ntoken, cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						promise->set_value(co_await self->m_detail.co_join (
							ntoken.ec_, cancel_slot, timeout
						));
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(), promise = std::move(promise),
						cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						std::error_code error; LIBGS_UNUSED(error);
						promise->set_value(co_await self->m_detail.co_join (
							error, cancel_slot, timeout
						));
						co_return ;
					});
				}
				return promise->get_future();
			}
			else if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(m_exec, [self = this->shared_from_this(),
					ntoken, nntoken, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken)
				]() mutable -> awaitable<void>
				{
					auto expected = co_await self->m_detail.co_join(ntoken.ec_, cancel_slot, timeout);
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
				libgs::dispatch(m_exec, [self = this->shared_from_this(),
					ntoken, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken)
				]() mutable -> awaitable<void>
				{
					std::error_code error; LIBGS_UNUSED(error);
					auto expected = co_await self->m_detail.co_join(error, cancel_slot, timeout);
					expected
					.transform([&callback = ntoken](int code) {
						callback(error_code(), code);
					})
					.or_else([&callback = ntoken](const error_code &error) {
						callback(error, 255);
					});
					co_return ;
				});
			}
		}
		else
		{
			using namespace operators;
			return join(token | std::chrono::nanoseconds(0));
		}
	}

public:
	template <typename Token>
	[[nodiscard]] auto write(const const_buffer &buf, Token &&token) noexcept
	{
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_error_code_token_v<Token> )
			return m_detail.write(token, buf);

		else if constexpr( is_sync_opt_token_v<Token> )
		{
			error_code error; LIBGS_UNUSED(error);
			return write(buf, error);
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
					return m_detail.co_write(ntoken.ec_, buf,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
				else
				{
					std::error_code error; LIBGS_UNUSED(error);
					return m_detail.co_write(error, buf,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
			}
			else if constexpr( is_use_future_v<nntoken_t> )
			{
				auto buf_ptr = std::make_shared<std::string>(
					reinterpret_cast<const char*>(buf.data()), buf.size()
				);
				auto promise = std::make_shared<std::promise<io_expected>>();
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(),
						ntoken, buf = std::move(buf_ptr), promise = std::move(promise),
						cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						promise->set_value(self->m_detail.write (
							ntoken.ec_, {buf->data(), buf->size()}, cancel_slot, timeout
						));
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(),
						buf = std::move(buf_ptr), promise = std::move(promise),
						cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						std::error_code error; LIBGS_UNUSED(error);
						promise->set_value(self->m_detail.write (
							error, {buf->data(), buf->size()}, cancel_slot, timeout
						));
						co_return ;
					});
				}
				return promise->get_future();
			}
			else if constexpr( is_detached_v<nntoken_t> )
				m_detail.write_detach(buf);
			else
			{
				auto buf_ptr = std::make_shared<std::string>(
					reinterpret_cast<const char*>(buf.data()), buf.size()
				);
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(), ntoken, nntoken,
						buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
						cancel_slot = asio::get_associated_cancellation_slot(ntoken)
					]() mutable -> awaitable<void>
					{
						auto expected = co_await self->m_detail.co_write (
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
					libgs::dispatch(m_exec, [self = this->shared_from_this(), nntoken,
						buf = std::move(buf_ptr), timeout = get_associated_redirect_time(token),
						cancel_slot = asio::get_associated_cancellation_slot(ntoken)
					]() mutable -> awaitable<void>
					{
						std::error_code error; LIBGS_UNUSED(error);
						auto expected = co_await self->m_detail.co_write (
							error, buf, cancel_slot, timeout
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
			using namespace operators;
			return write(buf, token | std::chrono::nanoseconds(0));
		}
	}

public:
	using read_channel = detail::process::read_channel;

	template <read_channel Channel, typename Token>
	[[nodiscard]] auto read(const mutable_buffer &buf, Token &&token) noexcept
	{
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( is_error_code_token_v<Token> )
			return m_detail.read(Channel, token, buf);

		else if constexpr( is_sync_opt_token_v<Token> )
		{
			error_code error; LIBGS_UNUSED(error);
			return read<Channel>(buf, error);
		}
		else if constexpr( is_redirect_time_v<token_t> )
		{
			auto ntoken = unbound_redirect_time(token);
			using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

			decltype(auto) nntoken = unbound_token(ntoken);
			using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

			if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
			{
				if constexpr( is_redirect_error_v<ntoken_t> )
				{
					return m_detail.co_read(Channel, ntoken.ec_, buf,
						asio::get_associated_cancellation_slot(nntoken),
						get_associated_redirect_time(token)
					);
				}
				else
				{
					std::error_code error; LIBGS_UNUSED(error);
					return m_detail.co_read(Channel, error, buf,
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
					libgs::dispatch(m_exec, [self = this->shared_from_this(), promise = std::move(promise),
						ntoken, buf, cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						promise->set_value(self->m_detail.read (
							Channel, ntoken.ec_, buf, cancel_slot, timeout
						));
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(m_exec, [self = this->shared_from_this(), promise = std::move(promise),
						buf, cancel_slot = asio::get_associated_cancellation_slot(nntoken),
						timeout = get_associated_redirect_time(token)
					]() mutable -> awaitable<void>
					{
						std::error_code error; LIBGS_UNUSED(error);
						promise->set_value(self->m_detail.read (
							Channel, error, buf, cancel_slot, timeout
						));
						co_return ;
					});
				}
				return promise->get_future();
			}
			else if constexpr( is_detached_v<nntoken_t> )
				m_detail.write_detach(buf);

			else if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(m_exec, [self = this->shared_from_this(),
					ntoken, nntoken, buf, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(ntoken)
				]() mutable -> awaitable<void>
				{
					auto expected = co_await self->m_detail.co_read (
						Channel, ntoken.ec_, buf, cancel_slot, timeout
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
				libgs::dispatch(m_exec, [self = this->shared_from_this(),
					nntoken, buf, timeout = get_associated_redirect_time(token),
					cancel_slot = asio::get_associated_cancellation_slot(ntoken)
				]() mutable -> awaitable<void>
				{
					std::error_code error; LIBGS_UNUSED(error);
					auto expected = co_await self->m_detail.co_read (
						Channel, error, buf, cancel_slot, timeout
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
			return read<Channel>(buf, token | std::chrono::nanoseconds(0));
		}
	}

public:
	void set_work_path(path_t path) noexcept {
		m_detail.set_work_path(path);
	}
	void setenv(std::string_view key, libgs::value value) noexcept {
		m_detail.setenv(key, value);
	}
	void unsetenv(std::string_view key) noexcept {
		m_detail.unsetenv(key);
	}

public:
	[[nodiscard]] process_state state() const noexcept {
		return m_detail.state();
	}
	[[nodiscard]] int exit_code() const noexcept {
		return m_detail.exit_code();
	}
	[[nodiscard]] uint64_t pid() const noexcept {
		return m_detail.pid();
	}

private:
	executor_t m_exec {};
	detail::process m_detail;
};

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec>::basic_process(string_t cmd, args_t args)
	requires concepts::match_sched<io_executor_t,Exec> :
	m_impl(std::make_shared<impl>())
{
	m_impl->set(std::move(cmd), std::move(args));
}

template <concepts::character CharT, concepts::exec Exec>
template <typename...Args>
basic_process<CharT,Exec>::basic_process(string_t cmd, Args&&...args) requires
	concepts::match_sched<io_executor_t,Exec> and
	concepts::formatter<char_t,Args...> :
	m_impl(std::make_shared<impl>())
{
	m_impl->set(std::move(cmd), std::forward<Args>(args)...);
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::match_sched<Exec> Exec0>
basic_process<CharT,Exec>::basic_process(Exec0 &&exec, string_t cmd, args_t args) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<Exec0>(exec))))
{
	m_impl->set(std::move(cmd), std::move(args));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::match_sched<Exec> Exec0, typename...Args>
basic_process<CharT,Exec>::basic_process(Exec0 &&exec, string_t cmd, Args&&...args)
	requires concepts::formatter<char_t,Args...> :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<Exec0>(exec))))
{
	m_impl->set(std::move(cmd), std::forward<Args>(args)...);
}

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec>::basic_process(basic_process &&other) noexcept :
	m_impl(std::move(other.m_impl))
{
	other.m_impl = std::make_shared<impl>(m_impl->m_exec);
}

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec> &basic_process<CharT,Exec>::operator=(basic_process &&other) noexcept
{
	if( &other == this )
		return *this;

	m_impl = std::move(other.m_impl);
	other.m_impl = new impl(m_impl->m_exec);
	return *this;
}

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec>::~basic_process() = default;

template <concepts::character CharT, concepts::exec Exec>
template <typename...Args>
sys_expected<> basic_process<CharT,Exec>::start(const string_t &cmd, Args&&...args) noexcept
	requires concepts::formatter<char_t,Args...>
{
	return m_impl->start(cmd, std::forward<Args>(args)...);
}

template <concepts::character CharT, concepts::exec Exec>
sys_expected<> basic_process<CharT,Exec>::start(const string_t &cmd, const args_t &args) noexcept
{
	return m_impl->start(cmd, args);
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::terminate() noexcept
{
	m_impl->terminate();
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::kill() noexcept
{
	m_impl->kill();
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::detach() noexcept
{
	m_impl->detach();
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::cancel() noexcept
{
	m_impl->cancel();
}

template <concepts::character CharT, concepts::exec Exec>
template <typename Token>
auto basic_process<CharT,Exec>::join(Token &&token) noexcept
	requires join_token_v<Token>
{
	return m_impl->join(std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::tf_opt_token<error_code,size_t> Token>
auto basic_process<CharT,Exec>::write(const const_buffer &buf, Token &&token) noexcept
{
	return m_impl->write(buf, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::tf_opt_token<error_code,size_t> Token>
auto basic_process<CharT,Exec>::read(const mutable_buffer &buf, Token &&token) noexcept
{
	return m_impl->template read<impl::read_channel::stdout>
		(buf, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::tf_opt_token<error_code,size_t> Token>
auto basic_process<CharT,Exec>::read_stderr(const mutable_buffer &buf, Token &&token) noexcept
{
	return m_impl->template read<impl::read_channel::stderr>
		(buf, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <typename Token>
auto basic_process<CharT,Exec>::run(const string_t &cmd, const args_t &args, Token &&token)
	noexcept requires task_token_v<Token>
{
	using token_t = std::remove_cvref_t<Token>;
	auto exp0 = start(cmd, args);
	if( exp0 )
	{
		if constexpr( is_detached_v<token_t> )
		{
			detach();
			return sys_expected<int>(0);
		}
		else
			return join(std::forward<Token>(token));
	}
	if constexpr( is_error_code_token_v<Token> )
	{
		token = exp0.error();
		return sys_expected<int>(sys_unexpected(token));
	}
	else if constexpr( is_sync_opt_token_v<Token> or is_time_v<token_t> )
		return sys_expected<int>(sys_unexpected(exp0.error()));

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;
		using nntoken_t = std::remove_cvref_t<decltype(unbound_token(ntoken))>;

		if constexpr( is_redirect_error_v<ntoken_t> )
			ntoken.ec_ = exp0.error();

		if constexpr( is_detached_v<nntoken_t> )
			return sys_expected<int>(0);

		else if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			return [error = exp0.error()]() -> awaitable<sys_expected<int>> {
				co_return sys_expected<int>(sys_unexpected(error));
			}();
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			std::promise<sys_expected<int>> promise;
			promise.set_value(sys_unexpected(exp0.error()));
			return promise.get_future();
		}
	}
	else
	{
		if constexpr( is_redirect_error_v<token_t> )
			token.ec_ = exp0.error();

		using ntoken_t = std::remove_cvref_t<decltype(unbound_token(token))>;
		if constexpr( is_detached_v<ntoken_t> )
			return sys_expected<int>(0);

		else if constexpr( is_use_awaitable_v<ntoken_t> or is_deferred_v<ntoken_t> )
		{
			return [error = exp0.error()]() -> awaitable<sys_expected<int>> {
				co_return sys_expected<int>(sys_unexpected(error));
			}();
		}
		else if constexpr( is_use_future_v<ntoken_t> )
		{
			std::promise<sys_expected<int>> promise;
			promise.set_value(sys_unexpected(exp0.error()));
			return promise.get_future();
		}
	}
}

template <concepts::character CharT, concepts::exec Exec>
template <typename Token>
auto basic_process<CharT,Exec>::run(const string_t &cmd, Token &&token) noexcept
	requires task_token_v<Token>
{
	return run(cmd, {}, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <typename Token>
auto basic_process<CharT,Exec>::run(Token &&token) noexcept
	requires task_token_v<Token>
{
	return run({}, {}, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::set_work_path(path_t path) noexcept
{
	m_impl->set_work_path(path);
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::setenv(std::string_view key, libgs::value value) noexcept
{
	m_impl->setenv(key, value);
}

template <concepts::character CharT, concepts::exec Exec>
void basic_process<CharT,Exec>::unsetenv(std::string_view key) noexcept
{
	m_impl->unsetenv(key);
}

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec>::state_t basic_process<CharT,Exec>::state() const noexcept
{
	return m_impl->state();
}

template <concepts::character CharT, concepts::exec Exec>
int basic_process<CharT,Exec>::exit_code() const noexcept
{
	return m_impl->exit_code();
}

template <concepts::character CharT, concepts::exec Exec>
uint64_t basic_process<CharT,Exec>::pid() const noexcept
{
	return m_impl->pid();
}

template <concepts::character CharT, concepts::exec Exec>
basic_process<CharT,Exec>::executor_t
basic_process<CharT,Exec>::get_executor() const noexcept
{
	return m_impl->get_executor();
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::opt_token<error_code> Token>
auto basic_process<CharT,Exec>::exec(const string_t &cmd, const args_t &args, Token &&token) noexcept
{
	return exec(libgs::get_executor(), cmd, args, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::opt_token<error_code> Token>
auto basic_process<CharT,Exec>::exec(const string_t &cmd, Token &&token) noexcept
{
	return exec(libgs::get_executor(), cmd, {}, std::forward<Token>(token));
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::match_sched<Exec> Exec0, concepts::opt_token<error_code> Token>
auto basic_process<CharT,Exec>::exec(Exec0 &&exec, const string_t &cmd, const args_t &args, Token &&token) noexcept
{
	process obj(exec, cmd, args);
	auto exp0 = obj.start();

	using expected_t = sys_expected<int>;
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_sync_opt_token_v<Token> )
	{
		if( exp0 )
			return obj.join(token);
		return expected_t(exp0.error());
	}
	else if constexpr( is_use_awaitable_v<token_t> or is_deferred_v<token_t> )
	{
		if( exp0 )
			return obj.join(token);

		return [error = exp0.error()]() -> awaitable<expected_t> {
			co_return error;
		}();
	}
	else if constexpr( is_use_future_v<token_t> )
	{
		if( exp0 )
			return obj.join(token);

		std::promise<expected_t> promise;
		promise.set_value({exp0.error()});
		return promise.get_future();
	}
	else if constexpr( is_detached_v<token_t> )
	{
		if( exp0 )
			obj.detach();
	}
	else
	{
		if( exp0 )
			return obj.join(std::forward<Token>(token));

		libgs::dispatch(std::forward<Exec0>(exec),
		[callback = std::forward<Token>(token), error = exp0.error()]{
			callback(error);
		});
	}
}

template <concepts::character CharT, concepts::exec Exec>
template <concepts::match_sched<Exec> Exec0, concepts::opt_token<error_code> Token>
auto basic_process<CharT,Exec>::exec(Exec0 &&exec, const string_t &cmd, Token &&token) noexcept
{
	return exec(cmd, {}, std::forward<Token>(token));
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_PROCESS_H