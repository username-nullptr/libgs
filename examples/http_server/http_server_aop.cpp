#include <libgs/http/server.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

class aop : public libgs::http::server::aop_t
{
public:
	[[nodiscard]] libgs::awaitable<bool> before(context_t &context) override
	{
		spdlog::info("request before log: '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	[[nodiscard]] libgs::awaitable<bool> after(context_t &context) override
	{
		spdlog::info("request after log: '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	[[nodiscard]] bool exception(context_t &context, const std::exception &ex) override
	{
		spdlog::error("request throw [{}] log: '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
};

class controller : public libgs::http::server::ctrlr_aop_t
{
public:
	[[nodiscard]] libgs::awaitable<bool> before(context_t &context) override
	{
		spdlog::info("request before log (controller): '{}'.", context.request().path());
		// Returning false will call the next 'before', then the service,
		// and finally 'after', and if returning true will terminate the chain of calls.
		co_return false;
	}
	[[nodiscard]] libgs::awaitable<bool> after(context_t &context) override
	{
		spdlog::info("request after log (controller): '{}'.", context.request().path());
		// If returning false will continue to call the next 'after',
		// otherwise terminates the call chain.
		co_return false;
	}
	[[nodiscard]] bool exception(context_t &context, const std::exception &ex) override
	{
		spdlog::error("request throw [{}] log (controller): '{}'.", ex, context.request().path());
		// Returning true terminates the chain of calls,
		// otherwise the next 'exception' is called until 'on_exception' of the server,
		// causing abort if 'on_exception' of the server also returns false.
		return true;
	}
	[[nodiscard]] libgs::awaitable<void> service(context_t &context) override
	{
		co_await context.response().write("hello libgs (controller)", asio::use_awaitable);
		co_return ;
	}
};

int main()
{
	spdlog::set_level(spdlog::level::trace);
	asio::ip::tcp::acceptor acceptor(libgs::get_executor());
	constexpr unsigned short port = 12345;

	libgs::http::server server(std::move(acceptor));
	server.bind({libgs::ip_type::v4, port})

	.on_request<libgs::http::method::GET>("/*",
	[](libgs::http::server::context_t &context) -> libgs::awaitable<void>
	{
		co_await context.response().write("hello libgs", asio::use_awaitable);
		co_return ;
	},
	new aop(), new aop())

	.on_request<libgs::http::method::GET>("/ctrlr*", new controller())

	.on_server_error([](std::error_code error)
	{
		spdlog::error(error);
		libgs::exit(-1);
		return true;
	})
	.start();

	spdlog::info("HTTP Server started ({}) ...", port);
	return libgs::exec();
}
