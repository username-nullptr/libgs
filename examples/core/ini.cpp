#include <libgs/core/ini.h>
#include <spdlog/spdlog.h>

#ifdef _MSC_VER
# pragma warning(disable:4819)
#endif

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::ini ini("./test.ini");

#if 1
	ini["hello"]["hello"] = "hello";
	ini["hello"]["world"] = "world";
	ini["hello"]["aaa"] = 123;

	ini["test"]["aaa"] = 3.14;
	ini["test"]["bbb"] = true;

#ifndef _MSC_VER
	ini["test"]["测"] = "aaa 你 bbb 好 ccc";
#endif
	ini.sync();
	ini.sync([](std::error_code error){});
	ini.sync(libgs::use_awaitable);
	ini.sync(libgs::use_future);
	ini.sync(libgs::deferred);
	ini.sync(libgs::detached);
#endif

	spdlog::debug("hello-hello: {}", ini["hello"]["hello"]);
	spdlog::debug("hello-world: {}", ini["hello"]["world"]);
	spdlog::debug("hello-aaa: {}", ini["hello"]["aaa"]);

	spdlog::debug("test-aaa: {}", ini["test"]["aaa"]);
	spdlog::debug("test-bbb: {}", ini["test"]["bbb"]);

#ifndef _MSC_VER
	spdlog::debug("test-测: {}", ini["test"]["测"]);
#endif
	return 0;
}