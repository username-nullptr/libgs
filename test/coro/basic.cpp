#include "test.h"

#include <libgs/coro.h>

#include <chrono>
#include <future>
#include <stdexcept>
#include <utility>

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
		{"condition variable notification", condition_variable_notification},
		{"shared mutex readers", shared_mutex_readers},
		{"future waiting", future_waiting},
	});
}
