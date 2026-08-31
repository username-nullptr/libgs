#include <libgs/core/execution.h>
#include <iostream>
#include <chrono>

int main()
{
	using namespace std::chrono_literals;

	libgs::dispatch([] {
		std::cout << "dispatch runs immediately on a compatible context\n";
	});
	libgs::post([] {
		std::cout << "post runs from the event queue\n";
	});
	libgs::post(30ms, [] {
		std::cout << "delayed post fired\n";
	});

	auto timer = libgs::start_timer(20ms, [] {
		std::cout << "periodic timer tick\n";
	});
	libgs::post(75ms, [&timer] {
		timer();
		libgs::exit();
	});

	return libgs::exec();
}
