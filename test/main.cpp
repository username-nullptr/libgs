// #include <libgs.h>
// #include <libgs/core.h>
// #include <spdlog/spdlog.h>

// #include <libgs/http/client.h>
// #include <libgs/http/client/request.h>

#include <list>
#include <iostream>
#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/parser.h>
#include <libgs/http/server.h>

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/execution.h>
#include <libgs/core/observer.h>
#include <libgs/coro/utils.h>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::optional<int> opt0 = 123;

	auto fff = [](int ii)
	{
		return libgs::make_optional(234);
	};

	constexpr bool bbbbb = libgs::optional<int>::and_then_v<decltype(fff)>;
	static_assert(bbbbb, "2222222222222");

	auto opt1 = opt0
	.and_then([](int ii)
	{
		return libgs::make_optional(456);
	})
	.or_else([]
	{
		return libgs::make_optional(789);
	});

	std::cout << *opt1 << std::endl;

	libgs::optional<std::string> opt2;

	auto opt3 = opt2
	.and_then([](std::string_view ii)
	{
		return libgs::make_optional(3.14);
	})
	.or_else([]
	{
		return libgs::make_optional(1.414);
	});

	std::cout << *opt3 << std::endl;

	return 0;
}
