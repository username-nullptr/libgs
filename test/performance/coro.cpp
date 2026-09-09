// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/coro.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <utility>

namespace
{

using steady_clock_t = std::chrono::steady_clock;
using duration_t = steady_clock_t::duration;

#ifdef NDEBUG
constexpr size_t direct_cycle_count = 5'000'000;
constexpr size_t await_cycle_count = 250'000;
constexpr size_t queued_waiter_count = 20'000;
#else
constexpr size_t direct_cycle_count = 100'000;
constexpr size_t await_cycle_count = 10'000;
constexpr size_t queued_waiter_count = 2'000;
#endif

constexpr size_t sample_count = 3;

template <typename Func>
duration_t median_duration(size_t operation_count, Func &&func)
{
	func(std::max<size_t>(1, operation_count / 10));

	std::array<duration_t,sample_count> samples {};
	for(auto &sample : samples)
		sample = func(operation_count);
	std::ranges::sort(samples);
	return samples[sample_count / 2];
}

template <typename Func>
duration_t run_coroutine(Func &&func)
{
	libgs::io_context_t context;
	auto future = asio::co_spawn(context, std::forward<Func>(func), asio::use_future);
	context.run();
	return future.get();
}

duration_t measure_mutex_direct(size_t cycle_count)
{
	libgs::coro::mutex mutex;
	size_t acquired = 0;
	const auto begin = steady_clock_t::now();
	for(size_t index = 0; index < cycle_count; ++index)
	{
		acquired += mutex.try_lock();
		mutex.unlock();
	}
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(acquired, cycle_count);
	return elapsed;
}

duration_t measure_atomic_mutex_baseline(size_t cycle_count)
{
	std::atomic_bool locked {false};
	size_t acquired = 0;
	const auto begin = steady_clock_t::now();
	for(size_t index = 0; index < cycle_count; ++index)
	{
		bool expected = false;
		acquired += locked.compare_exchange_strong(expected, true,
			std::memory_order_acquire, std::memory_order_relaxed
		);
		locked.store(false, std::memory_order_release);
	}
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(acquired, cycle_count);
	return elapsed;
}

duration_t measure_semaphore_direct(size_t cycle_count)
{
	libgs::coro::binary_semaphore semaphore(1);
	size_t acquired = 0;
	const auto begin = steady_clock_t::now();
	for(size_t index = 0; index < cycle_count; ++index)
	{
		acquired += semaphore.try_acquire();
		semaphore.release();
	}
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(acquired, cycle_count);
	return elapsed;
}

duration_t measure_atomic_semaphore_baseline(size_t cycle_count)
{
	std::atomic_size_t counter {1};
	size_t acquired = 0;
	const auto begin = steady_clock_t::now();
	for(size_t index = 0; index < cycle_count; ++index)
	{
		auto value = counter.load(std::memory_order_relaxed);
		acquired += counter.compare_exchange_strong(value, value - 1,
			std::memory_order_acquire, std::memory_order_relaxed
		);
		counter.store(1, std::memory_order_release);
	}
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(acquired, cycle_count);
	return elapsed;
}

duration_t measure_immediate_awaitable(size_t cycle_count)
{
	return run_coroutine([cycle_count]() -> libgs::awaitable<duration_t>
	{
		auto immediate = []() -> libgs::awaitable<void> { co_return; };
		const auto begin = steady_clock_t::now();
		for(size_t index = 0; index < cycle_count; ++index)
			co_await immediate();
		co_return steady_clock_t::now() - begin;
	});
}

duration_t measure_mutex_await(size_t cycle_count)
{
	libgs::coro::mutex mutex;
	return run_coroutine([&mutex, cycle_count]() -> libgs::awaitable<duration_t>
	{
		const auto begin = steady_clock_t::now();
		for(size_t index = 0; index < cycle_count; ++index)
		{
			co_await mutex.lock();
			mutex.unlock();
		}
		co_return steady_clock_t::now() - begin;
	});
}

duration_t measure_semaphore_await(size_t cycle_count)
{
	libgs::coro::binary_semaphore semaphore(1);
	return run_coroutine([&semaphore, cycle_count]() -> libgs::awaitable<duration_t>
	{
		const auto begin = steady_clock_t::now();
		for(size_t index = 0; index < cycle_count; ++index)
		{
			co_await semaphore.acquire();
			semaphore.release();
		}
		co_return steady_clock_t::now() - begin;
	});
}

duration_t measure_shared_mutex_exclusive_await(size_t cycle_count)
{
	libgs::coro::shared_mutex mutex;
	return run_coroutine([&mutex, cycle_count]() -> libgs::awaitable<duration_t>
	{
		const auto begin = steady_clock_t::now();
		for(size_t index = 0; index < cycle_count; ++index)
		{
			co_await mutex.lock();
			mutex.unlock();
		}
		co_return steady_clock_t::now() - begin;
	});
}

duration_t measure_shared_mutex_reader_await(size_t cycle_count)
{
	libgs::coro::shared_mutex mutex;
	return run_coroutine([&mutex, cycle_count]() -> libgs::awaitable<duration_t>
	{
		const auto begin = steady_clock_t::now();
		for(size_t index = 0; index < cycle_count; ++index)
		{
			co_await mutex.lock_shared();
			mutex.unlock_shared();
		}
		co_return steady_clock_t::now() - begin;
	});
}

duration_t measure_mutex_queue(size_t waiter_count)
{
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	LIBGS_TEST_CHECK(mutex.try_lock());
	size_t completed = 0;

	for(size_t index = 0; index < waiter_count; ++index)
	{
		asio::co_spawn(context, [&]() -> libgs::awaitable<void>
		{
			co_await mutex.lock();
			++completed;
			mutex.unlock();
		}, asio::detached);
	}
	asio::post(context, [&mutex] { mutex.unlock(); });

	const auto begin = steady_clock_t::now();
	context.run();
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(completed, waiter_count);
	LIBGS_TEST_CHECK(not mutex.is_locked());
	return elapsed;
}

duration_t measure_semaphore_queue(size_t waiter_count)
{
	libgs::io_context_t context;
	libgs::coro::semaphore semaphore(0);
	size_t completed = 0;

	for(size_t index = 0; index < waiter_count; ++index)
	{
		asio::co_spawn(context, [&]() -> libgs::awaitable<void>
		{
			co_await semaphore.acquire();
			++completed;
		}, asio::detached);
	}
	asio::post(context, [&semaphore, waiter_count] {
		semaphore.release(waiter_count);
	});

	const auto begin = steady_clock_t::now();
	context.run();
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(completed, waiter_count);
	LIBGS_TEST_CHECK_EQ(semaphore.count(), size_t {0});
	return elapsed;
}

duration_t measure_condition_notify_all(size_t waiter_count)
{
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	libgs::coro::condition_variable condition;
	size_t completed = 0;

	for(size_t index = 0; index < waiter_count; ++index)
	{
		asio::co_spawn(context, [&]() -> libgs::awaitable<void>
		{
			libgs::coro::unique_lock lock(mutex);
			co_await lock.lock();
			co_await condition.wait(lock);
			++completed;
			lock.unlock();
		}, asio::detached);
	}
	asio::post(context, [&condition] { condition.notify_all(); });

	const auto begin = steady_clock_t::now();
	context.run();
	const auto elapsed = steady_clock_t::now() - begin;
	LIBGS_TEST_CHECK_EQ(completed, waiter_count);
	LIBGS_TEST_CHECK(not mutex.is_locked());
	return elapsed;
}

void direct_fast_paths()
{
	libgs::test::print_performance_result(
		"coro/atomic mutex-state baseline (median of 3)", direct_cycle_count,
		median_duration(direct_cycle_count, measure_atomic_mutex_baseline), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/mutex try_lock + unlock (median of 3)", direct_cycle_count,
		median_duration(direct_cycle_count, measure_mutex_direct), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/atomic semaphore-count baseline (median of 3)", direct_cycle_count,
		median_duration(direct_cycle_count, measure_atomic_semaphore_baseline), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/semaphore try_acquire + release (median of 3)", direct_cycle_count,
		median_duration(direct_cycle_count, measure_semaphore_direct), "cycle"
	);
}

void awaitable_fast_paths()
{
	libgs::test::print_performance_result(
		"coro/immediate awaitable baseline (median of 3)", await_cycle_count,
		median_duration(await_cycle_count, measure_immediate_awaitable), "await"
	);
	libgs::test::print_performance_result(
		"coro/mutex await lock + unlock, uncontended (median of 3)", await_cycle_count,
		median_duration(await_cycle_count, measure_mutex_await), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/semaphore await acquire + release, uncontended (median of 3)", await_cycle_count,
		median_duration(await_cycle_count, measure_semaphore_await), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/shared_mutex await exclusive lock + unlock, uncontended (median of 3)",
		await_cycle_count,
		median_duration(await_cycle_count, measure_shared_mutex_exclusive_await), "cycle"
	);
	libgs::test::print_performance_result(
		"coro/shared_mutex await shared lock + unlock, uncontended (median of 3)",
		await_cycle_count,
		median_duration(await_cycle_count, measure_shared_mutex_reader_await), "cycle"
	);
}

void suspended_waiters()
{
	libgs::test::print_performance_result(
		"coro/mutex queued waiters, enqueue + drain (median of 3)", queued_waiter_count,
		median_duration(queued_waiter_count, measure_mutex_queue), "waiter"
	);
	libgs::test::print_performance_result(
		"coro/semaphore queued waiters, enqueue + release (median of 3)",
		queued_waiter_count,
		median_duration(queued_waiter_count, measure_semaphore_queue), "waiter"
	);
	libgs::test::print_performance_result(
		"coro/condition_variable notify_all, enqueue + wake (median of 3)",
		queued_waiter_count,
		median_duration(queued_waiter_count, measure_condition_notify_all), "waiter"
	);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"coroutine synchronization direct fast paths", direct_fast_paths},
		{"coroutine synchronization awaitable fast paths", awaitable_fast_paths},
		{"coroutine synchronization suspended waiters", suspended_waiters},
	});
}
