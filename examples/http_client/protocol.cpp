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
		libgs::http::url url("http://www.baidu.com");
		libgs::http::request_arg arg;

		arg.set_header("Connection", "keep-alive")
		   .set_header("Host", "www.baidu.com");

		libgs::http::client_generator generator(url, arg);
		auto text = generator.header_data<libgs::http::method::get>();

		auto sum = co_await async_write(socket, asio::buffer(text), asio::use_awaitable | error);
		if( error )
		{
			spdlog::info("Failed to write to server: {}", error);
			co_return ;
		}
		libgs::http::client_parser parser;
		char buffer[0xFFFF];
		for(;;)
		{
			sum = co_await socket.async_read_some(asio::buffer(buffer, 0xFFFF), asio::use_awaitable | error);
			if( error )
			{
				spdlog::info("Failed to read from server: {}", error);
				co_return ;
			}
			auto expected = parser.append({buffer, sum});
			if( not expected )
			{
				spdlog::info("Failed to parse reply: {}", expected.error());
				co_return ;
			}
			if( *expected )
				break;
		}
		text = parser.take_body();
		while( parser.stage() == libgs::http::stage::body )
		{
			sum = co_await socket.async_read_some (
				asio::buffer(buffer, 0xFFFF), asio::use_awaitable | error
			);
			if( error )
			{
				spdlog::info("Failed to read from server: {}", error);
				co_return ;
			}
			auto expected = parser.append({buffer, sum});
			if( not expected )
			{
				spdlog::info("Failed to parse reply-body: {}", expected.error());
				co_return ;
			}
			if( *expected )
				text += parser.take_body();
		}

		// Do something ...
		co_return ;
	});
	return libgs::exec();
}