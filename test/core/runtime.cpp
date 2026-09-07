#include "test.h"

#include <libgs/core/execution.h>

#include <chrono>
#include <future>
#include <vector>

namespace
{

using namespace std::chrono_literals;

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

void delayed_work_cancellation()
{
	libgs::io_context_t context;
	bool invoked = false;
	auto cancel = libgs::post(context, 1ms, [&] { invoked = true; });
	cancel();
	context.run();
	LIBGS_TEST_CHECK(not invoked);
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

} //namespace

int main()
{
	return libgs::test::run({
		{"queued and delayed work", queued_and_delayed_work},
		{"delayed work cancellation", delayed_work_cancellation},
		{"local dispatch and sleep", local_dispatch_and_sleep},
		{"periodic timer", periodic_timer},
	});
}
