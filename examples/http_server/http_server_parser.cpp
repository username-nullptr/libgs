#include <libgs/http/server/request_parser.h>
#include <libgs/core/coro.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

asio::awaitable<void> service(asio::ip::tcp::socket socket, asio::ip::tcp::socket::endpoint_type ep)
{
	libgs::http::request_parser parser;
	try {
		char rbuf[4096] = "";
		for(;;)
		{
			auto var = co_await (socket.async_read_some(asio::buffer(rbuf,4096), asio::use_awaitable) or
								 libgs::co_sleep_for(5s));
			if( var.index() == 1 )
			{
				spdlog::error("socket error: read timeout.");
				break;
			}
			auto res = std::get<0>(var);
			if( res == 0 )
				break;

			if( not parser.append({rbuf,res}) or parser.can_read_from_device() )
				continue;

			spdlog::debug("Version:{} - Method:{} - Path:{}",
						  parser.version(),
						  method_string(parser.method()),
						  parser.path());

			for(auto &[key,value] : parser.parameters())
				spdlog::debug("Parameter: {}: {}", key, value);

			for(auto &[key,value] : parser.headers())
				spdlog::debug("Header: {}: {}", key, value);

			for(auto &[key,value] : parser.cookies())
				spdlog::debug("Cookie: {}: {}", key, value);

			spdlog::debug("partial_body: {}\n", parser.take_body());

			auto wbuf = std::format("HTTP/1.1 200 OK\r\n"
									"{}:close\r\n"
									"{}:11\r\n"
									"\r\n"
									"Hello world",
									libgs::http::header::connection,
									libgs::http::header::content_length);
			co_await asio::async_write(socket, asio::buffer(wbuf, wbuf.size()), asio::use_awaitable);
			socket.close();
			break;
		}
	}
	catch(const std::exception &ex) {
		spdlog::error("socket error: {}", ex);
	}
	spdlog::error("disconnected: {}", ep);
	co_return ;
}

int main()
{
	spdlog::set_level(spdlog::level::trace);
	constexpr unsigned short port = 12345;

	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		asio::ip::tcp::acceptor server(libgs::io_context());
		try {
			server.bind({asio::ip::tcp::v4(), port});
			for(;;)
			{
				auto socket = co_await server.async_accept(asio::use_awaitable);
				auto ep = socket.remote_endpoint();

				spdlog::debug("new connction: {}", ep);
				libgs::dispatch(service(std::move(socket), std::move(ep)));
			}
		}
		catch(const std::exception &ex)
		{
			spdlog::error("server error: {}", ex);
			libgs::exit(-1);
		}
	});
	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::exec();
}