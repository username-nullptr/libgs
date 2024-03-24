#include <libgs/core/app_utls.h>
#include <libgs/core/log.h>

int main()
{
	libgs_log_debug("file path: {}", libgs::app::file_path());
	libgs_log_debug("directory path: {}", libgs::app::dir_path());
	libgs_log_debug("working path: {}", libgs::app::current_directory());
	libgs_log_debug("absolute path: {}", libgs::app::absolute_path("././"));
	libgs_log_debug("PATH: '{}'", libgs::app::getenv("PATH"));
	return 0;
}
