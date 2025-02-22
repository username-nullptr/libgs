// #include <libgs.h>
#include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client.h>
#include <libgs/http/client/request.h>

#include <list>
#include <iostream>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::http::request_arg req_arg;
	req_arg
	.set_header({
		{ "111", "222" },
		{ "333", "444" },
		{ "555", "666" }
	})
	.set_cookie({
		{ "aaa", "bbb" },
		{ "ccc", "ddd" },
		{ "eee", "fff" }
	});

	libgs::http::request_helper_v11 helper(std::move(req_arg));
	const char body[] = "hello world";

	auto buf = helper.header_data<libgs::http::method::GET>(sizeof(body));
	std::cout << "header data: " << buf << "\n" << std::endl;

	buf = helper.body_data(body);
	std::cout << "body data: " << buf << std::endl;

#if 1
	buf = helper.chunk_end_data({"111", "234"});
	buf = helper.chunk_end_data(std::make_pair("111","222"));
	buf = helper.chunk_end_data(std::make_tuple("111","222"));
	buf = helper.chunk_end_data({{"111","222"},{"333","444"}});
	buf = helper.chunk_end_data({std::make_pair("111","222"),std::make_pair("333","444")});
	buf = helper.chunk_end_data({std::make_tuple("111","222"),std::make_tuple("333","444")});
#endif

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
