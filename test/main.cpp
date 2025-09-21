// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/logger.h>
#include <iostream>
#include <memory>

using namespace std::chrono_literals;
using namespace libgs::operators;

void fff(int i)
{
	libgs_utils_log_info("2222: {}", i);
}

int main()
{
	auto timer = libgs::make_timer(500ms, []() -> libgs::awaitable<void>
	{
		libgs_utils_log_info("------------------");
		co_return ;
	});
	libgs::post(2.5s, [&timer]() mutable {
		timer.reset();
	});

	libgs::utils::signal<void(int,const char*)> sig;
	asio::io_context ioc;

	auto obj = std::make_shared<int>(int {0});

	sig
	.connect<libgs::utils::slot_mode::async>(
		[](bool i, std::string_view d)
		{
			libgs_utils_log_info("0000: {} {}", i, d);
		},
		[](bool i, bool d) -> libgs::awaitable<void>
		{
			libgs_utils_log_info("1111: {} {}", i, d);
			co_return ;
		}
	)
	.connect(fff)
	.connect(ioc, [](float i)
	{
		libgs_utils_log_info("3333: {}", i);
	})
	.connect(obj, [](int i, const char *d)
	{
		libgs_utils_log_info("444: {} {}", i, d);
	});

	sig(11, "hello");

	sig.disconnect(fff);

	libgs::post([&]{
		obj.reset();
		sig(22, "world");
	});

	std::thread([&] {
		ioc.run();
	}).detach();

	using namespace std::chrono_literals;
	libgs::post(5s, []{
		libgs::exit();
	});
	return libgs::exec();
}