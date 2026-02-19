#include <libgs/core/system/library.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	try {
		libgs::library library("~/liba.so");
		library.load();

		auto func_addr = library.interface("func");
		spdlog::debug("function address: {}", func_addr);

		auto func = library.interface<int(int)>("func").or_else([]{
			throw std::runtime_error("interface not found.");
		});
		int res = (*func)(111);

		spdlog::debug("call return: {}", res);
	}
	catch(std::exception &ex)
	{
		spdlog::error("{}", ex);
		return -1;
	}
	return 0;
}