// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/utils/logger.h>
#include <filesystem>
#include <iostream>

int main(int argc, const char *argv[])
{
	const std::filesystem::path log_directory = argc > 1 ?
		argv[1] : "./logs";

	libgs::utils::logger::config_t config {
		.path = log_directory
	};
	libgs::utils::logger::instance().set_config(config);
	libgs::utils::logger::instance("network").set_config(config);

	libgs_utils_log_info("Application logger: {}", "ready");
	libgs_utils_clog_info("network", "Named logger: request {}", 42);

	std::cout << "Logs are written below " << log_directory << '\n';
	return 0;
}
