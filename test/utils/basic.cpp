// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/observer.h>
#include <libgs/utils/signal_slot.h>

#include <memory>
#include <string_view>

namespace
{

void synchronous_signal()
{
	libgs::utils::signal<void(int,std::string_view)> changed;
	int total = 0;
	std::string_view last_label;

	changed.connect([&](int value, std::string_view label) {
		total += value;
		last_label = label;
	});
	changed(3, "first");
	LIBGS_TEST_CHECK_EQ(total, 3);
	LIBGS_TEST_CHECK_EQ(last_label, "first");

	changed.block();
	LIBGS_TEST_CHECK(changed.is_blocked());
	changed(10, "blocked");
	LIBGS_TEST_CHECK_EQ(total, 3);

	changed.block(false);
	changed.disconnect();
	changed(10, "disconnected");
	LIBGS_TEST_CHECK_EQ(total, 3);
}

void observer_lifecycle()
{
	using observer_t = libgs::utils::basic_observer<
		libgs::io_executor_t, void(int)
	>;

	libgs::io_context_t context;
	int received = 0;
	auto observer = observer_t::make(7, context.get_executor());
	observer->on_triggered<0>([&](int value) {
		received += value;
	});

	observer_t::trigger<0>(7, 4);
	context.run();
	LIBGS_TEST_CHECK_EQ(received, 4);

	observer.reset();
	context.restart();
	observer_t::trigger<0>(7, 8);
	context.run();
	LIBGS_TEST_CHECK_EQ(received, 4);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"synchronous signal", synchronous_signal},
		{"observer lifecycle", observer_lifecycle},
	});
}
