#include <libgs/core/ini.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::ini ini("./test.ini");

#if 0
	ini["hello"]["hello"] = "hello";
	ini["hello"]["world"] = "world";
	ini["hello"]["aaa"] = 123;

	ini["test"]["aaa"] = 3.14;
	ini["test"]["bbb"] = true;

	ini.sync();
#endif

	spdlog::debug("hello-hello: {}", ini["hello"]["hello"]);
	spdlog::debug("hello-world: {}", ini["hello"]["world"]);
	spdlog::debug("hello-aaa: {}", ini["hello"]["aaa"]);

	spdlog::debug("test-aaa: {}", ini["test"]["aaa"]);
	spdlog::debug("test-bbb: {}", ini["test"]["bbb"]);

	return 0;
}