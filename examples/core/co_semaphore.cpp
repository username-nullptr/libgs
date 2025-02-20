#include <libgs/core/coro.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	constexpr size_t count = 8;
	libgs::co_semaphore semaphore(3);
	size_t j = 0;

	for(size_t i=0; i<count; i++)
	{
		libgs::dispatch([&, id = libgs::this_thread_id()]() -> libgs::awaitable<void>
		{
			co_await semaphore.acquire();

			spdlog::info("======== {} : {}", id, j++);
			co_await libgs::co_sleep_for(1s);

			semaphore.release();

			if( j == count )
				libgs::exit();
			co_return ;
		});
	}
	return libgs::exec();
}