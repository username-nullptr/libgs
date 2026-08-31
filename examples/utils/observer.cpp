#include <libgs/utils/observer.h>
#include <iostream>

int main()
{
	using observer_t = libgs::utils::observer<void(int)>;
	constexpr std::uint64_t observer_id = 1001;

	auto receiver = observer_t::make(observer_id);
	receiver->on_triggered<0>([](int value)
	{
		std::cout << "Observer received " << value << '\n';
		libgs::exit();
	});

	libgs::post([] {
		observer_t::trigger<0>(observer_id, 42);
	});
	return libgs::exec();
}
