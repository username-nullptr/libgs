#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

class aop : public libgs::http::server::aop
{
public:
	libgs::awaitable<bool> before(context_t &context) override
	{
		spdlog::info("request before log: '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	libgs::awaitable<bool> after(context_t &context) override
	{
		spdlog::info("request after log: '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	bool exception(context_t &context, std::exception &ex) override
	{
		spdlog::error("request throw [{}] log: '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
};

class controller : public libgs::http::server::ctrlr_aop
{
public:
	libgs::awaitable<bool> before(context_t &context) override
	{
		spdlog::info("request before log (controller): '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	libgs::awaitable<bool> after(context_t &context) override
	{
		spdlog::info("request after log (controller): '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	bool exception(context_t &context, std::exception &ex) override
	{
		spdlog::error("request throw [{}] log (controller): '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
	libgs::awaitable<void> service(context_t &context) override
	{
		co_await context.response().co_write(asio::buffer("hello libgs (controller)"));
		co_return ;
	}
};

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::execution::get_executor());
	constexpr unsigned short port = 12345;

	libgs::http::server server(std::move(acceptor));
	server.bind({asio::ip::address_v4(), port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context &context) -> libgs::awaitable<void>
	{
		co_await context.response().co_write(asio::buffer("hello libgs"));
		co_return ;
	},
	new aop(), new aop())

	.on_request<libgs::http::method::GET>("/ctrlr*", new controller())

	.on_system_error([](std::error_code error)
	{
		spdlog::error(error);
		libgs::execution::exit(-1);
		return true;
	})
	.start();

	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::execution::exec();
}
