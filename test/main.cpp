// #include <libgs.h>
#include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client/request.h>
// #include <libgs/http/cxx/socket_session.h>

// #include <libgs/http/client.h>
#include <list>
#include <iostream>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	asio::io_context ioc;
	asio::thread_pool pool;
	asio::ip::tcp::socket socket(libgs::execution::context());

	libgs::dispatch(ioc, [&]() -> libgs::awaitable<void>
	{
		std::cout << "000000000000 : " << libgs::this_thread_id() << std::endl;
		co_await libgs::co_to_exec();

		std::cout << "1111111111 : " << libgs::this_thread_id() << std::endl;
		co_await libgs::co_to_exec(pool);

		std::cout << "2222222222222 : " << libgs::this_thread_id() << std::endl;

		std::error_code error;
		co_await socket.async_connect(
			{asio::ip::address::from_string("127.0.0.1"), 111},
			libgs::use_awaitable | error
		);

		std::cout << "aaaaaaaaaaaaaa : " << libgs::this_thread_id() << std::endl;

		co_return ;
	});

	std::thread thread([&]
	{
		asio::io_context::work worker(ioc); LIBGS_UNUSED(worker);
		ioc.run();
	});
	int res = libgs::execution::exec();
	ioc.stop();
	thread.join();
	return res;

	// spdlog::set_level(spdlog::level::trace);

	// std::list l0 { 0,1,2,3,4 };
	// std::list l1 { 5,6,7,8,9 };
	//
	// std::cout << "l0: ";
	// for(auto &i : l0)
	// 	std::cout << i << " ";
	// std::cout << std::endl;
	//
	// std::cout << "l1: ";
	// for(auto &i : l1)
	// 	std::cout << i << " ";
	// std::cout << std::endl;
	//
	// l0.splice(l0.end(), l1);
	//
	// std::cout << "l0: ";
	// for(auto &i : l0)
	// 	std::cout << i << " ";
	// std::cout << std::endl;
	//
	// std::cout << "l1: ";
	// for(auto &i : l1)
	// 	std::cout << i << " ";
	// std::cout << std::endl;

	// return libgs::execution::exec();
	return 0;
}
