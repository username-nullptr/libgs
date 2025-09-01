#include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/app_utls.h>
#include <iostream>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	asio::ip::tcp::socket socket(libgs::io_context());

	auto coro_task = [&]() -> libgs::awaitable<libgs::sys_expected<>>
	{
		libgs::error_code error;
		co_await socket.async_connect (
			{asio::ip::make_address("127.0.0.1"), 80},
			libgs::use_awaitable | error
		);
		co_return error ?
			libgs::sys_expected<void>(error) :
			libgs::sys_expected();
	};

	auto sync_task = [&]
	{
		libgs::error_code error;
		socket.connect({asio::ip::make_address("127.0.0.1"), 80}, error);
		return error ?
			libgs::sys_expected<void>(error) :
			libgs::sys_expected();
	};

	libgs::http::io_task<void> task(std::move(coro_task()), std::move(sync_task));

	// task
	// .transform([]
	// {
	// 	std::cout << "0000000000000" << std::endl;
	// })
	// .and_then([]
	// {
	// 	std::cout << "1111111111111111111" << std::endl;
	// 	return libgs::sys_expected();
	// })
	// .or_else([]
	// {
	// 	std::cout << "22222222222222" << std::endl;
	// });

	// libgs::dispatch([&]() -> libgs::awaitable<void>
	// {
	// 	(co_await task.coro())
	// 	.transform([]
	// 	{
	// 		std::cout << "0000000000000" << std::endl;
	// 	})
	// 	.and_then([]
	// 	{
	// 		std::cout << "1111111111111111111" << std::endl;
	// 		return libgs::sys_expected();
	// 	})
	// 	.or_else([]
	// 	{
	// 		std::cout << "22222222222222" << std::endl;
	// 	});
	// 	libgs::exit(0);
	// });

	task.async([](libgs::sys_expected<> expected)
	{
		expected
		.transform([]
		{
			std::cout << "0000000000000" << std::endl;
		})
		.and_then([]
		{
			std::cout << "1111111111111111111" << std::endl;
			return libgs::sys_expected();
		})
		.or_else([]
		{
			std::cout << "22222222222222" << std::endl;
		});
		libgs::exit(0);
	});

	// libgs::dispatch([&]() -> libgs::awaitable<void>
	// {
	// 	auto asd = task.async();
	// 	co_return ;
	// });
	return libgs::exec();
}
