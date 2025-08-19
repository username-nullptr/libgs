// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client.h>
// #include <libgs/http/client/request.h>

#include <list>
#include <iostream>
#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/parser.h>
#include <libgs/http/server.h>

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/execution.h>
#include <libgs/core/observer.h>
#include <libgs/coro/utils.h>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);
	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		using tcp_t = asio::ip::tcp;
		using udp_t = asio::ip::udp;

		udp_t::resolver resolver(libgs::io_context());
		auto results = co_await resolver.async_resolve (
			"baidu.com", "", asio::use_awaitable
		);
		if( results.empty() )
		{
			std::cerr << "Failed to resolve api domain" << std::endl;
			co_return ;
		}

		tcp_t::endpoint ep (
			results.begin()->endpoint().address(), 443
		);
		tcp_t::socket socket(libgs::io_context());
		co_await socket.async_connect(ep, asio::use_awaitable);

		libgs::http::protocol::url url("http://www.baidu.com");
		libgs::http::protocol::request_arg arg(url);

		arg.set_header("Connection", "keep-alive")
		   .set_header("Host", "www.baidu.com");

		libgs::http::protocol::client_generator generator(arg);
		auto text = generator.header_data<libgs::http::protocol::method::get>();
		auto sum = co_await async_write(socket, asio::buffer(text), asio::use_awaitable);

		libgs::http::protocol::client_parser parser;
		char buffer[0xFFFF];
		for(;;)
		{
			sum = co_await socket.async_read_some(asio::buffer(buffer, 0xFFFF), asio::use_awaitable);

			std::error_code error;
			bool res = parser.append({buffer, sum}, error);

			if( error )
			{
				std::cerr << "Error: " << error << std::endl;
				co_return ;
			}
			if( res )
				break;
		}
		text = parser.take_body();
		while( parser.can_read_from_device() )
		{
			sum = co_await socket.async_read_some(asio::buffer(buffer, 0xFFFF), asio::use_awaitable);

			std::error_code error;
			bool res = parser.append({buffer, sum}, error);

			if( error )
			{
				std::cerr << "Error: " << error << std::endl;
				co_return ;
			}
			if( res )
				text += parser.take_body();
		}

		int i = 0;
		i = 11;

		co_return ;
	});
	return libgs::exec();
	// return 0;
}
