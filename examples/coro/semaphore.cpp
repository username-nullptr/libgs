// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/coro.h>

#include <algorithm>
#include <iostream>

int main()
{
	using namespace libgs::coro::literals;
	constexpr int task_count = 6;

	libgs::coro::semaphore slots(2);
	int active = 0;
	int maximum_active = 0;
	int completed = 0;

	for(int task = 0; task < task_count; ++task)
	{
		libgs::dispatch([&, task]() -> libgs::awaitable<void>
		{
			co_await slots.acquire();
			maximum_active = std::max(maximum_active, ++active);

			std::cout << "task " << task << " entered; active = " << active << '\n';
			co_await 15_ms;

			--active;
			slots.release();

			if(++completed == task_count)
			{
				std::cout << "maximum concurrency = " << maximum_active << '\n';
				libgs::exit();
			}
		});
	}
	return libgs::exec();
}
