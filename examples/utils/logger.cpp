#include <libgs/utils/logger.h>

int main()
{
	libgs::utils::logger::config_t config {
		.path = "./logs/"
	};
	libgs::utils::logger::instance().set_config(config);
	libgs::utils::logger::instance("name").set_config(config);

	libgs_utils_log_info("hello world");
	libgs_utils_log_info("hello world {}", 123);

	std::string str = "string";
	int num = 1;
	libgs_utils_log_info("hello world {}, {}", str, num);

	libgs_utils_clog_info("name", "hello world");
	libgs_utils_clog_info("name", "hello world {}", 123);
	libgs_utils_clog_info("name", "hello world {}, {}", str, num);
	return 0;
}