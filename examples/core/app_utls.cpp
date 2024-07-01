#include <libgs/core/app_utls.h>
#include <spdlog/spdlog.h>

int main()
{
	spdlog::set_level(spdlog::level::trace);
	spdlog::debug("file path: {}", libgs::app::file_path());
	spdlog::debug("directory path: {}", libgs::app::dir_path());
	spdlog::debug("working path: {}", libgs::app::current_directory());
	spdlog::debug("absolute path: {}", libgs::app::absolute_path("././"));
	spdlog::debug("PATH: '{}'", libgs::app::getenv("PATH"));
	return 0;
}
