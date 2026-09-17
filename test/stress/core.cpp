// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/url.h>

#include <numeric>

namespace
{

constexpr size_t scale = LIBGS_STRESS_SCALE;

template <typename Queue>
void run_mpmc_queue_pressure(Queue &queue)
{
	constexpr size_t producer_count = 4;
	constexpr size_t consumer_count = 4;
	const size_t values_per_producer = 25'000 * scale;
	const size_t total_values = producer_count * values_per_producer;
	std::atomic_size_t ready {0};
	std::atomic_size_t producers_left {producer_count};
	std::atomic_bool start {false};
	std::atomic_bool corrupt {false};
	std::array<std::thread,producer_count> producers;
	std::array<std::thread,consumer_count> consumers;
	std::array<uint64_t,consumer_count> checksums {};
	std::array<size_t,consumer_count> counts {};
	auto seen = std::make_unique<std::atomic_uint8_t[]>(total_values);
	for(size_t index = 0; index < total_values; ++index)
		seen[index].store(0, std::memory_order_relaxed);

	for(size_t producer = 0; producer < producer_count; ++producer)
	{
		producers[producer] = std::thread([&, producer]
		{
			ready.fetch_add(1, std::memory_order_release);
			while(not start.load(std::memory_order_acquire))
				std::this_thread::yield();
			const auto first = producer * values_per_producer + 1;
			for(size_t offset = 0; offset < values_per_producer; ++offset)
			{
				while(not queue.enqueue(first + offset))
					std::this_thread::yield();
			}
			producers_left.fetch_sub(1, std::memory_order_release);
		});
	}
	for(size_t consumer = 0; consumer < consumer_count; ++consumer)
	{
		consumers[consumer] = std::thread([&, consumer]
		{
			ready.fetch_add(1, std::memory_order_release);
			while(not start.load(std::memory_order_acquire))
				std::this_thread::yield();
			for(;;)
			{
				if(auto value = queue.dequeue())
				{
					checksums[consumer] += *value;
					counts[consumer]++;
					if(*value == 0 or *value > total_values)
						corrupt.store(true, std::memory_order_relaxed);
					else if(seen[*value - 1].fetch_add(1,
						std::memory_order_relaxed) != 0)
						corrupt.store(true, std::memory_order_relaxed);
				}
				else if(producers_left.load(std::memory_order_acquire) == 0 and
					queue.empty())
					break;
				else
					std::this_thread::yield();
			}
		});
	}

	while(ready.load(std::memory_order_acquire) != producer_count + consumer_count)
		std::this_thread::yield();
	start.store(true, std::memory_order_release);
	for(auto &thread : producers)
		thread.join();
	for(auto &thread : consumers)
		thread.join();

	const auto count = std::accumulate(counts.begin(), counts.end(), size_t {0});
	const auto checksum = std::accumulate(checksums.begin(), checksums.end(), uint64_t {0});
	const auto expected = static_cast<uint64_t>(total_values) * (total_values + 1) / 2;
	LIBGS_TEST_CHECK_EQ(count, total_values);
	LIBGS_TEST_CHECK_EQ(checksum, expected);
	LIBGS_TEST_CHECK(not corrupt.load(std::memory_order_relaxed));
	for(size_t index = 0; index < total_values; ++index)
		LIBGS_TEST_CHECK_EQ(seen[index].load(std::memory_order_relaxed), 1U);
	LIBGS_TEST_CHECK(queue.empty());
}

void circular_queue_saturation_pressure()
{
	libgs::circular_lock_free_queue<uint64_t> queue(1);
	run_mpmc_queue_pressure(queue);
}

void linked_queue_reclamation_pressure()
{
	libgs::linked_lock_free_queue<uint64_t> queue(1);
	run_mpmc_queue_pressure(queue);
}

void forced_eviction_pressure()
{
	constexpr size_t producer_count = 4;
	constexpr size_t consumer_count = 2;
	const size_t values_per_producer = 20'000 * scale;
	const size_t total_values = producer_count * values_per_producer;
	libgs::linked_lock_free_queue<size_t> queue(32);
	std::atomic_size_t ready {0};
	std::atomic_size_t producers_left {producer_count};
	std::atomic_size_t evicted {0};
	std::atomic_size_t consumed {0};
	std::atomic_bool start {false};
	std::atomic_bool corrupt {false};
	auto seen = std::make_unique<std::atomic_uint8_t[]>(total_values);
	for(size_t index = 0; index < total_values; ++index)
		seen[index].store(0, std::memory_order_relaxed);

	std::array<std::thread,producer_count> producers;
	for(size_t producer = 0; producer < producer_count; ++producer)
	{
		producers[producer] = std::thread([&, producer]
		{
			ready.fetch_add(1, std::memory_order_release);
			while(not start.load(std::memory_order_acquire))
				std::this_thread::yield();
			const auto first = producer * values_per_producer;
			for(size_t offset = 0; offset < values_per_producer; ++offset)
			{
				evicted.fetch_add(queue.force_emplace(first + offset),
					std::memory_order_relaxed);
			}
			producers_left.fetch_sub(1, std::memory_order_release);
		});
	}
	std::array<std::thread,consumer_count> consumers;
	for(auto &consumer : consumers)
	{
		consumer = std::thread([&]
		{
			ready.fetch_add(1, std::memory_order_release);
			while(not start.load(std::memory_order_acquire))
				std::this_thread::yield();
			for(;;)
			{
				if(auto value = queue.dequeue())
				{
					consumed.fetch_add(1, std::memory_order_relaxed);
					if(*value >= total_values or
						seen[*value].fetch_add(1, std::memory_order_relaxed) != 0)
						corrupt.store(true, std::memory_order_relaxed);
				}
				else if(producers_left.load(std::memory_order_acquire) == 0 and
					queue.empty())
					break;
				else
					std::this_thread::yield();
			}
		});
	}

	while(ready.load(std::memory_order_acquire) != producer_count + consumer_count)
		std::this_thread::yield();
	start.store(true, std::memory_order_release);
	for(auto &thread : producers)
		thread.join();
	for(auto &thread : consumers)
		thread.join();
	LIBGS_TEST_CHECK(not corrupt.load(std::memory_order_relaxed));
	LIBGS_TEST_CHECK_EQ(consumed.load(std::memory_order_relaxed) +
		evicted.load(std::memory_order_relaxed), total_values);
	LIBGS_TEST_CHECK(queue.empty());
}

void concurrent_text_and_url_pressure()
{
	constexpr size_t thread_count = 8;
	const size_t iterations = 10'000 * scale;
	std::atomic_size_t completed {0};
	std::array<std::thread,thread_count> threads;
	for(size_t thread_index = 0; thread_index < thread_count; ++thread_index)
	{
		threads[thread_index] = std::thread([&, thread_index]
		{
			for(size_t index = 0; index < iterations; ++index)
			{
				const auto text = std::format("worker {} / item {} ? %", thread_index, index);
				const auto encoded = libgs::to_percent_encoding(text);
				LIBGS_TEST_CHECK_EQ(libgs::from_percent_encoding(encoded), text);
				libgs::url value("https://example.test:8443/api/{}/items/{}?q={}",
					thread_index, index, encoded);
				LIBGS_TEST_CHECK(value.is_valid());
				libgs::url copy(value.to_string());
				LIBGS_TEST_CHECK(copy.is_valid());
				LIBGS_TEST_CHECK_EQ(copy.to_string(), value.to_string());
			}
			completed.fetch_add(1, std::memory_order_release);
		});
	}
	for(auto &thread : threads)
		thread.join();
	LIBGS_TEST_CHECK_EQ(completed.load(std::memory_order_acquire), thread_count);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"circular queue saturation pressure", circular_queue_saturation_pressure},
		{"linked queue reclamation pressure", linked_queue_reclamation_pressure},
		{"forced queue eviction pressure", forced_eviction_pressure},
		{"concurrent text and URL pressure", concurrent_text_and_url_pressure},
	});
}
