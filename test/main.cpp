// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>
#include <libgs/core/modules.h>

#include <spdlog/spdlog.h>
#include <iostream>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::modules::reg_init("a0", []{
		spdlog::info("a0 init: {}", std::this_thread::get_id());
	});
	libgs::modules::reg_init("b0", []{
		spdlog::info("b0 init: {}", std::this_thread::get_id());
	});
	libgs::modules::reg_init("a1", {.after = { "a0", "b0" }}, []{
		spdlog::info("a1 init: {}", std::this_thread::get_id());
	});
	libgs::modules::reg_init("a2", {.after = { "a1" }}, []{
		spdlog::info("a2 init: {}", std::this_thread::get_id());
	});
	libgs::modules::reg_init("b2", {.after = { "a1" }}, []{
		spdlog::info("b2 init: {}", std::this_thread::get_id());
	});
	libgs::modules::reg_init("a3", {.after = { "b2" }}, []{
		spdlog::info("a3 init: {}", std::this_thread::get_id());
	});

	std::cout << libgs::modules::sprint() << std::endl;

	libgs::modules::do_init(libgs::io_context(), []{
		libgs::exit(0);
	});
	return libgs::exec();
}