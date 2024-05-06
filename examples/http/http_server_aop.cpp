#include <libgs/http/server.h>
#include <libgs/core/log.h>

using namespace std::chrono_literals;

class aop : public libgs::http::aop
{
public:
	libgs::awaitable<bool> before(libgs::http::service_context &context) override
	{
		libgs_log_info("request before log: '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	libgs::awaitable<bool> after(libgs::http::service_context &context) override
	{
		libgs_log_info("request after log: '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	bool exception(context_type &context, std::exception &ex) override
	{
		libgs_log_error("request throw <{}> log: '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
};

class controller : public libgs::http::ctrlr_aop
{
public:
	libgs::awaitable<bool> before(libgs::http::service_context &context) override
	{
		libgs_log_info("request before log (controller): '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	libgs::awaitable<bool> after(libgs::http::service_context &context) override
	{
		libgs_log_info("request after log (controller): '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	bool exception(context_type &context, std::exception &ex) override
	{
		libgs_log_error("request throw <{}> log (controller): '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
	libgs::awaitable<void> service(libgs::http::service_context &context) override
	{
		co_await context.response().write("hello libgs (controller)");
		co_return ;
	}
};

int main()
{
	libgs::http::server server;
	server.bind({libgs::io::address_v4(),12345})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::service_context &context) -> libgs::awaitable<void>
	{
		co_await context.response().write("hello libgs");
		co_return ;
	},
	new aop(), new aop())

	.on_request<libgs::http::method::GET>("/ctrlr*", new controller())

	.on_system_error([](std::error_code error)
	{
		libgs_log_error() << error;
		libgs::execution::exit(-1);
		return true;
	})
	.start();
	return libgs::execution::exec();
}
