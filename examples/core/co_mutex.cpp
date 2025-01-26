#include <libgs/core/coro.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	constexpr size_t count = 8;
	libgs::co_mutex mutex;
	size_t j = 0;

	for(size_t i=0; i<count; i++)
	{
		libgs::dispatch([&, id = libgs::this_thread_id()]() -> libgs::awaitable<void>
		{
			// co_await mutex.lock();
			libgs::co_unique_lock locker(mutex);
			co_await locker.lock();

			spdlog::info("======== {} : {}", id, j++);
			co_await libgs::co_sleep_for(1s);

			// mutex.unlock();

			if( j == count )
				libgs::exit();
			co_return ;
		});
	}
	return libgs::exec();
}