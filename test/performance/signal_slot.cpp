// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/utils/signal_slot.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

namespace
{

#ifdef NDEBUG
constexpr size_t emission_count = 1'000'000;
#else
constexpr size_t emission_count = 100'000;
#endif

void measure_synchronous_signal(size_t slot_count, std::string_view name)
{
	libgs::utils::signal<void(size_t)> fired;
	std::uint64_t checksum = 0;
	std::array<std::function<void(size_t)>,8> slots;
	for(size_t index = 0; index < slot_count; ++index)
		slots[index] = [&](size_t value) { checksum += value; };
	if(slot_count == 1)
		fired.connect<libgs::utils::slot_mode::sync>(std::move(slots[0]));
	else
	{
		fired.connect<libgs::utils::slot_mode::sync>(
			std::move(slots[0]), std::move(slots[1]),
			std::move(slots[2]), std::move(slots[3]),
			std::move(slots[4]), std::move(slots[5]),
			std::move(slots[6]), std::move(slots[7])
		);
	}

	for(size_t index = 0; index < 1'000; ++index)
		fired(index);
	checksum = 0;

	const auto begin = std::chrono::steady_clock::now();
	for(size_t index = 1; index <= emission_count; ++index)
		fired(index);
	const auto elapsed = std::chrono::steady_clock::now() - begin;

	const auto expected = static_cast<std::uint64_t>(emission_count) *
		(static_cast<std::uint64_t>(emission_count) + 1) / 2 * slot_count;
	LIBGS_TEST_CHECK_EQ(checksum, expected);
	libgs::test::print_performance_result(
		name, emission_count, elapsed, "emit"
	);
}

void synchronous_signal_throughput()
{
	measure_synchronous_signal(1, "signal-slot/synchronous 1 slot");
	measure_synchronous_signal(8, "signal-slot/synchronous 8 slots");
}

} //namespace

int main()
{
	return libgs::test::run({
		{"synchronous signal throughput", synchronous_signal_throughput},
	});
}
