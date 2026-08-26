#include <libgs/http/client.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);

	libgs::http::client client;
	libgs::http::request_arg arg;

#if 1
	auto context = client.request_get({"http://www.baidu.com", arg});
	auto status = context->wait_reply();
	// auto body = context->reply()->read();
	auto sum = context->reply()->save_file("./baidu.html");

	return 0;
#else
	libgs::dispatch([&]() mutable -> libgs::awaitable<void>
	{
		auto context = co_await client.request_get (
			{"http://www.baidu.com", arg}, libgs::use_awaitable
		);
		auto status = co_await context->wait_reply(libgs::use_awaitable);
		// auto body = co_await context->reply()->read(libgs::use_awaitable);
		auto sum = co_await context->reply()->save_file("./baidu.html", libgs::use_awaitable);

		libgs::exit();
		co_return ;
	});
	return libgs::exec();
#endif
}