// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client.h>
// #include <libgs/http/client/request.h>

#include <list>
#include <iostream>
#include <libgs/http/server.h>

#include <libgs/core/observer.h>
#include <libgs/core/execution.h>
#include <libgs/coro/utils.h>

using namespace std::chrono_literals;
// using namespace libgs::operators;

class aaa : public libgs::observer<aaa,int>
{
public:
	using observer::observer;
	ptr_t on_aaa(callback_t callback) {
		return set_callback(std::move(callback));
	}
};

class base {};

class derived : public base {};

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	auto a = aaa::make();
	a->on_aaa([](int i)
	{
		std::cout << "aaa: " << i << std::endl;
	});
	libgs::dispatch([]() -> libgs::awaitable<void>
	{
		for(int i=0; i<3; i++)
		{
			aaa::trigger(i);
			co_await libgs::coro::sleep_for(1s);
		}
		co_return libgs::exit();
	});
	return libgs::exec();


	// return 0;
}
