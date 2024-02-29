#include <libgs/core/log.h>
#include <libgs/io/timer.h>
#include <libgs/io/tcp_server.h>

using namespace std::chrono;

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::io::tcp_socket sock;
		std::error_code error;

		try {
			co_await sock.connect({"172.22.190.254", 6688}, libgs::use_awaitable);
			co_await sock.write("Hello world!", libgs::use_awaitable);

			char buf[128] = "";
			co_await sock.read({buf,128}, {"ww", 2s, libgs::use_awaitable});

			libgs_log_debug("recv: {}", buf);
		}
		catch(std::exception &ex) {
			libgs_log_error("00000000000 : {}", ex);
		}
		libgs_exe.exit();
	});
	return libgs_exe.exec();
}
