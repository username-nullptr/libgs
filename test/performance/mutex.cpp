// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/core/atomic_mutex.h>
#include <libgs/core/shared_mutex.h>

#include <algorithm>
#include <array>
#include <barrier>
#include <cstdlib>
#include <format>
#include <limits>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <thread>
#include <vector>

namespace
{

#ifdef NDEBUG
constexpr size_t iterations = 100'000 * libgs::test::performance_scale;
#else
constexpr size_t iterations = 10'000 * libgs::test::performance_scale;
#endif

constexpr size_t sample_count = 3;

template <typename Func>
std::chrono::steady_clock::duration median_duration(Func &&func)
{
	func();
	std::array<std::chrono::steady_clock::duration,sample_count> samples {};
	for(auto &sample : samples)
		sample = func();
	std::ranges::sort(samples);
	return samples[sample_count / 2];
}

template <typename Mutex, bool Shared = false>
std::chrono::steady_clock::duration measure_uncontended()
{
	Mutex mutex;
	const auto begin = std::chrono::steady_clock::now();
	for(size_t index = 0; index < iterations; ++index)
	{
		if constexpr( Shared )
		{
			mutex.lock_shared();
			mutex.unlock_shared();
		}
		else
		{
			mutex.lock();
			mutex.unlock();
		}
	}
	return std::chrono::steady_clock::now() - begin;
}

template <typename Mutex>
std::chrono::steady_clock::duration measure_exclusive(
	size_t thread_count, size_t work
)
{
	Mutex mutex;
	size_t value = 0;
	std::barrier start(static_cast<std::ptrdiff_t>(thread_count + 1));
	std::vector<std::thread> workers;
	workers.reserve(thread_count);
	for(size_t thread = 0; thread < thread_count; ++thread)
	{
		workers.emplace_back([&]
		{
			start.arrive_and_wait();
			for(size_t index = 0; index < iterations; ++index)
			{
				std::lock_guard lock(mutex);
				++value;
				for(size_t count = 0; count < work; ++count)
					libgs::none_instruction();
			}
		});
	}
	const auto begin = std::chrono::steady_clock::now();
	start.arrive_and_wait();
	for(auto &worker : workers)
		worker.join();
	const auto elapsed = std::chrono::steady_clock::now() - begin;
	LIBGS_TEST_CHECK_EQ(value, thread_count * iterations);
	return elapsed;
}

template <typename Mutex>
std::chrono::steady_clock::duration measure_shared(
	size_t thread_count, size_t write_interval
)
{
	Mutex mutex;
	size_t value = 0;
	std::barrier start(static_cast<std::ptrdiff_t>(thread_count + 1));
	std::vector<std::thread> workers;
	workers.reserve(thread_count);
	for(size_t thread = 0; thread < thread_count; ++thread)
	{
		workers.emplace_back([&, thread]
		{
			size_t checksum = 0;
			start.arrive_and_wait();
			for(size_t index = 0; index < iterations; ++index)
			{
				if( write_interval != 0 and (index + thread) % write_interval == 0 )
				{
					std::unique_lock lock(mutex);
					++value;
				}
				else
				{
					std::shared_lock lock(mutex);
					checksum += value;
				}
			}
			if( checksum == std::numeric_limits<size_t>::max() )
				std::abort();
		});
	}
	const auto begin = std::chrono::steady_clock::now();
	start.arrive_and_wait();
	for(auto &worker : workers)
		worker.join();
	return std::chrono::steady_clock::now() - begin;
}

void mutex_contention()
{
	libgs::test::print_performance_result(
		"std::mutex uncontended", iterations,
		median_duration(measure_uncontended<std::mutex>), "lock"
	);
	libgs::test::print_performance_result(
		"atomic_mutex uncontended", iterations,
		median_duration(measure_uncontended<libgs::atomic_mutex>), "lock"
	);
	libgs::test::print_performance_result(
		"spin_mutex uncontended", iterations,
		median_duration(measure_uncontended<libgs::spin_mutex>), "lock"
	);
	for(const size_t threads : {2U, 4U, 8U})
	{
		for(const size_t work : {0U, 16U})
		{
			libgs::test::print_performance_result(
				std::format("std::mutex {} threads, work {}", threads, work),
				threads * iterations,
				median_duration([=] {
					return measure_exclusive<std::mutex>(threads, work);
				}), "lock"
			);
			libgs::test::print_performance_result(
				std::format("atomic_mutex {} threads, work {}", threads, work),
				threads * iterations,
				median_duration([=] {
					return measure_exclusive<libgs::atomic_mutex>(threads, work);
				}), "lock"
			);
			libgs::test::print_performance_result(
				std::format("spin_mutex {} threads, work {}", threads, work),
				threads * iterations,
				median_duration([=] {
					return measure_exclusive<libgs::spin_mutex>(threads, work);
				}), "lock"
			);
		}
	}
}

void shared_mutex_contention()
{
	for(const bool shared : {false, true})
	{
		const auto suffix = shared ? "shared" : "exclusive";
		libgs::test::print_performance_result(
			std::format("std::shared_mutex uncontended {}", suffix), iterations,
			median_duration([=] {
				return shared ? measure_uncontended<std::shared_mutex,true>() :
					measure_uncontended<std::shared_mutex>();
			}), "lock"
		);
		libgs::test::print_performance_result(
			std::format("atomic_shared_mutex uncontended {}", suffix), iterations,
			median_duration([=] {
				return shared ? measure_uncontended<libgs::atomic_shared_mutex,true>() :
					measure_uncontended<libgs::atomic_shared_mutex>();
			}), "lock"
		);
		libgs::test::print_performance_result(
			std::format("spin_shared_mutex uncontended {}", suffix), iterations,
			median_duration([=] {
				return shared ? measure_uncontended<libgs::spin_shared_mutex,true>() :
					measure_uncontended<libgs::spin_shared_mutex>();
			}), "lock"
		);
	}
	for(const size_t threads : {2U, 4U, 8U})
	{
		for(const size_t write_interval : {size_t {0}, size_t {1}, size_t {10}})
		{
			libgs::test::print_performance_result(
				std::format("std::shared_mutex {} threads, write interval {}",
					threads, write_interval),
				threads * iterations,
				median_duration([=] {
					return measure_shared<std::shared_mutex>(threads, write_interval);
				}), "lock"
			);
			libgs::test::print_performance_result(
				std::format("atomic_shared_mutex {} threads, write interval {}",
					threads, write_interval),
				threads * iterations,
				median_duration([=] {
					return measure_shared<libgs::atomic_shared_mutex>(threads, write_interval);
				}), "lock"
			);
			libgs::test::print_performance_result(
				std::format("spin_shared_mutex {} threads, write interval {}",
					threads, write_interval),
				threads * iterations,
				median_duration([=] {
					return measure_shared<libgs::spin_shared_mutex>(threads, write_interval);
				}), "lock"
			);
		}
	}
}

} //namespace

int main()
{
	// Keep glibc's single-thread-only pthread fast path out of the comparison.
	std::thread([] {}).join();
	return libgs::test::run({
		{"mutex contention", mutex_contention},
		{"shared mutex contention", shared_mutex_contention},
	});
}
