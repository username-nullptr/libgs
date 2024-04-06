#include <libgs/core/coroutine.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
	asio::thread_pool pool;
	libgs::co_spawn_detached([&]() -> asio::awaitable<void>
	{
		co_await (libgs::co_spawn([&]() -> asio::awaitable<void>
		{
			std::atomic_bool run {true};
			auto state = co_await asio::this_coro::cancellation_state;

			state.slot().assign([&](asio::cancellation_type) {
				run = false;
			});
			for(int i=0; i<10 and run; i++)
			{
				std::cout << "i = " << i << std::endl;
				std::this_thread::sleep_for(500ms);
			}
			std::cout << "ccccccccccccc" << std::endl;
			state.clear(asio::cancellation_type::all);
			co_return ;
		},
		pool) or libgs::co_sleep_for(1s));

		std::cout << "end" << std::endl;
		libgs::execution::exit();
		co_return ;
	});
	libgs::execution::exec();
	return 0;
}
