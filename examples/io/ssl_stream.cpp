#include <libgs/core/log.h>
#include <libgs/io/tcp_socket.h>
#include <libgs/io/ssl_stream.h>

using namespace std::chrono_literals;

int main()
{
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::ssl_context ssl(libgs::ssl_context::sslv23_client);
		libgs::io::ssl_stream<libgs::io::tcp_socket> stream(ssl);

		// std::error_code error;
		// co_await stream.next_layer().co_connect({"127.0.0.1", 6688}, {10s, error});
		try {
//			co_await stream.next_layer().connect({"127.0.0.1", 6688}, 10s);
			co_await stream.handshake(libgs::ssl::stream_base::client, 10s);
			co_await stream.write("hello world");

			char buf[128] = "";
			auto size = co_await stream.read_some({buf,128});

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