// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/modules.h>

#include <mutex>
#include <string>
#include <vector>

int main()
{
	std::mutex mutex;
	std::vector<std::string> order;
	libgs::utils::modules::reg_init("database", [&]
	{
		std::scoped_lock lock(mutex);
		order.emplace_back("database");
		return true;
	});
	libgs::utils::modules::reg_init("service", {
		.parents = {"database"}
	}, [&](const libgs::string_vector &arguments)
	{
		LIBGS_TEST_CHECK_EQ(arguments.size(), 2U);
		std::scoped_lock lock(mutex);
		order.emplace_back("service");
	});

	const auto graph = libgs::utils::modules::sprint();
	LIBGS_TEST_CHECK(graph.find("database") != std::string::npos);
	LIBGS_TEST_CHECK(graph.find("service") != std::string::npos);
	const auto unexpected = libgs::utils::modules::do_init(
		libgs::string_vector {"--test", "value"}
	);
	LIBGS_TEST_CHECK(unexpected.failures.empty());
	LIBGS_TEST_CHECK(unexpected.unregistered.empty());
	LIBGS_TEST_CHECK(unexpected.children.empty());
	LIBGS_TEST_CHECK_EQ(order.size(), 2U);
	LIBGS_TEST_CHECK_EQ(order[0], "database");
	LIBGS_TEST_CHECK_EQ(order[1], "service");
	return 0;
}
