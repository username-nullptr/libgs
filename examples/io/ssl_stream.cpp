#include <libgs/io/ssl_tcp_socket.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

int main()
{
	spdlog::set_level(spdlog::level::trace);
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		libgs::ssl::context ssl(libgs::ssl::context::sslv23_client);
#if 0
		libgs::io::ssl_stream<libgs::io::tcp_socket> stream(ssl);
		// std::error_code error;
		// co_await stream.next_layer().co_connect({"127.0.0.1", 6688}, {10s, error});
		try {
			co_await stream.next_layer().connect({"127.0.0.1", 6688}, 10s);
			co_await stream.handshake(libgs::ssl::stream_base::client, 10s);
			co_await stream.write("hello world");

			char buf[128] = "";
			auto size = co_await stream.read_some({buf,128});

			spdlog::debug("ssl_stream<tcp_socket> read: {}.", std::string_view(buf,size));
			libgs::execution::exit();
		}
		catch(std::exception &ex)
		{
			spdlog::error("tcp_socket<tcp_socket> error: {}.", ex);
			libgs::execution::exit(-1);
		}
#else
		libgs::io::ssl_tcp_socket socket(ssl);
		// std::error_code error;
		// co_await stream.next_layer().co_connect({"127.0.0.1", 6688}, {10s, error});
		try {
			co_await socket.connect({"127.0.0.1", 6688}, 10s);
		    co_await socket.write("hello world");

		    char buf[128] = "";
		    auto size = co_await socket.read_some({buf,128});

		    spdlog::debug("ssl_stream<tcp_socket> read: {}.", std::string_view(buf,size));
		    libgs::execution::exit();
		}
		catch(std::exception &ex)
		{
		    spdlog::error("tcp_socket<tcp_socket> error: {}.", ex);
		    libgs::execution::exit(-1);
		}
#endif
		co_return ;
	});
	return libgs::execution::exec();
}