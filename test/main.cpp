// #include <libgs/http/server.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/execution.h>
#include <libgs/core/app_utls.h>

#include <libgs/coro/utils.h>
#include <libgs/utils/modules.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/logger.h>
#include <iostream>
#include <memory>

using namespace std::chrono_literals;
using namespace libgs::operators;

int main()
{
	libgs::utils::modules::reg_init("a0", {.parents = {"e0"}}, []
	{
		spdlog::info("a0 init");
	});

	libgs::utils::modules::reg_init("b0", {.parents = {"c0"}}, []
	{
		spdlog::info("b0 init");
	});

	libgs::utils::modules::reg_init("c0", []
	{
		spdlog::info("c0 init");
	});

	libgs::utils::modules::reg_init("d0", {.children = {"a0"}, .parents = {"c0"}}, []
	{
		spdlog::info("d0 init");
	});

	libgs::utils::modules::reg_init("e0", []
	{
		spdlog::info("e0 init");
	});

	std::cout << libgs::utils::modules::sprint() << std::endl;

	libgs::utils::modules::do_init([]
	{
		libgs::exit(123);
	});
	return libgs::exec();
}