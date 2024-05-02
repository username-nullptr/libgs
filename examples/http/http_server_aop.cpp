#include <libgs/http/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

class aop : public libgs::http::aop
{
public:
	libgs::awaitable<bool> before(request_type &request, response_type &response) override
	{
		libgs_log_info("request before log: '{}'.", request.path());
		co_return false;
	}
	libgs::awaitable<bool> after(request_type &request, response_type &response) override
	{
		libgs_log_info("request after log: '{}'.", request.path());
		co_return false;
	}
};

class controller : public libgs::http::ctrlr_aop
{
public:
	libgs::awaitable<bool> before(request_type &request, response_type &response) override
	{
		libgs_log_info("request before log (controller): '{}'.", request.path());
		co_return false;
	}
	libgs::awaitable<bool> after(request_type &request, response_type &response) override
	{
		libgs_log_info("request after log (controller): '{}'.", request.path());
		co_return false;
	}
	libgs::awaitable<void> service(request_type &request, response_type &response) override
	{
		co_await response.write("hello libgs (controller)");
		co_return ;
	}
};

int main()
{
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*", {
	[](libgs::http::server::request &request, libgs::http::server::response &response) -> libgs::awaitable<void>
	{
		co_await response.write("hello libgs");
		co_return ;
	},
	new aop(), new aop()})

	.on_request<libgs::http::method::GET>("/ctrlr*", new controller())

	.on_error([](std::error_code error) -> libgs::awaitable<void>
	{
		libgs_log_error() << error;
		libgs::execution::exit(-1);
		co_return ;
	})
	.start();
	return libgs::execution::exec();
}
