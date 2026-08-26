
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_CONNECTION_POOL_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_CONNECTION_POOL_H

#include <libgs/coro/utils.h>
#include <libgs/core/shared_mutex.h>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace libgs::http_nt
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
class LIBGS_HTTP_NT_TAPI basic_connection_pool<Stream,Constructor>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	using opt_helper_t = connection_t::opt_helper_t;
	using resolver_t = asio::ip::basic_resolver<protocol_t>;

public:
	explicit impl(const auto &exec, const config_t &config) :
		m_config(config), m_exec(exec) {}

	impl(config_t config) :
		m_config(config), m_exec(libgs::get_executor()) {}

public:
	[[nodiscard]] con_expected_t get
	(const core_concepts::text_p<char> auto &host, const value &service) noexcept
	{
		resolver_t resolver(m_exec);
		error_code error {};

		auto results = resolver.resolve (
			strtls::to_view(host), *service, error
		);
		if( error )
			return sys_unexpected(error);

		else if( results.empty() )
		{
			return sys_unexpected (
				make_error_code(errc::not_found)
			);
		}
		return get(results);
	}

	[[nodiscard]] awaitable<con_expected_t> co_get(
		const core_concepts::text_p<char> auto &host, const value &service,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		auto task = libgs::dispatch(m_exec, [&]() mutable -> awaitable<void>
		{
			resolver_t resolver(m_exec);
			error_code error {};

			auto results = co_await resolver.async_resolve (
				strtls::to_view(host), *service, use_awaitable | cancel_slot | error
			);
			if( error )
			{
				result.despair(error);
				co_return ;
			}
			else if( results.empty() )
			{
				result.despair(make_error_code(errc::not_found));
				co_return ;
			}
			result = co_await co_get(results, cancel_slot, 0ns);
			co_return ;
		},
		use_awaitable);

		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					result.despair(make_error_code(errc::timed_out));
				else
					result.despair(std::get<1>(var));
			}
		}
		co_return result;
	}

	[[nodiscard]] con_expected_t co_get(std::error_code &error,
		const core_concepts::text_p<char> auto &host, const value &service,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get (
			host, service, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] con_expected_t get(const dns_results &eps) noexcept
	{
		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		if( eps.empty() )
			return result;

		for(auto &ep : eps)
		{
			if( auto connection = get_connection(ep); connection )
				return std::move(*connection);
		}
		for(auto &ep : eps)
		{
			result = get(ep);
			if( result )
				return result;
		}
		return result;
	}

	[[nodiscard]] awaitable<con_expected_t> co_get(const dns_results &eps,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		if( eps.empty() )
			co_return result;

		auto task = libgs::dispatch(m_exec, [&]() mutable -> awaitable<void>
		{
			for(auto &ep : eps)
			{
				if( auto connection = get_connection(ep); connection )
				{
					result = std::move(*connection);
					co_return ;
				}
			}
			for(auto &ep : eps)
			{
				result = co_await co_get(ep, cancel_slot, 0ns);
				if( result )
					co_return ;
			}
		},
		use_awaitable);

		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					result.despair(make_error_code(errc::timed_out));
				else
					result.despair(std::get<1>(var));
			}
		}
		co_return result;
	}

	[[nodiscard]] con_expected_t co_get(std::error_code &error, const dns_results &eps,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get (
			eps, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] con_expected_t get(const endpoint_t &ep) noexcept
	{
		using namespace std::chrono_literals;
		auto connection = _get(ep, 0ns);

		if( not connection or connection->peek() )
			return connection;

		std::error_code error;
		connection->opt_helper().connect(ep, error);

		if( error )
			return sys_unexpected(error);
		return finish_connect(std::move(connection));
	}

	[[nodiscard]] awaitable<con_expected_t> co_get(const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;

		if( timeout == 0ns )
		{
			auto connection = co_await co_acquire(ep, cancel_slot, timeout);
			if( not connection or connection->peek() )
				co_return connection;

			auto &sock_helper = connection->opt_helper();
			add_task(&sock_helper);

			std::error_code error;
			co_await sock_helper.connect(ep, use_awaitable | cancel_slot | error);
			erase_task(&sock_helper);

			if( error )
				co_return sys_unexpected(error);
			co_return finish_connect(std::move(connection));
		}
		auto start_time = std::chrono::steady_clock::now();
		auto connection = co_await co_acquire(ep, cancel_slot, timeout);

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
			coro::sleep_for(sock_helper.get_executor(), timeout - diff_time)
		);
		erase_task(&sock_helper);

		if( var.index() == 0 )
		{
			if( error )
				co_return sys_unexpected(error);
			co_return finish_connect(std::move(connection));
		}

		else if( not std::get<1>(var) )
			co_return sys_unexpected(make_error_code(errc::timed_out));

		co_return sys_unexpected(std::get<1>(var));
	}

	[[nodiscard]] con_expected_t co_get(std::error_code &error, const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get (
			ep, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] con_expected_t try_get
	(const core_concepts::text_p<char> auto &host, const value &service) noexcept
	{
		resolver_t resolver(m_exec);
		error_code error {};

		auto results = resolver.resolve (
			strtls::to_view(host), *service, error
		);
		if( error )
			return sys_unexpected(error);

		else if( results.empty() )
		{
			return sys_unexpected (
				make_error_code(errc::not_found)
			);
		}
		return try_get(results);
	}

	[[nodiscard]] awaitable<con_expected_t> co_try_get(
		const core_concepts::text_p<char> auto &host, const value &service,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		auto task = libgs::dispatch(m_exec, [&]() mutable -> awaitable<void>
		{
			resolver_t resolver(m_exec);
			error_code error {};

			auto results = co_await resolver.async_resolve (
				strtls::to_view(host), *service, use_awaitable | cancel_slot | error
			);
			if( error )
			{
				result.despair(error);
				co_return ;
			}
			else if( results.empty() )
			{
				result.despair(make_error_code(errc::not_found));
				co_return ;
			}
			result = co_await co_try_get(results, cancel_slot, 0ns);
			co_return ;
		},
		use_awaitable);

		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					result.despair(make_error_code(errc::timed_out));
				else
					result.despair(std::get<1>(var));
			}
		}
		co_return result;
	}

	[[nodiscard]] con_expected_t co_try_get(std::error_code &error,
		const core_concepts::text_p<char> auto &host, const value &service,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_try_get (
			host, service, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] con_expected_t try_get(const dns_results &eps) noexcept
	{
		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		if( eps.empty() )
			return result;

		for(auto &ep : eps)
		{
			if( auto connection = get_connection(ep); connection )
				return std::move(*connection);
		}
		for(auto &ep : eps)
		{
			result = try_get(ep);
			if( result )
				return result;
		}
		return result;
	}

	[[nodiscard]] awaitable<con_expected_t> co_try_get(const dns_results &eps,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace std::chrono_literals;
		con_expected_t result (
			sys_unexpected(make_error_code(errc::invalid_argument))
		);
		if( eps.empty() )
			co_return result;

		auto task = libgs::dispatch(m_exec, [&]() mutable -> awaitable<void>
		{
			for(auto &ep : eps)
			{
				if( auto connection = get_connection(ep); connection )
				{
					result = std::move(*connection);
					co_return ;
				}
			}
			for(auto &ep : eps)
			{
				result = co_await co_try_get(ep, cancel_slot, 0ns);
				if( result )
					co_return ;
			}
		},
		use_awaitable);

		if( timeout == 0ns )
			co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_exec, timeout)
			);
			if( var.index() == 1 )
			{
				if( not std::get<1>(var) )
					result.despair(make_error_code(errc::timed_out));
				else
					result.despair(std::get<1>(var));
			}
		}
		co_return result;
	}

	[[nodiscard]] con_expected_t co_try_get(std::error_code &error, const dns_results &eps,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_try_get (
			eps, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] con_expected_t try_get(const endpoint_t &ep) noexcept
	{
		auto connection = _try_get(ep);
		if( not connection or connection->peek() )
			return connection;

		std::error_code error;
		connection->opt_helper().connect(ep, error);

		if( error )
			return sys_unexpected(error);
		return finish_connect(std::move(connection));
	}

	[[nodiscard]] awaitable<con_expected_t> co_try_get(const endpoint_t &ep,
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
			co_return finish_connect(std::move(connection));
		}
		auto var = co_await(std::move(task) or
			coro::sleep_for(sock_helper.get_executor(), timeout)
		);
		erase_task(&sock_helper);

		if( var.index() == 0 )
		{
			if( error )
				co_return sys_unexpected(error);
			co_return finish_connect(std::move(connection));
		}

		else if( not std::get<1>(var) )
			co_return sys_unexpected(make_error_code(errc::timed_out));

		co_return sys_unexpected(std::get<1>(var));
	}

	[[nodiscard]] con_expected_t co_try_get(std::error_code &error, const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_try_get (
			ep, std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	[[nodiscard]] bool try_increment() noexcept
	{
		std::lock_guard locker(m_mutex); LIBGS_UNUSED(locker);
		if( m_counter.load(std::memory_order_relaxed) >= m_config.max_count )
			return false;

		m_counter.fetch_add(1, std::memory_order_relaxed);
		return true;
	}

	bool emplace(socket_t &&socket, bool enable_no_delay = false)
	{
		opt_helper_t opt(socket);
		if( enable_no_delay )
		{
			error_code error {};
			if constexpr( std::is_same_v<protocol_t,asio::ip::tcp> )
				opt.set_option(asio::ip::tcp::no_delay(true), error);

			if( error )
			{
				decrement();
				return false;
			}
		}
		std::lock_guard locker(m_mutex);
		if( not m_valid.load(std::memory_order_acquire) )
		{
			auto before = m_counter.fetch_sub(1, std::memory_order_relaxed);
			assert(before > 0);
			LIBGS_UNUSED(before);

			notify_one_locked();
			return false;
		}
		m_pool
		.emplace(opt.remote_endpoint(), std::list<socket_t>())
		.first->second
		.emplace_back(std::move(socket));

		notify_one_locked();
		return true;
	}

	[[nodiscard]] size_t count() const noexcept {
		return m_counter;
	}

private:
	[[nodiscard]] con_expected_t _get
	(const endpoint_t &ep, const std::chrono::nanoseconds &rtime) noexcept
	{
		const auto deadline = std::chrono::steady_clock::now() + rtime;
		for(;;)
		{
			if( not m_valid.load(std::memory_order_acquire) )
				return sys_unexpected(make_error_code(errc::operation_aborted));

			if( auto connection = get_connection(ep); connection )
				return std::move(*connection);

			if( try_increment() )
				return make(constructor_t::make(m_exec));

			std::unique_lock locker(m_mutex);
			auto available = [this, &ep]
			{
				auto it = m_pool.find(ep);
				return not m_valid.load(std::memory_order_acquire) or
					m_counter.load(std::memory_order_relaxed) < m_config.max_count or
					(it != m_pool.end() and not it->second.empty());
			};
			using namespace std::chrono_literals;
			if( rtime == 0ns )
				m_condition.wait(locker, available);

			else if( not m_condition.wait_until(locker, deadline, available) )
			{
				return sys_unexpected (
					make_error_code(errc::timed_out)
				);
			}
			if( not m_valid.load(std::memory_order_acquire) )
			{
				return sys_unexpected (
					make_error_code(errc::operation_aborted)
				);
			}
		}
	}

	[[nodiscard]] con_expected_t _try_get(const endpoint_t &ep) noexcept
	{
		for(;;)
		{
			if( not m_valid.load(std::memory_order_acquire) )
				return sys_unexpected(make_error_code(errc::operation_aborted));

			if( auto connection = get_connection(ep); connection )
				return std::move(*connection);

			if( not try_increment() )
			{
				return sys_unexpected (
					make_error_code(errc::no_memory)
				);
			}
			return make(constructor_t::make(m_exec));
		}
	}

private:
	[[nodiscard]] awaitable<con_expected_t> co_acquire(const endpoint_t &ep,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;

		const auto deadline = timeout == 0ns ?
			asio::steady_timer::time_point::max() :
			std::chrono::steady_clock::now() + timeout;
		for(;;)
		{
			auto connection = _try_get(ep);
			if( connection or connection.error() != errc::no_memory )
				co_return connection;

			auto waiter = std::make_shared<asio::steady_timer>(m_exec);
			waiter->expires_at(deadline);
			{
				std::lock_guard locker(m_mutex);
				if( not m_valid.load(std::memory_order_acquire) )
					co_return sys_unexpected(make_error_code(errc::operation_aborted));

				auto it = m_pool.find(ep);
				if( m_counter.load(std::memory_order_relaxed) < m_config.max_count or
					(it != m_pool.end() and not it->second.empty()) )
					continue;
				m_waiters.emplace_back(waiter);
			}
			error_code error {};
			co_await waiter->async_wait(use_awaitable | cancel_slot | error);

			bool notified = true;
			{
				std::lock_guard locker(m_mutex);
				auto it = std::find(m_waiters.begin(), m_waiters.end(), waiter);
				if( it != m_waiters.end() )
				{
					m_waiters.erase(it);
					notified = false;
				}
			}
			if( notified )
				continue;
			if( not error )
				co_return sys_unexpected(make_error_code(errc::timed_out));
			co_return sys_unexpected(error);
		}
	}

	[[nodiscard]] optional<connection_t> get_connection(const endpoint_t &ep) noexcept
	{
		for(;;)
		{
			auto socket = get_socket(ep);
			if( not socket )
				return {};

			opt_helper_t opt_helper(*socket);
			if( opt_helper.is_open() and opt_helper.message_peek() )
				return make(std::move(*socket));

			opt_helper.close();
			decrement();
		}
	}

	[[nodiscard]] optional<socket_t> get_socket(const endpoint_t &ep) noexcept
	{
		std::lock_guard locker(m_mutex);
		auto it = m_pool.find(ep);

		if( it == m_pool.end() )
			return {};

		if( it->second.empty() )
		{
			m_pool.erase(it);
			return {};
		}
		auto socket = std::move(it->second.front());
		it->second.pop_front();

		if( it->second.empty() )
			m_pool.erase(it);
		return socket;
	}

	[[nodiscard]] connection_t make(socket_t &&socket) noexcept
	{
		return connection_t(std::move(socket),
		[self = this->shared_from_this()](socket_t &&sock) mutable
		{
			if( not self->m_valid.load(std::memory_order_acquire) )
				return ;

			if( opt_helper_t opt_helper(sock); opt_helper.is_open() and opt_helper.message_peek() )
				LIBGS_UNUSED(self->emplace(std::move(sock)));
			else
				self->decrement();
		});
	}

private:
	[[nodiscard]] con_expected_t finish_connect(con_expected_t connection) noexcept
	{
		if( not connection )
			return connection;

		error_code error;
		if constexpr( std::is_same_v<protocol_t,asio::ip::tcp> )
		{
			connection->opt_helper().set_option (
				asio::ip::tcp::no_delay(true), error
			);
		}
		if( error )
		{
			connection->opt_helper().close();
			return sys_unexpected(error);
		}
		return connection;
	}

	void decrement() noexcept
	{
		std::lock_guard locker(m_mutex); LIBGS_UNUSED(locker);
		auto before = m_counter.fetch_sub(1, std::memory_order_relaxed);

		assert(before > 0);
		LIBGS_UNUSED(before);

		notify_one_locked();
	}

public:
	void invalidate() noexcept
	{
		std::lock_guard locker(m_mutex); LIBGS_UNUSED(locker);
		m_valid.store(false, std::memory_order_release);

		for(auto &waiter : m_waiters)
			cancel_waiter(waiter);

		m_waiters.clear();
		m_condition.notify_all();
	}

private:
	void notify_one_locked() noexcept
	{
		if( not m_waiters.empty() )
		{
			auto waiter = std::move(m_waiters.front());
			m_waiters.pop_front();
			cancel_waiter(waiter);
		}
		m_condition.notify_one();
	}

	static void cancel_waiter(const std::shared_ptr<asio::steady_timer> &waiter) noexcept
	{
		try {
			waiter->cancel();
		}
		catch(...) {}
	}

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
	std::atomic_bool m_valid { true };

	std::unordered_map<endpoint_t,std::list<socket_t>> m_pool {};
	std::atomic_size_t m_counter {0};

	std::condition_variable m_condition {};
	std::mutex m_mutex {};
	std::list<std::shared_ptr<asio::steady_timer>> m_waiters {};

	std::unordered_set<const opt_helper_t*> m_curr_tasks {};
	spin_mutex m_tasks_mutex {};

	executor_t m_exec {};
};

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::basic_connection_pool(const config_t &config) requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>(config))
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
	m_impl->invalidate();
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>::basic_connection_pool(basic_connection_pool &&other) noexcept :
	m_impl(std::move(other.m_impl))
{
	other.m_impl = std::make_shared<impl>(config_t{});
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
basic_connection_pool<Stream,Constructor>&
basic_connection_pool<Stream,Constructor>::operator=(basic_connection_pool &&other) noexcept
{
	if( this == &other )
		return *this;

	m_impl->invalidate();
	m_impl = std::move(other.m_impl);

	other.m_impl = std::make_shared<impl>(config_t{});
	return *this;
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::get
(const core_concepts::text_p<char> auto &host, const value &service, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->get(host, service)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->get(host, service);

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
				return m_impl->co_get(no_time_token.ec_, host, service,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_get(host, service,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(no_time_token.ec_, host, service,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(host, service,
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
				libgs::dispatch(m_impl->m_exec, [impl = m_impl->shared_from_this(), no_time_token,
					host = strtls::to_string(host), service, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						no_time_token.ec_, host, service, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [impl = m_impl->shared_from_this(),
					host = strtls::to_string(host), service, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						host, service, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				host = strtls::to_string(host), service, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					no_time_token.ec_, host, service, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				host = strtls::to_string(host), service, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					host, service, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return get(host, service, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::get(const dns_results &eps, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->get(eps)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->get(eps);

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
				return m_impl->co_get(no_time_token.ec_, eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_get(eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(no_time_token.ec_, eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(eps,
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
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), no_time_token, eps, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						no_time_token.ec_, eps, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), eps, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						eps, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				eps, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					no_time_token.ec_, eps, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				eps, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					eps, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return get(eps, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
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
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_get(no_time_token.ec_, ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_get(ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(no_time_token.ec_, ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_get(ep,
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
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), no_time_token, ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						no_time_token.ec_, ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
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
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				ep, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					no_time_token.ec_, ep, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				ep, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					ep, cancel_slot, timeout
				);
				original_token(std::move(expected));
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
auto basic_connection_pool<Stream,Constructor>::try_get
(const core_concepts::text_p<char> auto &host, const value &service, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->try_get(host, service)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->try_get(host, service);

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
				return m_impl->co_try_get(no_time_token.ec_, host, service,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_try_get(host, service,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(),
					m_impl->co_try_get(no_time_token.ec_, host, service,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
			else
			{
				return libgs::dispatch(get_executor(),
					m_impl->co_try_get(host, service,
						asio::get_associated_cancellation_slot(no_time_token),
						get_associated_redirect_time(token)
					), deferred
				);
			}
		}
		else if constexpr( is_use_future_v<original_token_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				libgs::dispatch(m_impl->m_exec, [impl = m_impl->shared_from_this(), no_time_token,
					host = strtls::to_string(host), service, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						no_time_token.ec_, host, service, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [impl = m_impl->shared_from_this(),
					host = strtls::to_string(host), service, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						host, service, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				host = strtls::to_string(host), service, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					no_time_token.ec_, host, service, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				host = strtls::to_string(host), service, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					host, service, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return try_get(host, service, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::try_get(const dns_results &eps, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->try_get(eps)
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->try_get(eps);

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
				return m_impl->co_try_get(no_time_token.ec_, eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_try_get(eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_try_get(no_time_token.ec_, eps,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_try_get(eps,
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
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), no_time_token, eps, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						no_time_token.ec_, eps, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), eps, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						eps, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				eps, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					no_time_token.ec_, eps, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				eps, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					eps, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return try_get(eps, token | 0ns);
	}
}

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Constructor>::try_get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,con_expected_t>
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
		decltype(auto) no_time_token = unbound_redirect_time(token);
		using no_time_token_t = std::remove_cvref_t<decltype(no_time_token)>;

		decltype(auto) original_token = unbound_token(no_time_token);
		using original_token_t = std::remove_cvref_t<decltype(original_token)>;

		if constexpr( is_use_awaitable_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return m_impl->co_try_get(no_time_token.ec_, ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_try_get(ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_deferred_v<original_token_t> )
		{
			if constexpr( is_redirect_error_v<no_time_token_t> )
			{
				return libgs::dispatch(get_executor(), m_impl->co_try_get(no_time_token.ec_, ep,
					asio::get_associated_cancellation_slot(no_time_token),
					get_associated_redirect_time(token)
				), deferred);
			}
			else
			{
				return libgs::dispatch(get_executor(), m_impl->co_try_get(ep,
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
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), no_time_token, ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_try_get (
						no_time_token.ec_, ep, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_exec, [
					impl = m_impl->shared_from_this(), ep, promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(no_time_token),
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
		else if constexpr( is_redirect_error_v<no_time_token_t> )
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), no_time_token, original_token,
				ep, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					no_time_token.ec_, ep, cancel_slot, timeout
				);
				original_token(std::move(expected));
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_exec, [
				impl = m_impl->shared_from_this(), original_token,
				ep, timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(no_time_token)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_try_get (
					ep, cancel_slot, timeout
				);
				original_token(std::move(expected));
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
		return m_impl->emplace(std::move(socket), true);
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

} //namespace libgs::http_nt

#if LIBGS_OPENSSL_SUPPORT
namespace libgs::http_nt { namespace detail
{

[[nodiscard]] LIBGS_HTTP_NT_API
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

} //namespace libgs::http_nt

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_CONNECTION_POOL_H
