// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>

#include <spdlog/spdlog.h>
#include <iostream>

using namespace std::chrono_literals;
using namespace libgs::operators;

void fff(int i)
{
	std::cout << "2222: " << i << std::endl;
}

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::utils::signal<void(int,const char*)> sig;

	asio::io_context ioc;

	sig
	.connect (
		[](bool i, std::string_view d)
		{
			std::cout << "0000: " << i << " " << d << std::endl;
		},
		[](bool i, bool d) -> libgs::awaitable<void>
		{
			std::cout << "1111: " << i << " " << d << std::endl;
			co_return ;
		}
	)
	.connect(fff);

	sig(11, "hello");

	sig.disconnect(fff);

	sig(22, "world");

	return libgs::exec();
}