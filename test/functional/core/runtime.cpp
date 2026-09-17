// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/core/async_expected.h>

namespace
{

using namespace std::chrono_literals;

void dispatch_and_post_ordering()
{
	libgs::io_context_t context;
	std::vector<int> order;

	asio::post(context, [&]
	{
		order.push_back(1);
		libgs::dispatch(context, [&] { order.push_back(2); });
		libgs::post(context, [&]() -> libgs::awaitable<void>
		{
			order.push_back(4);
			co_return ;
		}, [&order](std::exception_ptr error)
		{
			LIBGS_TEST_CHECK(not error);
			order.push_back(5);
		});
		order.push_back(3);
	});

	context.run();
	LIBGS_TEST_CHECK_EQ(order, (std::vector<int>{1, 2, 3, 4, 5}));
}

void synchronous_dispatch_context_selection()
{
	libgs::io_context_t context;
	std::vector<int> order;

	asio::post(context, [&]
	{
		const auto caller = std::this_thread::get_id();
		order.push_back(1);
		const auto worker = libgs::dispatch(context, [&]
		{
			order.push_back(2);
			return std::this_thread::get_id();
		}, libgs::use_sync);
		order.push_back(3);
		LIBGS_TEST_CHECK(worker == caller);
	});

	context.run();
	LIBGS_TEST_CHECK_EQ(order, (std::vector<int>{1, 2, 3}));

	context.restart();
	auto work_guard = asio::make_work_guard(context);
	std::promise<void> runner_ready;
	auto ready = runner_ready.get_future();
	auto runner = std::async(std::launch::async, [&]
	{
		runner_ready.set_value();
		context.run();
	});
	ready.get();

	const auto caller = std::this_thread::get_id();
	const auto worker = libgs::dispatch(context,
		[] { return std::this_thread::get_id(); }, libgs::use_sync);
	LIBGS_TEST_CHECK(worker != caller);

	work_guard.reset();
	runner.get();
}

void queued_and_delayed_work()
{
	libgs::io_context_t context;
	std::vector<int> order;

	auto value = libgs::post(context, [&]
	{
		order.push_back(1);
		return 42;
	}, libgs::use_future);
	libgs::post(context, [&] { order.push_back(2); });
	libgs::post(context, 1ms, [&] { order.push_back(3); });

	context.run();
	LIBGS_TEST_CHECK_EQ(value.get(), 42);
	LIBGS_TEST_CHECK_EQ(order.size(), 3U);
	LIBGS_TEST_CHECK_EQ(order[0], 1);
	LIBGS_TEST_CHECK_EQ(order[1], 2);
	LIBGS_TEST_CHECK_EQ(order[2], 3);
}

void future_exception_propagation()
{
	libgs::io_context_t context;
	auto result = libgs::post(context, []() -> int
	{
		throw std::runtime_error("execution probe");
	}, libgs::use_future);

	context.run();
	LIBGS_TEST_CHECK_THROWS(result.get(), std::runtime_error);
}

void delayed_work_cancellation()
{
	libgs::io_context_t context;
	bool invoked = false;
	auto cancel = libgs::post(context, 1ms, [&] { invoked = true; });
	cancel();
	context.run();
	LIBGS_TEST_CHECK(not invoked);
}

void cross_thread_delayed_cancellation()
{
	libgs::io_context_t context;
	std::atomic_bool invoked = false;
	auto cancel = libgs::post(context, 50ms, [&] { invoked = true; });
	auto runner = std::async(std::launch::async, [&] { context.run(); });

	cancel();
	runner.get();
	LIBGS_TEST_CHECK(not invoked.load());
}

void local_dispatch_and_sleep()
{
	libgs::io_context_t context;
	const auto [result, work_count] = libgs::local_dispatch(
		context, [] { return 7 * 6; }, libgs::use_sync
	);
	LIBGS_TEST_CHECK_EQ(result, 42);
	LIBGS_TEST_CHECK_EQ(*work_count, 1U);

	const auto before = std::chrono::steady_clock::now();
	libgs::sleep_for(1ms);
	LIBGS_TEST_CHECK(std::chrono::steady_clock::now() >= before);
}

void local_event_pumps()
{
	libgs::io_context_t stopped_context;
	stopped_context.run();
	auto [value, work_count] = libgs::local_dispatch(stopped_context,
		[]() -> libgs::awaitable<int>
		{
			co_return 42;
		}, libgs::use_sync);
	LIBGS_TEST_CHECK_EQ(value, 42);
	LIBGS_TEST_CHECK(*work_count > 0);

	asio::thread_pool pool(1);
	auto future = libgs::local_dispatch(pool, [] { return 21 * 2; }, libgs::use_future);
	auto [pool_value, pool_work_count] = future.get();
	pool.join();
	LIBGS_TEST_CHECK_EQ(pool_value, 42);
	LIBGS_TEST_CHECK_EQ(*pool_work_count, 0U);

	LIBGS_TEST_CHECK_THROWS(libgs::local_dispatch(stopped_context,
		[]() -> libgs::awaitable<void>
		{
			throw std::runtime_error("local dispatch probe");
			co_return ;
		}, libgs::use_sync), std::runtime_error);
}

void asynchronous_sleep_tokens()
{
	libgs::io_context_t context;
	auto future = libgs::sleep_for(context, 1ms, libgs::use_future);
	bool callback_called = false;
	libgs::sleep_until(context, std::chrono::steady_clock::now(),
		[&](const libgs::error_code &error)
		{
			LIBGS_TEST_CHECK(not error);
			callback_called = true;
		});

	context.run();
	future.get();
	LIBGS_TEST_CHECK(callback_called);
}

void async_work_never_completes_inline()
{
	libgs::io_context_t context;
	bool initiating = true;
	bool completed = false;

	libgs::async_work<>::handle(context,
		[](auto handler) mutable { std::move(handler)(); },
		[&]
		{
			LIBGS_TEST_CHECK(not initiating);
			completed = true;
		});
	initiating = false;

	LIBGS_TEST_CHECK(not completed);
	context.run();
	LIBGS_TEST_CHECK(completed);
}

void posted_completion_uses_immediate_executor()
{
	libgs::io_context_t io_context;
	libgs::io_context_t completion_context;
	bool initiating = true;
	bool completed = false;

	libgs::post_completion(io_context.get_executor(),
		asio::bind_immediate_executor(completion_context.get_executor(),
		[&](libgs::error_code error, int value)
		{
			LIBGS_TEST_CHECK(not initiating);
			LIBGS_TEST_CHECK(not error);
			LIBGS_TEST_CHECK_EQ(value, 42);
			completed = true;
		}), libgs::error_code {}, 42);
	initiating = false;

	LIBGS_TEST_CHECK(not completed);
	completion_context.run();
	LIBGS_TEST_CHECK(completed);
}

void periodic_timer()
{
	libgs::io_context_t context;
	int ticks = 0;
	libgs::work_canceller_t cancel;
	cancel = libgs::start_timer(context, 1ms, [&]
	{
		if(++ticks == 3)
			cancel();
	});
	context.run();
	LIBGS_TEST_CHECK_EQ(ticks, 3);
}

void cross_thread_periodic_cancellation()
{
	libgs::io_context_t context;
	std::atomic_size_t ticks = 0;
	auto cancel = libgs::start_timer(context, 1ms, [&]
	{
		ticks.fetch_add(1, std::memory_order_relaxed);
	});
	auto runner = std::async(std::launch::async, [&] { context.run(); });

	for(size_t retry = 0; retry < 1'000 and ticks.load() == 0; ++retry)
		std::this_thread::sleep_for(1ms);
	cancel();
	runner.get();
	LIBGS_TEST_CHECK(ticks.load() > 0);
}

void awaitable_and_absolute_work()
{
	libgs::io_context_t context;
	auto awaitable_result = libgs::post(context,
		[]() -> libgs::awaitable<int> { co_return 42; }, libgs::use_future);
	bool absolute_invoked = false;
	libgs::post(context, std::chrono::steady_clock::now(),
		[&] { absolute_invoked = true; });

	int ticks = 0;
	libgs::work_canceller_t cancel;
	cancel = libgs::start_timer(context, 1ms,
		[&](const libgs::work_canceller_t &stop)
		{
			++ticks;
			stop();
		}, true);
	context.run();
	LIBGS_TEST_CHECK_EQ(awaitable_result.get(), 42);
	LIBGS_TEST_CHECK(absolute_invoked);
	LIBGS_TEST_CHECK_EQ(ticks, 1);
}

void runtime_recovers_from_handler_exception()
{
	libgs::post([] { throw std::runtime_error("runtime probe"); });
	LIBGS_TEST_CHECK_THROWS(libgs::exec(), std::runtime_error);
	LIBGS_TEST_CHECK(not libgs::is_run());
}

void global_event_loop()
{
	auto result = std::async(std::launch::async, [] { return libgs::exec(); });
	for(size_t retry = 0; retry < 1'000 and not libgs::is_run(); ++retry)
		std::this_thread::sleep_for(1ms);
	const bool started = libgs::is_run();
	LIBGS_TEST_CHECK_THROWS(libgs::exec(), libgs::runtime_error);
	libgs::post([] { libgs::exit(7); });
	LIBGS_TEST_CHECK_EQ(result.get(), 7);
	LIBGS_TEST_CHECK(started);
	LIBGS_TEST_CHECK(not libgs::is_run());
}

} //namespace

int main()
{
	return libgs::test::run({
		{"dispatch and post ordering", dispatch_and_post_ordering},
		{"synchronous dispatch context selection", synchronous_dispatch_context_selection},
		{"queued and delayed work", queued_and_delayed_work},
		{"future exception propagation", future_exception_propagation},
		{"delayed work cancellation", delayed_work_cancellation},
		{"cross-thread delayed cancellation", cross_thread_delayed_cancellation},
		{"local dispatch and sleep", local_dispatch_and_sleep},
		{"local event pumps", local_event_pumps},
		{"asynchronous sleep tokens", asynchronous_sleep_tokens},
		{"async work never completes inline", async_work_never_completes_inline},
		{"posted completion uses immediate executor",
			posted_completion_uses_immediate_executor},
		{"periodic timer", periodic_timer},
		{"cross-thread periodic cancellation", cross_thread_periodic_cancellation},
		{"awaitable and absolute work", awaitable_and_absolute_work},
		{"runtime recovers from handler exception", runtime_recovers_from_handler_exception},
		{"global event loop", global_event_loop},
	});
}
