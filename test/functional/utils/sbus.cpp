// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/sbus.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

namespace
{

using namespace std::chrono_literals;

bool wait_for_count(const std::atomic_size_t &count, size_t expected)
{
	for(int retry = 0; retry < 2'000 and count.load() < expected; ++retry)
		std::this_thread::sleep_for(1ms);
	return count.load() == expected;
}

void delivery_and_cancellation()
{
	constexpr std::string_view topic = "libgs.test.sbus.delivery";
	auto interface = std::make_shared<libgs::utils::sbus::local_interface>();
	std::atomic_size_t topic_received = 0;
	std::atomic_size_t global_received = 0;

	const auto topic_sid = interface->subscribe(topic, [&](const void*, size_t) {
		topic_received.fetch_add(1);
	});
	const auto global_sid = interface->subscribe(
	[&](std::string_view, const void*, size_t) {
		global_received.fetch_add(1);
	});

	interface->cancel_sid(global_sid);
	for(size_t batch = 1; batch <= 100; ++batch)
	{
		for(size_t index = 0; index < 64; ++index)
			libgs::utils::sbus::publish(topic, index);
		LIBGS_TEST_CHECK(wait_for_count(topic_received, batch * 64));
	}
	LIBGS_TEST_CHECK_EQ(global_received.load(), 0);

	interface->cancel_sid(topic_sid);
	libgs::utils::sbus::publish(topic, 42);
	std::this_thread::sleep_for(5ms);
	LIBGS_TEST_CHECK_EQ(topic_received.load(), 6'400);
}

void large_payload_fanout_owns_one_copy()
{
	constexpr std::string_view topic = "libgs.test.sbus.large-payload";
	constexpr size_t payload_size = 64 * 1'024;
	auto interface = std::make_shared<libgs::utils::sbus::local_interface>();
	std::atomic_size_t received {0};
	std::atomic<const void*> delivered_data {nullptr};
	std::atomic_bool invalid {false};
	std::vector<std::byte> payload(payload_size, std::byte {0x2a});
	const auto *caller_data = payload.data();

	auto callback = [&](const void *data, size_t size)
	{
		if( size != payload_size or data == caller_data )
			invalid.store(true, std::memory_order_relaxed);
		else
		{
			auto expected = static_cast<const void*>(nullptr);
			if( not delivered_data.compare_exchange_strong(expected, data) and expected != data )
				invalid.store(true, std::memory_order_relaxed);
			auto bytes = static_cast<const std::byte*>(data);
			if( bytes[0] != std::byte {0x2a} or bytes[size - 1] != std::byte {0x2a} )
				invalid.store(true, std::memory_order_relaxed);
		}
		received.fetch_add(1, std::memory_order_release);
	};
	interface->subscribe(topic, callback);
	interface->subscribe(topic, callback);

	libgs::utils::sbus::publish(topic, payload.data(), payload.size());
	payload.clear();
	payload.shrink_to_fit();
	LIBGS_TEST_CHECK(wait_for_count(received, 2));
	LIBGS_TEST_CHECK(not invalid.load(std::memory_order_relaxed));
	interface->cancel();
}

void indexed_topics_and_empty_topic_cancellation()
{
	constexpr std::string_view target_topic = "libgs.test.sbus.indexed-target";
	std::vector<std::shared_ptr<libgs::utils::sbus::local_interface>> unrelated;
	std::atomic_size_t unrelated_received {0};
	unrelated.reserve(32);
	for(size_t index = 0; index < 32; ++index)
	{
		auto interface = std::make_shared<libgs::utils::sbus::local_interface>();
		interface->subscribe(
			"libgs.test.sbus.unrelated." + std::to_string(index),
			[&](const void*, size_t) { unrelated_received.fetch_add(1); }
		);
		unrelated.emplace_back(std::move(interface));
	}

	auto target = std::make_shared<libgs::utils::sbus::local_interface>();
	std::atomic_size_t target_received {0};
	const auto target_sid = target->subscribe(target_topic,
	[&](const void*, size_t) {
		target_received.fetch_add(1, std::memory_order_release);
	});

	libgs::utils::sbus::publish(target_topic, 42);
	LIBGS_TEST_CHECK(wait_for_count(target_received, 1));
	std::this_thread::sleep_for(5ms);
	LIBGS_TEST_CHECK_EQ(unrelated_received.load(), 0U);

	target->cancel_sid(target_sid);
	libgs::utils::sbus::publish(target_topic, 43);
	std::this_thread::sleep_for(5ms);
	LIBGS_TEST_CHECK_EQ(target_received.load(), 1U);

	// Topic "" and a global subscription are distinct index entries.
	auto empty = std::make_shared<libgs::utils::sbus::local_interface>();
	std::atomic_size_t empty_received {0};
	std::atomic_size_t global_received {0};
	const auto empty_sid = empty->subscribe("", [&](const void*, size_t) {
		empty_received.fetch_add(1, std::memory_order_release);
	});
	empty->subscribe([&](std::string_view, const void*, size_t) {
		global_received.fetch_add(1, std::memory_order_release);
	});

	libgs::utils::sbus::publish("", 1);
	LIBGS_TEST_CHECK(wait_for_count(empty_received, 1));
	LIBGS_TEST_CHECK(wait_for_count(global_received, 1));
	empty->cancel_sid(empty_sid);
	libgs::utils::sbus::publish("", 2);
	LIBGS_TEST_CHECK(wait_for_count(global_received, 2));
	std::this_thread::sleep_for(5ms);
	LIBGS_TEST_CHECK_EQ(empty_received.load(), 1U);

	empty->cancel();
	target->cancel();
	for(auto &interface : unrelated)
		interface->cancel();
}

} //namespace

int main()
{
	return libgs::test::run({
		{"sbus delivery and cancellation", delivery_and_cancellation},
		{"sbus large payload fanout owns one copy", large_payload_fanout_owns_one_copy},
		{"sbus indexed topics and empty-topic cancellation", indexed_topics_and_empty_topic_cancellation},
	});
}
