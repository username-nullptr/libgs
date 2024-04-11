#include <libgs/http/server/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

int main()
{
	libgs::http::server server;
	server
	.on_request([](libgs::http::server::request_ptr request) -> libgs::awaitable<void>
	{
		libgs_log_debug("Version:{} - Method:{} - Path:{}",
						request->version(),
						libgs::http::to_method_string(request->method()),
						request->path());

		for(auto &[key,value] : request->parameters())
			libgs_log_debug("Parameter: {}: {}", key, value);

		for(auto &[key,value] : request->headers())
			libgs_log_debug("Header: {}: {}", key, value);

		for(auto &[key,value] : request->cookies())
			libgs_log_debug("Cookie: {}: {}", key, value);


		libgs_log_debug("partial_body: {}\n", co_await request->read_all());
		co_return ;
	})
	.on_error([](const std::error_code &error)
	{

	});
	return libgs::execution::exec();
}