#include <libgs/core/log.h>

using namespace std::chrono_literals;

void func_0()
{
	libgs::sleep_for(1ms);
	for(int i=0; i<200; i++)
		libgs_log_debug("==== {}", i);
}

void func_1()
{
	libgs::sleep_for(1ms);
	for(int i=200; i<400; i++)
		libgs_log_debug("==== {}", i);
}

void func_2()
{
	libgs::sleep_for(1ms);
	for(int i=400; i<600; i++)
		libgs_log_debug("==== {}", i);
}

int main()
{
#if 0
	auto context = libgs::log::logger::get_context();
	context.async = true;
	context.source_visible = true;
	context.header_breaks_aline = true;
	libgs::log::logger::set_context(context);
#endif

	std::thread t0(func_0);
	std::thread t1(func_1);
	std::thread t2(func_2);
	t0.join();
	t1.join();
	t2.join();
	return 0;
}