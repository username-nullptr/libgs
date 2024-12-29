#include <libgs/core/modules.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
#if 1
	libgs::modules::run_init();
	spdlog::info("modules initialized.");
	return 0;
#else
	libgs::co_spawn_detached([]() -> libgs::awaitable<void>
	{
		co_await libgs::modules::co_run_init();
		spdlog::info("modules initialized.");
		co_return libgs::exit(0);
	});
	return libgs::exec();
#endif
}