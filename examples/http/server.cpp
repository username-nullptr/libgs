#include <libgs/http/server.hpp>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
#if 0
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request([](libgs::http::server::request &request, libgs::http::server::response &response) -> libgs::awaitable<void>
	{
		try {
			libgs_log_debug("Version:{} - Method:{} - Path:{}",
							request.version(),
							libgs::http::to_method_string(request.method()),
							request.path());

			for(auto &[key,value] : request.parameters())
				libgs_log_debug("Parameter: {}: {}", key, value);

			for(auto &[key,value] : request.headers())
				libgs_log_debug("Header: {}: {}", key, value);

			for(auto &[key,value] : request.cookies())
				libgs_log_debug("Cookie: {}: {}", key, value);

			if( request.can_read_body() )
				libgs_log_debug("partial_body: {}\n", co_await request.read_all());

			// If you don't write anything, the server will write the default body for you
			// co_await response.write("hello world");
		}
		catch(std::exception &ex) {
			libgs_log_error() << ex;
		}
		co_return ;
	})
	.on_error([](std::error_code error) -> libgs::awaitable<void>
	{
		libgs_log_error() << error;
		libgs::execution::exit(-1);
		co_return ;
	})
	.start();
#else
	libgs::http::wserver server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request([](libgs::http::wserver::request &request, libgs::http::wserver::response &response) -> libgs::awaitable<void>
	{
		try {
			libgs_log_wdebug(L"Version:{} - Method:{} - Path:{}",
							request.version(),
							libgs::http::to_method_string<wchar_t>(request.method()),
							request.path());

			for(auto &[key,value] : request.parameters())
				libgs_log_wdebug(L"Parameter: {}: {}", key, value);

			for(auto &[key,value] : request.headers())
				libgs_log_wdebug(L"Header: {}: {}", key, value);

			for(auto &[key,value] : request.cookies())
				libgs_log_wdebug(L"Cookie: {}: {}", key, value);

			if( request.can_read_body() )
				libgs_log_debug("partial_body: {}\n", co_await request.read_all());

			// If you don't write anything, the server will write the default body for you
			// co_await response.write("hello world");
		}
		catch(std::exception &ex) {
			libgs_log_error() << ex;
		}
		co_return ;
	})
	.on_error([](std::error_code error) -> libgs::awaitable<void>
	{
		  libgs_log_error() << error;
		  libgs::execution::exit(-1);
		  co_return ;
	})
	.start();
#endif
	return libgs::execution::exec();
}