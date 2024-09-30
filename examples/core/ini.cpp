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
	ini["test"]["测"] = "aaa 你 bbb 好 ccc";

	ini.sync();
#endif

	spdlog::debug("hello-hello: {}", ini["hello"]["hello"]);
	spdlog::debug("hello-world: {}", ini["hello"]["world"]);
	spdlog::debug("hello-aaa: {}", ini["hello"]["aaa"]);

	spdlog::debug("test-aaa: {}", ini["test"]["aaa"]);
	spdlog::debug("test-bbb: {}", ini["test"]["bbb"]);
	spdlog::debug("test-测: {}", ini["test"]["测"]);

	return 0;
}