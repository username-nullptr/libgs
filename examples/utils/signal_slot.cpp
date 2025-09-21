#include <libgs/utils/signal_slot.h>
#include <libgs/utils/logger.h>

void fff(int i)
{
	libgs_utils_log_info("2222: {}", i);
}

int main()
{
	libgs::utils::signal<void(int,const char*)> sig;
	asio::io_context ioc;

	sig
	.connect (
		[](bool i, std::string_view d)
		{
			libgs_utils_log_info("0000: {} {}", i, d);
		},
		[](bool i, bool d)
		{
			libgs_utils_log_info("1111: {} {}", i, d);
		}
	)
	.connect<libgs::utils::slot_mode::async>([](bool i, bool d) -> libgs::awaitable<void>
	{
		libgs_utils_log_info("1111: {} {}", i, d);
		co_return ;
	})
	.connect(fff)
	.connect(ioc, [](float i)
	{
		libgs_utils_log_info("3333: {}", i);
	});

	sig(11, "hello");

	sig.disconnect(fff);

	sig(22, "world");

	std::thread([&] {
		ioc.run();
	}).detach();

	using namespace std::chrono_literals;
	libgs::post(2s, []{
		libgs::exit();
	});
	return libgs::exec();
}