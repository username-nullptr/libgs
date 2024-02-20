#include <iostream>
#include <libgs/core/log.h>
#include <libgs/core/algorithm.h>

using namespace std::chrono;

int main()
{
	std::string aaa = "hello world";
	auto res = libgs::str_replace(aaa, "or", std::string("AsD"));

	libgs_log_error("rrrrrrrrrrr {} : {}", aaa, res);

	return 0;
}
