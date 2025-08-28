#include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/app_utls.h>
#include <iostream>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::sys_expected<> sed;
	sed = sed
	.transform([]{

	})
	.and_then([]{
		return libgs::sys_expected();
	})
	.or_else([]{
	})
	.or_else();

	auto aaa = libgs::strtls::to_int32("555")
		.transform([](int32_t iii) {
			std::cout << iii << std::endl;
			return std::string("hello");
		})
		.and_then([](std::string_view iii) {
			std::cout << iii << std::endl;
			return libgs::optional<int32_t>(234);
		})
		.or_else(123);

	std::cout << *aaa << std::endl;

	return 0;
}
