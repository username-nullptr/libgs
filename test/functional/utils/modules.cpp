// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/modules.h>

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
	libgs::utils::modules::reg_init("failing", [] { return false; });
	libgs::utils::modules::reg_init("blocked", {
		.parents = {"failing"}
	}, [] {});
	libgs::utils::modules::reg_init("orphan", {
		.parents = {"missing-parent"}
	}, [] {});
	LIBGS_TEST_CHECK_THROWS(
		libgs::utils::modules::reg_init("database", [] {}),
		libgs::runtime_error
	);
	LIBGS_TEST_CHECK_THROWS(
		libgs::utils::modules::reg_init("", [] {}),
		libgs::runtime_error
	);

	const auto graph = libgs::utils::modules::sprint();
	LIBGS_TEST_CHECK(graph.find("database") != std::string::npos);
	LIBGS_TEST_CHECK(graph.find("service") != std::string::npos);
	auto unexpected = libgs::utils::modules::do_init(
		libgs::string_vector {"--test", "value"}
	);
	LIBGS_TEST_CHECK_EQ(unexpected.failures, std::vector<std::string> {"failing"});
	LIBGS_TEST_CHECK_EQ(unexpected.unregistered,
		std::vector<std::string> {"missing-parent"});
	std::ranges::sort(unexpected.children);
	LIBGS_TEST_CHECK_EQ(unexpected.children,
		(std::vector<std::string> {"blocked", "orphan"}));
	LIBGS_TEST_CHECK_EQ(order.size(), 2U);
	LIBGS_TEST_CHECK_EQ(order[0], "database");
	LIBGS_TEST_CHECK_EQ(order[1], "service");
	return 0;
}
