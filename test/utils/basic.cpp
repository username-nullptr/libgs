// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/observer.h>
#include <libgs/utils/signal_slot.h>

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace
{

using namespace std::chrono_literals;

struct awaitable_sender
{
	libgs::utils::signal<libgs::awaitable<void>(std::string)> fired;
};

struct async_sender
{
	libgs::utils::signal<void(std::string)> fired;
};

struct awaitable_observer
{
	std::shared_ptr<awaitable_observer> *owner = nullptr;
	bool *destroyed = nullptr;
	int *received = nullptr;

	~awaitable_observer()
	{
		*destroyed = true;
	}

	libgs::awaitable<void> receive(int value)
	{
		auto exec = co_await asio::this_coro::executor;
		auto *shared_owner = owner;
		asio::post(exec, [shared_owner] { shared_owner->reset(); });

		asio::steady_timer timer(exec, 1ms);
		co_await timer.async_wait(asio::use_awaitable);
		*received = *destroyed ? -1 : value;
	}
};

struct synchronous_observer
{
	int *received = nullptr;

	void receive(int value)
	{
		*received = value;
	}
};

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

void awaitable_signal_lifecycle()
{
	libgs::io_context_t context;
	std::string received;
	auto result = asio::co_spawn(context, [&]() -> libgs::awaitable<void>
	{
		auto sender = std::make_unique<awaitable_sender>();
		sender->fired.connect<libgs::utils::slot_mode::sync>(
			[&](std::string value) -> libgs::awaitable<void>
			{
				auto exec = co_await asio::this_coro::executor;
				asio::steady_timer timer(exec, 1ms);
				co_await timer.async_wait(asio::use_awaitable);
				received = std::move(value);
			}
		);

		auto pending = sender->fired.emit(std::string(4096, 'x'));
		sender.reset();
		co_await std::move(pending);
	}, asio::use_future);

	context.run();
	result.get();
	LIBGS_TEST_CHECK_EQ(received, std::string(4096, 'x'));
}

void async_signal_lifecycle()
{
	libgs::io_context_t context;
	std::string received;
	auto sender = std::make_unique<async_sender>();
	sender->fired.connect<libgs::utils::slot_mode::async>(
		context, [&](std::string value) { received = std::move(value); }
	);

	sender->fired.emit(std::string(4096, 'y'));
	sender.reset();
	context.run();
	LIBGS_TEST_CHECK_EQ(received, std::string(4096, 'y'));
}

void signal_observer_lifecycle()
{
	libgs::io_context_t context;
	libgs::utils::signal<void(int)> fired;
	bool destroyed = false;
	int received = 0;

	auto observer = std::make_shared<awaitable_observer>();
	observer->owner = &observer;
	observer->destroyed = &destroyed;
	observer->received = &received;
	fired.connect<libgs::utils::slot_mode::async>(
		observer, context, &awaitable_observer::receive
	);

	fired.emit(9);
	context.run();
	LIBGS_TEST_CHECK(not observer);
	LIBGS_TEST_CHECK(destroyed);
	LIBGS_TEST_CHECK_EQ(received, 9);
}

void expired_signal_observer()
{
	libgs::io_context_t context;
	libgs::utils::signal<void(int)> fired;
	int received = 0;
	auto observer = std::make_shared<synchronous_observer>();
	observer->received = &received;
	fired.connect<libgs::utils::slot_mode::async>(
		observer, context, &synchronous_observer::receive
	);

	fired.emit(9);
	observer.reset();
	context.run();
	LIBGS_TEST_CHECK_EQ(received, 0);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"synchronous signal", synchronous_signal},
		{"observer lifecycle", observer_lifecycle},
		{"awaitable signal lifecycle", awaitable_signal_lifecycle},
		{"async signal lifecycle", async_signal_lifecycle},
		{"signal observer lifecycle", signal_observer_lifecycle},
		{"expired signal observer", expired_signal_observer},
	});
}
