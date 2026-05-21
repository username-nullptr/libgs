#include <libgs/http_nt/client.h>

#include <libgs/core/system/app_utls.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>

#include <libgs/coro/utils.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/process.h>
#include <libgs/utils/logger.h>

#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <memory>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs_utils_log_warning(">>>>>>>>>>>>>>>> {}", 123);


	libgs::utils::signal<void(int,std::string)> ssss;

	// // ssss.connect([](int,std::string_view)
	// // ssss.connect<libgs::utils::slot_mode::async>([](int,std::string_view)
	// ssss.connect<libgs::utils::slot_mode::backpressure>([](int,std::string_view)
	// {
	//
	// 	// co_return ;
	// });


	ssss.connect([]
	{

	});

	ssss.connect([](int,std::string_view) -> libgs::awaitable<void>
	// ssss.connect<libgs::utils::slot_mode::async>([](int,std::string_view) -> libgs::awaitable<void>
	// ssss.connect<libgs::utils::slot_mode::backpressure>([](int,std::string_view) -> libgs::awaitable<void>
	{

		co_return ;
	});

	ssss.connect (
		[](int,std::string_view) -> libgs::awaitable<void> {
			co_return ;
		},
		[](int) -> libgs::awaitable<void> {
			co_return ;
		},
		[](double) {}
	);

	libgs::utils::signal<libgs::awaitable<void>(int,std::string)> ssss1;

	ssss1.connect([](int,std::string_view) -> libgs::awaitable<void>
	// ssss.connect<libgs::utils::slot_mode::async>([](int,std::string_view) -> libgs::awaitable<void>
	// ssss.connect<libgs::utils::slot_mode::backpressure>([](int,std::string_view) -> libgs::awaitable<void>
	{

		co_return ;
	});

	ssss1.connect (
		[](int,std::string_view) -> libgs::awaitable<void> {
			co_return ;
		},
		[](int) -> libgs::awaitable<void> {
			co_return ;
		},
		[](double) {}
	);

	libgs::dispatch([&]() -> libgs::awaitable<void>
	{
		ssss.emit(11, "123");
		co_await ssss1.emit(11, "123");
		co_return ;
	});

	auto ttt = asio::awaitable<void>() and asio::awaitable<void>();

	// using co_spawn_t = decltype (
	// 	asio::co_spawn(libgs::get_executor(), asio::awaitable<void>(), asio::deferred)
	// );

	libgs::http_nt::client client;
	libgs::http_nt::request_arg arg;

#if 0
	auto context = client.request_get({"http://www.baidu.com", arg});
	auto status = context->wait_reply();
	// auto body = context->reply()->read();
	auto sum = context->reply()->save_file("./baidu.html");

	return 0;
#elif 1
	libgs::dispatch([&]() mutable -> libgs::awaitable<void>
	{
		auto context = co_await client.request_get (
			{"http://www.baidu.com", arg}, libgs::use_awaitable
		);
		auto status = co_await context->wait_reply(libgs::use_awaitable);
		// auto body = co_await context->reply()->read(libgs::use_awaitable);
		auto sum = co_await context->reply()->save_file("./baidu.html", libgs::use_awaitable);

		libgs::exit();
		co_return ;
	});
	return libgs::exec();
#endif
}