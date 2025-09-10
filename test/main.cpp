// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>
#include <libgs/utils/modules.h>

#include <spdlog/spdlog.h>
#include <iostream>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	// spdlog::set_level(spdlog::level::trace);

	libgs::utils::modules::reg_init("a0", []{
		spdlog::info("a0 init: {}", std::this_thread::get_id());
	});
	libgs::utils::modules::reg_init("b0", []{
		spdlog::info("b0 init: {}", std::this_thread::get_id());
		return false;
	});
	libgs::utils::modules::reg_init("a1", {.after = { "a0", "b0" }}, []{
		spdlog::info("a1 init: {}", std::this_thread::get_id());
	});
	libgs::utils::modules::reg_init("a2", {.after = { "a1" }}, []{
		spdlog::info("a2 init: {}", std::this_thread::get_id());
	});
	libgs::utils::modules::reg_init("b2", {.after = { "a1" }}, []{
		spdlog::info("b2 init: {}", std::this_thread::get_id());
	});
	libgs::utils::modules::reg_init("a3", {.after = { "b2" }}, []{
		spdlog::info("a3 init: {}", std::this_thread::get_id());
	});

	std::cout << libgs::utils::modules::sprint() << std::endl;

	libgs::utils::modules::do_init(libgs::io_context(),
	[](const libgs::utils::modules::unexpected &unexpected)
	{
		for(auto &name : unexpected.failures)
			spdlog::error("failed init: {}", name);
		for(auto &name : unexpected.unregistered)
			spdlog::error("unregistered: {}", name);
		for(auto &name : unexpected.children)
			spdlog::error("not init children: {}", name);
		libgs::exit(0);
	});
	return libgs::exec();
}