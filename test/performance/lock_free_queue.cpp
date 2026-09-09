// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/core/lock_free_queue.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <thread>

namespace
{

#ifdef NDEBUG
constexpr size_t operation_count = 1'000'000;
#else
constexpr size_t operation_count = 100'000;
#endif

template <libgs::queue_type Type>
std::chrono::steady_clock::duration measure_spsc_queue(size_t item_count)
{
	libgs::lock_free_queue<size_t,Type> queue(65'536);
	std::atomic_uint ready = 0;
	std::atomic_bool start = false;
	std::uint64_t checksum = 0;

	std::thread producer([&]
	{
		ready.fetch_add(1, std::memory_order_release);
		while(not start.load(std::memory_order_acquire))
			std::this_thread::yield();

		for(size_t value = 1; value <= item_count; ++value)
		{
			while(not queue.enqueue(value))
				std::this_thread::yield();
		}
	});

	std::thread consumer([&]
	{
		ready.fetch_add(1, std::memory_order_release);
		while(not start.load(std::memory_order_acquire))
			std::this_thread::yield();

		for(size_t index = 0; index < item_count;)
		{
			if(auto value = queue.dequeue())
			{
				checksum += *value;
				++index;
			}
			else
				std::this_thread::yield();
		}
	});

	while(ready.load(std::memory_order_acquire) != 2)
		std::this_thread::yield();
	const auto begin = std::chrono::steady_clock::now();
	start.store(true, std::memory_order_release);
	producer.join();
	consumer.join();
	const auto elapsed = std::chrono::steady_clock::now() - begin;

	const auto expected = static_cast<std::uint64_t>(item_count) *
		(static_cast<std::uint64_t>(item_count) + 1) / 2;
	LIBGS_TEST_CHECK_EQ(checksum, expected);
	LIBGS_TEST_CHECK(queue.empty());
	return elapsed;
}

void lock_free_queue_throughput()
{
	constexpr size_t sample_count = 3;
	measure_spsc_queue<libgs::queue_type::linked>(operation_count / 10);
	measure_spsc_queue<libgs::queue_type::circular>(operation_count / 10);

	std::array<std::chrono::steady_clock::duration,sample_count> linked {};
	std::array<std::chrono::steady_clock::duration,sample_count> circular {};
	for(size_t sample = 0; sample < sample_count; ++sample)
	{
		if(sample % 2 == 0)
		{
			linked[sample] = measure_spsc_queue<libgs::queue_type::linked>(operation_count);
			circular[sample] = measure_spsc_queue<libgs::queue_type::circular>(operation_count);
		}
		else
		{
			circular[sample] = measure_spsc_queue<libgs::queue_type::circular>(operation_count);
			linked[sample] = measure_spsc_queue<libgs::queue_type::linked>(operation_count);
		}
	}
	std::ranges::sort(linked);
	std::ranges::sort(circular);
	libgs::test::print_performance_result(
		"lock-free queue/linked SPSC (median of 3)", operation_count,
		linked[sample_count / 2], "item"
	);
	libgs::test::print_performance_result(
		"lock-free queue/circular SPSC (median of 3)", operation_count,
		circular[sample_count / 2], "item"
	);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"lock-free queue throughput", lock_free_queue_throughput},
	});
}
