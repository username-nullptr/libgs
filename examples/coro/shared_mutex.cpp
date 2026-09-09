// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/coro.h>

#include <iostream>

int main()
{
	using namespace libgs::coro::literals;

	libgs::coro::shared_mutex mutex;
	int value = 0;

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		libgs::coro::shared_unique_lock lock(mutex);
		co_await lock.lock();
		value = 42;
		co_return;
	});

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		co_await 10_ms;
		libgs::coro::shared_lock lock(mutex);
		co_await lock.lock_shared();

		std::cout << "Shared read: " << value << '\n';
		libgs::exit();
		co_return;
	});

	return libgs::exec();
}
