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

void aaa(libgs::concepts::async_tf_opt_token<size_t,std::error_code> auto &&token)
{

}

void callback(size_t)
{

}

int main()
{
	std::error_code error;

	aaa(callback | 111ms);

	return 0;

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
