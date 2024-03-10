#include <libgs/core/ini.h>
#include <libgs/core/log.h>

int main()
{
	libgs::ini ini("./test.ini");

#if 0
	ini["hello"]["hello"] = "hello";
	ini["hello"]["world"] = "world";
	ini["hello"]["aaa"] = 123;

	ini["test"]["aaa"] = 3.14;
	ini["test"]["bbb"] = true;

	ini.sync();
#endif

	libgs_log_debug("hello-hello: {}", ini["hello"]["hello"]);
	libgs_log_debug("hello-world: {}", ini["hello"]["world"]);
	libgs_log_debug("hello-aaa: {}", ini["hello"]["aaa"]);

	libgs_log_debug("test-aaa: {}", ini["test"]["aaa"]);
	libgs_log_debug("test-bbb: {}", ini["test"]["bbb"]);

	return 0;
}