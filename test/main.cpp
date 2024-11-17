// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client/request.h>
// #include <libgs/http/cxx/socket_session.h>

#include <list>
#include <iostream>

// using namespace std::chrono_literals;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	std::list<int> l0 { 0,1,2,3,4 };

	std::list<int> l1 { 5,6,7,8,9 };

	std::cout << "l0: ";
	for(auto &i : l0)
		std::cout << i << " ";
	std::cout << std::endl;

	std::cout << "l1: ";
	for(auto &i : l1)
		std::cout << i << " ";
	std::cout << std::endl;

	l0.splice(l0.end(), l1);

	std::cout << "l0: ";
	for(auto &i : l0)
		std::cout << i << " ";
	std::cout << std::endl;

	std::cout << "l1: ";
	for(auto &i : l1)
		std::cout << i << " ";
	std::cout << std::endl;

	// return libgs::execution::exec();
	return 0;
}
