// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/coro.h>
#include <iostream>
#include <chrono>
#include <future>

int main()
{
	using namespace std::chrono_literals;
	using namespace libgs::coro::literals;

	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		std::cout << "Coroutine started on thread " << libgs::this_thread_id() << '\n';
		co_await 20_ms;

		auto answer = std::async(std::launch::async, []
		{
			std::this_thread::sleep_for(20ms);
			return 42;
		});
		std::cout << "Future result: " << co_await libgs::coro::wait(answer) << '\n';

		co_await libgs::coro::goto_thread();
		std::cout << "Moved to worker thread " << libgs::this_thread_id() << '\n';

		libgs::exit();
	});
	return libgs::exec();
}
