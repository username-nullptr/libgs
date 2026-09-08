// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/coro.h>

#include <iostream>

int main()
{
	using namespace libgs::coro::literals;
	constexpr int worker_count = 4;

	libgs::coro::mutex mutex;
	int next_value = 0;
	int completed = 0;

	for(int worker = 0; worker < worker_count; ++worker)
	{
		libgs::dispatch([&, worker]() -> libgs::awaitable<void>
		{
			libgs::coro::unique_lock lock(mutex);
			co_await lock.lock();

			const auto value = next_value++;
			co_await 10_ms;

			std::cout << "worker " << worker << " observed " << value << '\n';
			lock.unlock();

			if(++completed == worker_count)
				libgs::exit();
		});
	}
	return libgs::exec();
}
