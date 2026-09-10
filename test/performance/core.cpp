// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "benchmark.h"
#include "test.h"

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/algorithm/sha1.h>
#include <libgs/core/url.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <string>

namespace
{

#ifdef NDEBUG
constexpr size_t text_cycle_count = 100'000;
constexpr size_t url_cycle_count = 100'000;
constexpr size_t hash_cycle_count = 20'000;
#else
constexpr size_t text_cycle_count = 5'000;
constexpr size_t url_cycle_count = 5'000;
constexpr size_t hash_cycle_count = 1'000;
#endif

using duration_t = std::chrono::steady_clock::duration;

template <typename Func>
duration_t median_duration(size_t count, Func &&func)
{
	func(std::max<size_t>(1, count / 20));
	std::array<duration_t,3> samples {};
	for(auto &sample : samples)
		sample = func(count);
	std::ranges::sort(samples);
	return samples[1];
}

duration_t measure_percent_encoding(size_t count)
{
	std::string source(1024, 'a');
	for(size_t index = 7; index < source.size(); index += 16)
		source[index] = ' ';
	size_t checksum = 0;
	const auto begin = std::chrono::steady_clock::now();
	for(size_t index = 0; index < count; ++index)
	{
		source[0] = static_cast<char>('a' + index % 26);
		auto encoded = libgs::to_percent_encoding(source);
		auto decoded = libgs::from_percent_encoding(std::move(encoded));
		checksum += decoded.size() + static_cast<unsigned char>(decoded[0]);
	}
	const auto elapsed = std::chrono::steady_clock::now() - begin;
	LIBGS_TEST_CHECK(checksum > count * source.size());
	return elapsed;
}

duration_t measure_url_round_trip(size_t count)
{
	size_t checksum = 0;
	const auto begin = std::chrono::steady_clock::now();
	for(size_t index = 0; index < count; ++index)
	{
		libgs::url value("https://example.test:8443/api/items?q={}&flag", index);
		LIBGS_TEST_CHECK(value.is_valid());
		auto text = value.to_string();
		checksum += text.size() + value.parameters().size();
	}
	const auto elapsed = std::chrono::steady_clock::now() - begin;
	LIBGS_TEST_CHECK(checksum > count);
	return elapsed;
}

duration_t measure_sha1(size_t count)
{
	std::string payload(4096, 'x');
	size_t checksum = 0;
	const auto begin = std::chrono::steady_clock::now();
	for(size_t index = 0; index < count; ++index)
	{
		payload[0] = static_cast<char>(index);
		auto digest = libgs::sha1(payload).finalize().hex(false);
		checksum += digest.size() + static_cast<unsigned char>(digest[0]);
	}
	const auto elapsed = std::chrono::steady_clock::now() - begin;
	LIBGS_TEST_CHECK(checksum > count * 40);
	return elapsed;
}

void core_hot_paths()
{
	libgs::test::print_performance_result(
		"core/percent encode + decode 1 KiB (median of 3)", text_cycle_count,
		median_duration(text_cycle_count, measure_percent_encoding), "cycle"
	);
	libgs::test::print_performance_result(
		"core/URL parse + serialize (median of 3)", url_cycle_count,
		median_duration(url_cycle_count, measure_url_round_trip), "cycle"
	);
	libgs::test::print_performance_result(
		"core/SHA-1 4 KiB (median of 3)", hash_cycle_count,
		median_duration(hash_cycle_count, measure_sha1), "digest"
	);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"core hot paths", core_hot_paths},
	});
}
