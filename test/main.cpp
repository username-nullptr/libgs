#include <libgs/core/library.h>
#include <iostream>

using namespace std::chrono_literals;
// using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	auto aaa = libgs::strtls::to_int32("555")
	.and_then([](int32_t iii) {
		std::cout << iii << std::endl;
		return libgs::optional<int32_t>(234);
	})
	.or_else(123);

	std::cout << *aaa << std::endl;

	return 0;
}
