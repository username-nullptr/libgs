// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client/request.h>
// #include <libgs/http/cxx/socket_session.h>

#include <libgs/http/opt_token.h>
#include <list>
#include <iostream>

// using namespace std::chrono_literals;


int main()
{
	std::fstream f0;
	libgs::http::file_range r0, r1, r2;
	libgs::http::file_ranges rs0, rs1;

	using namespace libgs::http::operators;

	auto aaa = f0 | r0 | r1 | r2 | rs0 | rs1;


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
