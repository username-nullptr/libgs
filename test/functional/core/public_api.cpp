// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/core/async_expected.h>
#include <libgs/core/algorithm/math.h>
#include <libgs/core/container.h>
#include <libgs/core/string_deque.h>
#include <libgs/core/string_list.h>
#include <libgs/core/string_set.h>
#include <libgs/core/system.h>
#include <libgs/core/utils/formatter.h>
#include <libgs/core/utils/utils.h>

namespace
{

enum class feature : uint8_t {
	none = 0, read = 1, write = 2, execute = 4
};
LIBGS_DECLARE_FLAGS(features, feature);
LIBGS_DECLARE_OPERATORS_FOR_FLAGS(features);

class parameter_owner final : public libgs::mutable_parameters<parameter_owner>
{
public:
	parameter_owner() : mutable_parameters(&storage) {}
	libgs::parameter_map storage;
};

void flags_and_parameters()
{
	features selected {feature::read, feature::write};
	LIBGS_TEST_CHECK(selected.test_flag(feature::read));
	LIBGS_TEST_CHECK_EQ(selected.value<uint8_t>(), uint8_t {3});
	selected.set_flag(feature::write, false).set_flag(feature::execute);
	LIBGS_TEST_CHECK(not selected.test_flag(feature::write));
	LIBGS_TEST_CHECK(selected.test_flag(feature::execute));
	LIBGS_TEST_CHECK_EQ(*(feature::read | feature::write), uint32_t {3});
	LIBGS_TEST_CHECK_EQ(*(feature::read | features {feature::write}), uint32_t {3});
	LIBGS_TEST_CHECK(!(selected & uint8_t {2}));

	parameter_owner owner;
	owner.set_parameter("first", 1).set_parameter(std::string("second"), "two");
	LIBGS_TEST_CHECK_EQ(owner.parameter(0)->to_int().value_or(0), 1);
	LIBGS_TEST_CHECK_EQ(owner.parameter("second")->to_string(), "two");
	LIBGS_TEST_CHECK(owner.contains_parameter("first", 1));
	LIBGS_TEST_CHECK(not owner.contains_parameter(2));
	LIBGS_TEST_CHECK_THROWS(owner.parameter(2), libgs::runtime_error);

	const auto &const_map = owner.storage;
	LIBGS_TEST_CHECK_EQ(const_map["first"].to_int().value_or(0), 1);
	LIBGS_TEST_CHECK_THROWS(const_map["missing"], libgs::out_of_range);
	owner.unset_parameter("first").unset_parameter("missing");
	LIBGS_TEST_CHECK(not owner.contains_parameter("first"));
}

void value_and_string_algorithms()
{
	libgs::value number("7f");
	LIBGS_TEST_CHECK_EQ(number.to_uint(16).value_or(0), 127U);
	LIBGS_TEST_CHECK_EQ(number.get<int>(16).value_or(0), 127);
	LIBGS_TEST_CHECK(number.is_alnum());
	LIBGS_TEST_CHECK(not number.is_digit());
	LIBGS_TEST_CHECK(libgs::value("-1.25").is_rlnum());
	LIBGS_TEST_CHECK(libgs::value("ASCII").is_ascii());
	LIBGS_TEST_CHECK(not libgs::strtls::is_rlnum("-"));
	LIBGS_TEST_CHECK(not libgs::strtls::to_int8("128"));
	LIBGS_TEST_CHECK(not libgs::strtls::to_arith<int8_t>("128", 10));
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_arith<int8_t>("127", 10).value_or(0), 127);
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_string(0), "0");
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_string(
		std::numeric_limits<int>::min()), std::to_string(std::numeric_limits<int>::min()));
	int unchanged = -42;
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_string(unchanged), "-42");
	LIBGS_TEST_CHECK_EQ(unchanged, -42);
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_string(255, 16, true), "FF");

	size_t replacements = 0;
	LIBGS_TEST_CHECK_EQ(
		libgs::strtls::replace(replacements, std::string("aaaa"), "aa", "b"),
		"bb"
	);
	LIBGS_TEST_CHECK_EQ(replacements, 2U);
	LIBGS_TEST_CHECK_EQ(libgs::strtls::remove(std::string("a-b-c"), '-'), "abc");

	const std::array projected {1, 2, 3};
	LIBGS_TEST_CHECK_EQ(libgs::mean(projected.begin(), projected.end(),
		[](const int &value) {
			return &value;
		}), 2);
	const std::list<int> iterator_projected {1, 2, 3};
	LIBGS_TEST_CHECK_EQ(libgs::mean(iterator_projected.begin(), iterator_projected.end(),
		[](std::list<int>::const_iterator it) {
			return &*it;
		}), 2);

	const libgs::string_deque deque {"a", "b", "c"};
	LIBGS_TEST_CHECK_EQ(deque.join(1, "::"), "b::c");
	const libgs::string_list list {"left", "right"};
	LIBGS_TEST_CHECK_EQ(list.join('/'), "left/right");
	const libgs::string_set set {"beta", "alpha"};
	LIBGS_TEST_CHECK_EQ(set.join(','), "alpha,beta");
}

void optional_expected_and_async_helpers()
{
	auto optional = libgs::make_optional(std::string("value"));
	LIBGS_TEST_CHECK_EQ(optional.and_then([](const std::string &value) {
		return libgs::optional<size_t>(value.size());
	}).value_or(0), 5U);
	LIBGS_TEST_CHECK_EQ(libgs::optional<int>().or_else(42).value_or(0), 42);

	libgs::expected<int,std::string> value(21);
	LIBGS_TEST_CHECK_EQ(value.transform([](int number) { return number * 2; }).value_or(), 42);
	value.despair("failed");
	LIBGS_TEST_CHECK_EQ(value.or_else(7).value_or(), 7);
	LIBGS_TEST_CHECK_EQ(value.or_else([](const std::string &error) {
		return libgs::expected<int,std::string>(static_cast<int>(error.size()));
	}).value_or(), 6);

	libgs::expected<void,std::string> no_value;
	LIBGS_TEST_CHECK(no_value);
	no_value.despair("error");
	LIBGS_TEST_CHECK(no_value.is_error());
	no_value.emplace();
	LIBGS_TEST_CHECK(no_value.has_value());

	libgs::error_code error;
	LIBGS_TEST_CHECK_EQ(libgs::expected_value_or_error(
		libgs::sys_expected<int>(9), error), 9);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(libgs::expected_value_or_error(
		libgs::sys_expected<int>(libgs::sys_unexpected(
			std::make_error_code(std::errc::invalid_argument))), error), 0);
	LIBGS_TEST_CHECK(error == std::errc::invalid_argument);
	LIBGS_TEST_CHECK_THROWS(libgs::expected_value_or_throw(
		libgs::sys_expected<int>(libgs::sys_unexpected(error))), std::system_error);

	std::string text = "borrowed";
	auto borrowed = libgs::capture_async_argument(std::ref(text));
	LIBGS_TEST_CHECK(&libgs::unwrap_async_argument(borrowed) == &text);
	auto owned = libgs::capture_async_argument(std::string("owned"));
	LIBGS_TEST_CHECK_EQ(libgs::unwrap_async_argument(owned), "owned");
}

void formatting_and_endpoints()
{
	const libgs::optional<int> empty;
	LIBGS_TEST_CHECK_EQ(std::format("{}", empty), "optional(null)");
	LIBGS_TEST_CHECK_EQ(std::format("{}", libgs::optional<int>(42)), "42");
	LIBGS_TEST_CHECK_EQ(std::format("{}", std::atomic_int(7)), "7");
	LIBGS_TEST_CHECK_EQ(std::format("{}", std::filesystem::path("a/b")), "a/b");
	LIBGS_TEST_CHECK_EQ(std::format("{}", std::vector<std::byte> {}),
		"empty[std::vector<std::byte>]");
	LIBGS_TEST_CHECK_EQ(std::format("{}", std::make_pair(1, 2)), "'1'-'2'");

	libgs::tcp_endpoint_wrapper loopback(libgs::ip_type::loopback, 8080);
	LIBGS_TEST_CHECK(loopback->address().is_loopback());
	LIBGS_TEST_CHECK_EQ(loopback->port(), uint16_t {8080});
	LIBGS_TEST_CHECK_EQ(std::format("{}", *loopback), "127.0.0.1:8080");

	const libgs::udp_endpoint_wrapper ipv6(libgs::ip_type::v6, 53);
	const asio::ip::udp::endpoint &endpoint = ipv6;
	LIBGS_TEST_CHECK(endpoint.address().is_v6());
	LIBGS_TEST_CHECK_EQ(endpoint.port(), uint16_t {53});
}

void application_environment()
{
	using namespace libgs::app::literals;
	const auto original = libgs::app::current_directory();
	LIBGS_TEST_CHECK(original);
	libgs::test::temporary_directory directory;
	const bool changed = bool(libgs::app::set_current_directory(directory.path()));
	const auto current = libgs::app::current_directory();
	std::error_code equivalent_error;
	const bool current_matches = changed and current and std::filesystem::equivalent(
		*current, directory.path(), equivalent_error);
	if( not current_matches )
	{
		std::cerr << "current directory mismatch: changed=" << changed
			<< ", current='" << (current ? current->string() : "<error>")
			<< "', expected='" << directory.path().string()
			<< "', equivalent_error='" << equivalent_error.message() << "'\n";
	}
	const bool absolute_matches = "relative.txt"_abs ==
		libgs::app::dir_path().value() / "relative.txt";
	LIBGS_TEST_CHECK(libgs::app::set_current_directory(*original));
	LIBGS_TEST_CHECK(changed);
	LIBGS_TEST_CHECK(current_matches);
	LIBGS_TEST_CHECK(absolute_matches);

	constexpr std::string_view key = "LIBGS_TEST_PUBLIC_API_ENV";
	LIBGS_TEST_CHECK(libgs::app::setenv(key, "first"));
	LIBGS_TEST_CHECK(libgs::app::setenv(key, "second", false));
	LIBGS_TEST_CHECK_EQ(libgs::app::getenv(key).value_or(""), "first");
	LIBGS_TEST_CHECK(libgs::app::getenvs()->contains(std::string(key)));
	LIBGS_TEST_CHECK(libgs::app::unsetenv(key));
	LIBGS_TEST_CHECK(libgs::app::current_user());
	LIBGS_TEST_CHECK(libgs::app::home_directory());
	libgs::none_instruction();
}

std::filesystem::path fixture_path;

void dynamic_library()
{
	libgs::library missing(fixture_path.string() + ".missing");
	auto missing_result = missing.load();
	LIBGS_TEST_CHECK(not missing_result);
	LIBGS_TEST_CHECK(missing_result.error() == std::errc::no_such_file_or_directory);

	libgs::library plugin(fixture_path);
	LIBGS_TEST_CHECK(plugin.load());
	LIBGS_TEST_CHECK(plugin.load());
	LIBGS_TEST_CHECK(plugin.is_loaded());
	LIBGS_TEST_CHECK(plugin.exists("libgs_test_twice"));
	LIBGS_TEST_CHECK(not plugin.exists("libgs_test_missing"));
	auto twice = plugin.interface<int(int)>("libgs_test_{}", "twice");
	LIBGS_TEST_CHECK(twice);
	LIBGS_TEST_CHECK_EQ((*twice)(21), 42);

	libgs::library moved(std::move(plugin));
	LIBGS_TEST_CHECK(moved.is_loaded());
	LIBGS_TEST_CHECK(not plugin.is_loaded());
	LIBGS_TEST_CHECK(moved.unload());
	LIBGS_TEST_CHECK(moved.is_loaded());
	libgs::library assigned(fixture_path.string() + ".missing");
	assigned = std::move(moved);
	LIBGS_TEST_CHECK(assigned.is_loaded());
	LIBGS_TEST_CHECK(not moved.is_loaded());
	LIBGS_TEST_CHECK(assigned.unload());
	LIBGS_TEST_CHECK(not assigned.is_loaded());
	LIBGS_TEST_CHECK(assigned.unload());
}

} //namespace

int main(int argc, const char *const argv[])
{
	if( argc < 2 )
		return 2;
	fixture_path = std::filesystem::absolute(argv[1]);
	// Treat the required fixture path as argv[0] for the shared test parser so
	// optional --case/--repeat/--seed arguments can follow it.
	return libgs::test::run(argc - 1, argv + 1, {
		{"flags and parameters", flags_and_parameters},
		{"value and string algorithms", value_and_string_algorithms},
		{"optional, expected, and async helpers", optional_expected_and_async_helpers},
		{"formatting and endpoints", formatting_and_endpoints},
		{"application environment", application_environment},
		{"dynamic library", dynamic_library},
	});
}
