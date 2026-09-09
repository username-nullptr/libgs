// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_TEST_PERFORMANCE_BENCHMARK_H
#define LIBGS_TEST_PERFORMANCE_BENCHMARK_H

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string_view>

namespace libgs::test
{

inline void print_performance_result(
	std::string_view name,
	size_t operations,
	std::chrono::steady_clock::duration elapsed,
	std::string_view unit
)
{
	const auto seconds = std::chrono::duration<double>(elapsed).count();
	const auto nanoseconds_per_operation =
		std::chrono::duration<double,std::nano>(elapsed).count() /
		static_cast<double>(operations);
	const auto operations_per_second = static_cast<double>(operations) / seconds;

	std::cout << std::fixed << std::setprecision(2)
		<< "[PERF] " << name
		<< ": " << operations_per_second << ' ' << unit << "/s, "
		<< nanoseconds_per_operation << " ns/" << unit << ", "
		<< std::chrono::duration<double,std::milli>(elapsed).count() << " ms\n";
}

} //namespace libgs::test

#endif //LIBGS_TEST_PERFORMANCE_BENCHMARK_H
