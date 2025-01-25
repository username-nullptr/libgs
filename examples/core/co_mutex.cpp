#include <libgs/core/coro.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	constexpr size_t num_threads = 8;
	asio::thread_pool pool(num_threads);

	libgs::co_mutex mutex;
	std::atomic m = 0, n = 0;

	for(size_t i=0; i<num_threads; i++)
	{
		libgs::dispatch(pool, [&]
		{
			libgs::dispatch([&, id = libgs::this_thread_id()]() -> libgs::awaitable<void>
			{
				co_await mutex.lock();

				spdlog::info("======== {} : {}", id, m++);
				co_await libgs::co_sleep_for(1s);

				mutex.unlock();
				co_return ;
			},
			libgs::use_sync);

			if( ++n == num_threads )
				libgs::exit();
		});
	}
	return libgs::exec();
}