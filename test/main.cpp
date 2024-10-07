// #include <libgs.h>
#include <libgs/core.h>
#include <spdlog/spdlog.h>

#include <libgs/http/client/session_pool.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	// libgs::http::session_pool spool(ioc);
	asio::thread_pool pool(16);

	libgs::co_spawn_detached([&]() -> asio::awaitable<void>
	{
		asio::cancellation_signal signal;
		asio::steady_timer hht(libgs::execution::get_executor(), 3s);

		hht.async_wait([&signal](const std::error_code& error)
		{
			signal.emit(asio::cancellation_type::all);
		});
		std::error_code tte;
		co_await asio::co_spawn(pool, [&]() -> asio::awaitable<void>
		{
			spdlog::info("000000000000000000");

			asio::steady_timer timer(libgs::execution::get_executor(), 60s);
			std::error_code error;
			co_await timer.async_wait(libgs::use_awaitable | error);

			spdlog::info("111111111111111111 {}", error);
			co_return ;
		},
		asio::use_awaitable | signal.slot() | tte);
		spdlog::info("-----------------------)))- {}", tte);

		libgs::execution::exit(0);
		co_return ;
	});
	return libgs::execution::exec();
//	return 0;
}
