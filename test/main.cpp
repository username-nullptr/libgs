#include <iostream>
#include <libgs/core/log.h>
#include <libgs/io/tcp_socket.h>

using namespace std::chrono;

int main()
{
#if 1
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_socket socket;

		std::error_code error;
		co_await socket.connect({"baidu.com",80}, {2s, libgs::use_awaitable_e[error]});

		libgs_log_debug("test ==== {}", error);
		libgs_exe.exit(error.value());
	});
	return libgs_exe.exec();
#endif
}
