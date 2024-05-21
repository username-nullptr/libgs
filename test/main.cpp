#include <libgs/core/log.h>
#include <libgs/io/ssl_tcp_socket.h>
#include <libgs/io/timer.h>

using namespace std::chrono_literals;

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::ssl::context sslc(libgs::ssl::context::sslv23_client);
		libgs::io::ssl_stream<libgs::io::tcp_socket> ssl_stream(sslc);

		// std::error_code error;
		// co_await socket.co_connect({"127.0.0.1", 6688}, {10s, error});
		try {
			co_await ssl_stream.next_layer().connect({"192.168.10.108", 6688}, 10s);
			co_await ssl_stream.handshake(libgs::ssl::stream_base::client, 10s);
			co_await ssl_stream.write("hello world");

			char buf[128] = "";
			auto size = co_await ssl_stream.read_some({buf,128});

			libgs_log_debug("tcp_socket read: {}.", std::string_view(buf,size));
			libgs::execution::exit();
		}
		catch(std::exception &ex)
		{
			libgs_log_error("tcp_socket error: {}.", ex);
			libgs::execution::exit(-1);
		}
		co_return ;
	});
	return libgs::execution::exec();
}
