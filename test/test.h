// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_TEST_TEST_H
#define LIBGS_TEST_TEST_H

#include <exception>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

namespace libgs::test
{

class temporary_directory
{
public:
	temporary_directory()
	{
		static std::atomic_uint64_t sequence {0};
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		m_path = std::filesystem::temp_directory_path() /
			("libgs-test-" + std::to_string(stamp) + '-' +
			 std::to_string(sequence.fetch_add(1)));
		std::filesystem::create_directories(m_path);
	}

	~temporary_directory()
	{
		std::error_code error;
		std::filesystem::remove_all(m_path, error);
	}

	temporary_directory(const temporary_directory&) = delete;
	temporary_directory &operator=(const temporary_directory&) = delete;

	[[nodiscard]] const std::filesystem::path &path() const noexcept
	{
		return m_path;
	}

private:
	std::filesystem::path m_path;
};

struct test_case
{
	std::string_view name;
	void (*function)();
};

[[noreturn]] inline void fail (
	std::string_view expression,
	const std::source_location &location = std::source_location::current()
)
{
	throw std::runtime_error (
		std::string(location.file_name()) + ':' + std::to_string(location.line()) +
		": check failed: " + std::string(expression)
	);
}

inline int run(std::initializer_list<test_case> tests)
{
	size_t failures = 0;
	for(const auto &test : tests)
	{
		try
		{
			test.function();
			std::cout << "[PASS] " << test.name << '\n';
		}
		catch(const std::exception &error)
		{
			failures++;
			std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
		}
		catch(...)
		{
			failures++;
			std::cerr << "[FAIL] " << test.name << ": unknown exception\n";
		}
	}

	std::cout << (tests.size() - failures) << '/' << tests.size()
		<< " tests passed\n";
	return failures == 0 ? 0 : 1;
}

} //namespace libgs::test

#define LIBGS_TEST_CHECK(expression) \
	do { \
		if( not static_cast<bool>(expression) ) \
			::libgs::test::fail(#expression); \
	} while(false)

#define LIBGS_TEST_CHECK_EQ(actual, expected) \
	do { \
		if( not ((actual) == (expected)) ) \
			::libgs::test::fail(#actual " == " #expected); \
	} while(false)

#endif //LIBGS_TEST_TEST_H
