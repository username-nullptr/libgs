#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/parser.h>
#include <libgs/core/execution.h>
#include <spdlog/spdlog.h>

using tcp_t = asio::ip::tcp;
using udp_t = asio::ip::udp;
using namespace libgs::operators;

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		std::error_code error;
		udp_t::resolver resolver(libgs::io_context());

		auto results = co_await resolver.async_resolve (
			"baidu.com", "", asio::use_awaitable | error
		);
		if( error )
		{
	        spdlog::info("Failed to resolve domain: {}", error);
			co_return ;
		}
		tcp_t::endpoint ep (
			results.begin()->endpoint().address(), 443
		);
		tcp_t::socket socket(libgs::io_context());
		co_await socket.async_connect(ep, asio::use_awaitable | error);
		if( error )
		{
			spdlog::info("Failed to connect to server: {}", error);
			co_return ;
		}
		libgs::http::protocol::url url("http://www.baidu.com");
		libgs::http::protocol::request_arg arg(url);

		arg.set_header("Connection", "keep-alive")
		   .set_header("Host", "www.baidu.com");

		libgs::http::protocol::client_generator generator(arg);
		auto text = generator.header_data<libgs::http::protocol::method::get>();

		auto sum = co_await async_write(socket, asio::buffer(text), asio::use_awaitable | error);
		if( error )
		{
			spdlog::info("Failed to write to server: {}", error);
			co_return ;
		}
		libgs::http::protocol::client_parser parser;
		char buffer[0xFFFF];
		for(;;)
		{
			sum = co_await socket.async_read_some(asio::buffer(buffer, 0xFFFF), asio::use_awaitable | error);
			if( error )
			{
				spdlog::info("Failed to read from server: {}", error);
				co_return ;
			}
			bool res = parser.append({buffer, sum}, error);
			if( error )
			{
				spdlog::info("Failed to parse reply: {}", error);
				co_return ;
			}
			if( res )
				break;
		}
		text = parser.take_body();
		while( parser.can_read_from_device() )
		{
			sum = co_await socket.async_read_some(asio::buffer(buffer, 0xFFFF), asio::use_awaitable | error);
			if( error )
			{
				spdlog::info("Failed to read from server: {}", error);
				co_return ;
			}
			bool res = parser.append({buffer, sum}, error);
			if( error )
			{
				spdlog::info("Failed to parse reply-body: {}", error);
				co_return ;
			}
			if( res )
				text += parser.take_body();
		}

		// Do something ...
		co_return ;
	});
	return libgs::exec();
}