#include <libgs/coro.h>

#include <iostream>

int main()
{
	using namespace libgs::coro::literals;

	libgs::coro::mutex mutex;
	libgs::coro::condition_variable changed;
	bool ready = false;

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		libgs::coro::unique_lock lock(mutex);
		co_await lock.lock();
		co_await changed.wait(lock, [&] { return ready; });

		std::cout << "consumer observed ready = true\n";
		libgs::exit();
	});

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		co_await 20_ms;
		libgs::coro::unique_lock lock(mutex);
		co_await lock.lock();

		ready = true;
		lock.unlock();
		changed.notify_one();
	});

	return libgs::exec();
}
