// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/coro.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{

using namespace std::chrono_literals;

void mutex_state()
{
	libgs::coro::mutex mutex;
	LIBGS_TEST_CHECK(not mutex.is_locked());
	LIBGS_TEST_CHECK(mutex.try_lock());
	LIBGS_TEST_CHECK(mutex.is_locked());
	LIBGS_TEST_CHECK(not mutex.try_lock());
	mutex.unlock();
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void unique_lock_ownership()
{
	libgs::coro::mutex mutex;
	libgs::coro::unique_lock lock(mutex);
	LIBGS_TEST_CHECK(lock.mutex() == &mutex);
	LIBGS_TEST_CHECK(not lock.is_locked());
	LIBGS_TEST_CHECK(lock.try_lock());
	LIBGS_TEST_CHECK(lock.is_locked());

	libgs::coro::unique_lock moved(std::move(lock));
	LIBGS_TEST_CHECK(not lock.is_locked());
	LIBGS_TEST_CHECK(moved.is_locked());
	moved.unlock();
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void semaphore_counts()
{
	libgs::coro::basic_semaphore<3> semaphore(2);
	LIBGS_TEST_CHECK_EQ(semaphore.max(), size_t {3});
	LIBGS_TEST_CHECK_EQ(semaphore.count(), size_t {2});
	LIBGS_TEST_CHECK(semaphore.try_acquire());
	LIBGS_TEST_CHECK(semaphore.try_acquire());
	LIBGS_TEST_CHECK(not semaphore.try_acquire());
	LIBGS_TEST_CHECK_EQ(semaphore.release(2), size_t {2});

	bool invalid_release = false;
	try {
		semaphore.release(2);
	}
	catch(const std::invalid_argument&) {
		invalid_release = true;
	}
	LIBGS_TEST_CHECK(invalid_release);

	libgs::coro::binary_semaphore binary(0);
	LIBGS_TEST_CHECK(not binary.try_acquire());
	LIBGS_TEST_CHECK_EQ(binary.release(), size_t {1});
	LIBGS_TEST_CHECK(binary.try_acquire());
}

void asynchronous_mutex_and_timeout()
{
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		co_await mutex.lock();
		const bool reacquired = co_await mutex.try_lock_for(1ms);
		mutex.unlock();
		co_return reacquired;
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(not result.get());
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void asynchronous_semaphore()
{
	libgs::io_context_t context;
	libgs::coro::binary_semaphore semaphore(0);
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<int>
	{
		libgs::post(context, 1ms, [&] { semaphore.release(); });
		co_await semaphore.acquire();
		co_return 42;
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(result.get(), 42);
	LIBGS_TEST_CHECK_EQ(semaphore.count(), 0U);
}

void asynchronous_semaphore_timeout()
{
	libgs::io_context_t context;
	libgs::coro::binary_semaphore semaphore(0);
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		if( co_await semaphore.try_acquire_for(1ms) )
			co_return false;
		semaphore.release();
		co_return semaphore.try_acquire();
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(result.get());
	LIBGS_TEST_CHECK_EQ(semaphore.count(), 0U);
}

void multithreaded_mutex_contention()
{
	constexpr size_t task_count = 8;
	constexpr size_t iteration_count = 1'000;
	asio::thread_pool pool(4);
	libgs::coro::mutex mutex;
	std::atomic_size_t active {0};
	std::atomic_size_t completed {0};
	std::atomic_bool overlap {false};
	std::vector<std::future<void>> futures;

	for(size_t task = 0; task < task_count; ++task)
	{
		futures.emplace_back(asio::co_spawn(pool,
		[&]() -> libgs::awaitable<void>
		{
			auto exec = co_await asio::this_coro::executor;
			for(size_t iteration = 0; iteration < iteration_count; ++iteration)
			{
				co_await mutex.lock();
				if( active.fetch_add(1, std::memory_order_acq_rel) != 0 )
					overlap.store(true, std::memory_order_relaxed);
				co_await asio::post(exec, asio::use_awaitable);
				active.fetch_sub(1, std::memory_order_release);
				completed.fetch_add(1, std::memory_order_relaxed);
				mutex.unlock();
				co_await asio::post(exec, asio::use_awaitable);
			}
		}, asio::use_future));
	}
	for(auto &future : futures)
		future.get();
	pool.join();

	LIBGS_TEST_CHECK(not overlap.load(std::memory_order_relaxed));
	LIBGS_TEST_CHECK_EQ(active.load(std::memory_order_relaxed), 0U);
	LIBGS_TEST_CHECK_EQ(
		completed.load(std::memory_order_relaxed), task_count * iteration_count
	);
}

void multithreaded_semaphore_contention()
{
	constexpr size_t max_concurrency = 3;
	constexpr size_t task_count = 8;
	constexpr size_t iteration_count = 1'000;
	asio::thread_pool pool(4);
	libgs::coro::basic_semaphore<max_concurrency> semaphore(max_concurrency);
	std::atomic_size_t active {0};
	std::atomic_size_t high_watermark {0};
	std::atomic_size_t completed {0};
	std::vector<std::future<void>> futures;

	for(size_t task = 0; task < task_count; ++task)
	{
		futures.emplace_back(asio::co_spawn(pool,
		[&]() -> libgs::awaitable<void>
		{
			auto exec = co_await asio::this_coro::executor;
			for(size_t iteration = 0; iteration < iteration_count; ++iteration)
			{
				co_await semaphore.acquire();
				const auto current = active.fetch_add(1, std::memory_order_acq_rel) + 1;
				auto maximum = high_watermark.load(std::memory_order_relaxed);
				while( maximum < current and not high_watermark.compare_exchange_weak(
					maximum, current, std::memory_order_relaxed
				)) {}
				co_await asio::post(exec, asio::use_awaitable);
				active.fetch_sub(1, std::memory_order_release);
				completed.fetch_add(1, std::memory_order_relaxed);
				semaphore.release();
				co_await asio::post(exec, asio::use_awaitable);
			}
		}, asio::use_future));
	}
	for(auto &future : futures)
		future.get();
	pool.join();

	LIBGS_TEST_CHECK(high_watermark.load(std::memory_order_relaxed) <= max_concurrency);
	LIBGS_TEST_CHECK_EQ(active.load(std::memory_order_relaxed), 0U);
	LIBGS_TEST_CHECK_EQ(
		completed.load(std::memory_order_relaxed), task_count * iteration_count
	);
	LIBGS_TEST_CHECK_EQ(semaphore.count(), max_concurrency);
}

void large_mutex_waiter_queue()
{
	constexpr size_t waiter_count = 20'000;
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	size_t completed = 0;
	LIBGS_TEST_CHECK(mutex.try_lock());

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
	context.run();

	LIBGS_TEST_CHECK_EQ(completed, waiter_count);
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void condition_variable_notification()
{
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	libgs::coro::condition_variable condition;
	bool ready = false;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		libgs::coro::unique_lock lock(mutex);
		co_await lock.lock();
		libgs::post(context, 1ms, [&]
		{
			ready = true;
			condition.notify_one();
		});
		co_return co_await condition.wait_for(lock, 50ms, [&] { return ready; });
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(result.get());
	LIBGS_TEST_CHECK(ready);
}

void shared_mutex_readers()
{
	libgs::io_context_t context;
	libgs::coro::shared_mutex mutex;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		co_await mutex.lock_shared();
		LIBGS_TEST_CHECK(mutex.try_lock_shared());
		const bool writer_acquired = co_await mutex.try_lock_for(1ms);
		mutex.unlock_shared();
		mutex.unlock_shared();
		co_await mutex.lock();
		mutex.unlock();
		co_return writer_acquired;
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(not result.get());
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void timed_shared_mutex_and_lock_ownership()
{
	libgs::coro::shared_mutex mutex;
	libgs::coro::shared_lock lock(mutex);
	LIBGS_TEST_CHECK(lock.mutex() == &mutex);
	LIBGS_TEST_CHECK(lock.try_lock_shared());
	LIBGS_TEST_CHECK(lock.is_locked());

	libgs::coro::shared_lock moved(std::move(lock));
	LIBGS_TEST_CHECK(not lock.is_locked());
	LIBGS_TEST_CHECK(moved.is_locked());

	libgs::io_context_t context;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		const bool writer = co_await mutex.try_lock_until(
			std::chrono::steady_clock::now() + 1ms);
		moved.unlock_shared();
		const bool reader = co_await mutex.try_lock_shared_for(5ms);
		if( reader )
			mutex.unlock_shared();
		co_return not writer and reader;
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(result.get());
	LIBGS_TEST_CHECK(not mutex.is_locked());
}

void condition_variable_timeout()
{
	libgs::io_context_t context;
	libgs::coro::mutex mutex;
	libgs::coro::condition_variable condition;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		libgs::coro::unique_lock lock(mutex);
		co_await lock.lock();
		const auto deadline = std::chrono::steady_clock::now() + 1ms;
		const bool notified = co_await condition.wait_until(lock, deadline);
		LIBGS_TEST_CHECK(lock.is_locked());
		lock.unlock();
		co_return notified;
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(not result.get());
}

void coroutine_utilities()
{
	using namespace libgs::coro::literals;
	libgs::io_context_t context;
	bool thread_completed = false;
	std::thread worker([&] { thread_completed = true; });
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<bool>
	{
		co_await 0_ms;
		co_await libgs::coro::sleep_until(std::chrono::steady_clock::now());
		co_await libgs::coro::wait(worker);
		co_return thread_completed and not worker.joinable();
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK(result.get());
}

void future_waiting()
{
	libgs::io_context_t context;
	auto external = std::async(std::launch::async, [] { return 42; });
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<int>
	{
		co_return co_await libgs::coro::wait(external);
	}, asio::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(result.get(), 42);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"mutex state", mutex_state},
		{"unique lock ownership", unique_lock_ownership},
		{"semaphore counts", semaphore_counts},
		{"asynchronous mutex timeout", asynchronous_mutex_and_timeout},
		{"asynchronous semaphore", asynchronous_semaphore},
		{"asynchronous semaphore timeout", asynchronous_semaphore_timeout},
		{"multithreaded mutex contention", multithreaded_mutex_contention},
		{"multithreaded semaphore contention", multithreaded_semaphore_contention},
		{"large mutex waiter queue", large_mutex_waiter_queue},
		{"condition variable notification", condition_variable_notification},
		{"shared mutex readers", shared_mutex_readers},
		{"timed shared mutex and lock ownership", timed_shared_mutex_and_lock_ownership},
		{"condition variable timeout", condition_variable_timeout},
		{"coroutine utilities", coroutine_utilities},
		{"future waiting", future_waiting},
	});
}
