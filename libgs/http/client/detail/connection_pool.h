
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H
#define LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H

#include <libgs/coro/utils.h>
#include <libgs/core/shared_mutex.h>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace libgs::http
{

template <concepts::stream Stream>
default_stream_constructor<Stream>::socket_t
default_stream_constructor<Stream>::make(auto &&exec)
{
	return socket_t(get_executor_helper (
		std::forward<decltype(exec)>(exec))
	);
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
class LIBGS_HTTP_TAPI basic_connection_pool<Stream,Constructor>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using opt_helper_t = connection_t::opt_helper_t;

public:
	explicit impl(const auto &exec, config_t config) :
		m_config(std::move(config)), m_exec(exec) {}

	impl(config_t config) :
		m_config(std::move(config)),
		m_exec(libgs::get_executor()) {}

public:
	[[nodiscard]] sys_expected<connection_t> get(const endpoint_t &ep) noexcept
	{
		auto connection = _get(ep);
		if( not connection or connection->peek() )
			return connection;

		std::error_code error;
		connection->opt_helper().connect(ep, error);

		if( error )
			return sys_unexpected(error);
		return connection;
	}

	[[nodiscard]] awaitable<sys_expected<connection_t>> co_get(const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;

		if( timeout == 0ns )
		{
			auto connection = _get(ep, timeout);
			if( not connection or connection->peek() )
				co_return connection;

			auto &sock_helper = connection->opt_helper();
			add_task(&sock_helper);

			std::error_code error;
			// co_await sock_helper.connect(ep, use_awaitable | cancel_slot | error);
			co_await sock_helper.connect(ep, use_awaitable | error);
			erase_task(&sock_helper);

			if( error )
				co_return sys_unexpected(error);
			co_return connection;
		}
		auto start_time = std::chrono::steady_clock::now();
		auto connection = _get(ep, timeout);
		if( not connection or connection->peek() )
			co_return connection;

		auto end_time = std::chrono::steady_clock::now();
		auto diff_time = end_time - start_time;

		if( diff_time >= timeout )
		{
			co_return sys_unexpected (
				make_error_code(errc::timed_out)
			);
		}
		auto &sock_helper = connection->opt_helper();
		add_task(&sock_helper);

		std::error_code error;
		auto var = co_await (
			sock_helper.connect(ep, use_awaitable | cancel_slot | error) or
			coro::sleep_for(sock_helper.get_executor(), diff_time)
		);
		erase_task(&sock_helper);

		if( var.index() == 0 )
			co_return connection;

		else if( not std::get<1>(var) )
			co_return sys_unexpected(make_error_code(errc::timed_out));

		co_return sys_unexpected(std::get<1>(var));
	}

	[[nodiscard]] sys_expected<connection_t> co_get(std::error_code &error, const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get (
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] sys_expected<connection_t> try_get(const endpoint_t &ep) noexcept
	{
		auto connection = _try_get(ep);
		if( not connection or connection->peek() )
			return connection;

		std::error_code error;
		connection->opt_helper().connect(ep, error);

		if( error )
			return sys_unexpected(error);
		return connection;
	}

	[[nodiscard]] awaitable<sys_expected<connection_t>> co_try_get(const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto connection = _try_get(ep);
		if( not connection or connection->peek() )
			co_return connection;

		auto &sock_helper = connection->opt_helper();
		add_task(&sock_helper);

		using namespace libgs::operators;
		using namespace std::chrono_literals;

		std::error_code error;
		auto task = sock_helper.connect(ep, use_awaitable | cancel_slot | error);

		if( timeout == 0ns )
		{
			co_await std::move(task);
			erase_task(&sock_helper);

			if( error )
				co_return sys_unexpected(error);
			co_return connection;
		}
		auto var = co_await(std::move(task) or
			coro::sleep_for(sock_helper.get_executor(), timeout)
		);
		erase_task(&sock_helper);

		if( var.index() == 0 )
			co_return connection;

		else if( not std::get<1>(var) )
			co_return sys_unexpected(make_error_code(errc::timed_out));

		co_return sys_unexpected(std::get<1>(var));
	}

	[[nodiscard]] sys_expected<connection_t> co_try_get(std::error_code &error, const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get (
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] bool try_increment() noexcept
	{
		auto expected = m_counter.load(std::memory_order_relaxed);
		while( expected < m_config.max_count )
		{
			if( m_counter.compare_exchange_weak
				(expected, expected + 1, std::memory_order_relaxed) )
    		    return true;
		}
		return false;
	}

	void emplace(socket_t &&socket)
	{
		opt_helper_t opt(socket);
		spin_shared_unique_lock locker(m_pool_mutex);
		m_pool
		.emplace(opt.remote_endpoint(), std::list<socket_t>())
		.first->second
		.emplace_back(std::move(socket));
	}

	[[nodiscard]] size_t count() const noexcept {
		return m_counter;
	}

private:
	[[nodiscard]] sys_expected<connection_t> _get
	(const endpoint_t &ep, const std::chrono::nanoseconds &rtime) noexcept
	{
		for(;;)
		{
			auto socket = get_socket(ep);
			if( socket )
			{
				opt_helper_t opt_helper(*socket);
				if( opt_helper.is_open() and opt_helper.message_peek() )
					return make(std::move(*socket));

				opt_helper.close();
				--m_counter;
				continue;
			}
			using namespace std::chrono_literals;
			if( rtime == 0ns )
				increment();

			else if( not increment(rtime) )
			{
				return sys_unexpected (
					make_error_code(errc::no_memory)
				);
			}
			break;
		}
		return make(constructor_t::make(m_exec));
	}

	[[nodiscard]] sys_expected<connection_t> _try_get(const endpoint_t &ep) noexcept
	{
		for(;;)
		{
			auto socket = try_get_socket(ep);
			if( socket )
			{
				opt_helper_t opt_helper(*socket);
				if( opt_helper.is_open() and opt_helper.message_peek() )
					return make(std::move(*socket));

				opt_helper.close();
				--m_counter;
				continue;
			}
			else if( not try_increment() )
			{
				return sys_unexpected (
					make_error_code(errc::no_memory)
				);
			}
			break;
		}
		return make(constructor_t::make(m_exec));
	}

private:
	[[nodiscard]] optional<socket_t> get_socket(const endpoint_t &ep) noexcept
	{
		spin_shared_shared_lock locker(m_pool_mutex);
		auto it = m_pool.find(ep);
		if( it == m_pool.end() )
			return {};

		else if( it->second.empty() )
		{
			locker.unlock();
			m_pool_mutex.lock();
			m_pool.erase(ep);
			m_pool_mutex.unlock();
			return {};
		}
		auto socket = std::move(it->second.front());
		it->second.pop_front();
		return socket;
	}

	[[nodiscard]] sys_expected<connection_t> make(socket_t &&socket) noexcept
	{
		return connection_t(std::move(socket),
		[self = this->shared_from_this()](socket_t &&sock) mutable
		{
			if( not self->m_valid )
				return ;

			if( opt_helper_t opt_helper(sock); opt_helper.is_open() and opt_helper.message_peek() )
				self->emplace(std::move(sock));
			else
				--self->m_counter;
		});
	}

private:
	void increment() noexcept
	{
		std::unique_lock locker(m_mutex);
		m_condition.wait(locker, [this]{
			return m_counter < m_config.max_count;
		});
		++m_counter;
		m_condition.notify_one();
	}

	[[nodiscard]] bool increment(const std::chrono::nanoseconds &rtime) noexcept
	{
		std::unique_lock locker(m_mutex);
		bool cond_met = m_condition.wait_for(locker, rtime, [this]{
			return m_counter < m_config.max_count;
		});
		if( not cond_met )
			return false;

		++m_counter;
		m_condition.notify_one();
		return true;
	}

private:
	void add_task(const opt_helper_t *obj)
	{
		m_tasks_mutex.lock();
		m_curr_tasks.emplace(obj);
		m_tasks_mutex.unlock();
	}

	void erase_task(const opt_helper_t *obj)
	{
		m_tasks_mutex.lock();
		m_curr_tasks.erase(obj);
		m_tasks_mutex.unlock();
	}

public:
	const config_t m_config {};
	bool m_valid = true;

	std::unordered_map<endpoint_t,std::list<socket_t>> m_pool {};
	std::atomic_size_t m_counter {0};

	spin_shared_mutex m_pool_mutex {};
	std::condition_variable m_condition {};
	std::mutex m_mutex {};

	std::unordered_set<const opt_helper_t*> m_curr_tasks {};
	spin_mutex m_tasks_mutex {};
	executor_t m_exec {};
};

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::basic_connection_pool(config_t config) requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>(std::move(config)))
{

}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::basic_connection_pool
(core_concepts::match_sched<executor_t> auto &&exec, config_t config) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec)), std::move(config)))
{

}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::~basic_connection_pool()
{
	m_impl->m_valid = false;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::basic_connection_pool(basic_connection_pool &&other) noexcept :
	m_impl(std::move(other.m_impl))
{
	other.m_impl = std::make_shared<impl>();
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>&
basic_connection_pool<Stream,Constructor>::operator=(basic_connection_pool &&other) noexcept
{
	if( this == &other )
		return *this;
	m_impl = std::move(other.m_impl);
	other.m_impl = std::make_shared<impl>();
	return *this;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,connection_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->get(ep)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->get(ep);

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
				return m_impl->co_get(ntoken.ec_, ep,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_get(ep,
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
				libgs::dispatch(m_impl->m_connection.get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						ntoken.ec_, ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection.get_executor(), [
					impl = m_impl->shared_from_this(), ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_connection.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken, ep,
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					ntoken.ec_, ep, cancel_slot, timeout
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
			libgs::dispatch(m_impl->m_connection.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken, ep,
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					ep, cancel_slot, timeout
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
		return get(ep, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::try_get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,connection_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->try_get(ep)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->try_get(ep);

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
				return m_impl->co_try_get(ntoken.ec_, ep,
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_try_get(ep,
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
				libgs::dispatch(m_impl->m_connection.get_executor(), [
					impl = m_impl->shared_from_this(), ntoken, ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						ntoken.ec_, ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_connection.get_executor(), [
					impl = m_impl->shared_from_this(), ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_connection.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken, ep,
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					ntoken.ec_, ep, cancel_slot, timeout
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
			libgs::dispatch(m_impl->m_connection.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken, ep,
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					ep, cancel_slot, timeout
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
		return try_get(ep, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
bool basic_connection_pool<Stream,Constructor>::emplace(socket_t &socket)
{
	if( m_impl->try_increment() )
	{
		m_impl->emplace(std::move(socket));
		return true;
	}
	return false;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
void basic_connection_pool<Stream,Constructor>::operator<<(socket_t &socket)
{
	emplace(socket);
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>&
basic_connection_pool<Stream,Constructor>::cancel() noexcept
{
	m_impl->m_tasks_mutex.lock();
	for(auto &task : m_impl->m_curr_tasks)
		task->cancel();
	m_impl->m_tasks_mutex.unlock();
	return *this;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::executor_t
basic_connection_pool<Stream,Constructor>::get_executor() noexcept
{
	return m_impl->m_exec;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::config_t
basic_connection_pool<Stream,Constructor>::config() const noexcept
{
	return m_impl->m_config;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
size_t basic_connection_pool<Stream,Constructor>::count() const noexcept
{
	return m_impl->count();
}

} //namespace libgs::http

#if LIBGS_OPENSSL_SUPPORT
namespace libgs::http { namespace detail
{

[[nodiscard]] LIBGS_HTTP_API
asio::ssl::context &default_ssl_context() noexcept;

} //namespace detail

template <core_concepts::exec Exec>
default_stream_constructor<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::socket_t
default_stream_constructor<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::make(auto &&exec)
{
	using next_layer_t = socket_t::next_layer_type;
	using next_layer_constructor_t = default_stream_constructor<next_layer_t>;

	auto next_layer = next_layer_constructor_t::make (
		std::forward<decltype(exec)>(exec)
	);
	return socket_t(std::move(next_layer),
		detail::default_ssl_context()
	);
}

} //namespace libgs::http

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H