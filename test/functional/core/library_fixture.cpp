// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#if defined(_WIN32)
# define LIBGS_TEST_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
# define LIBGS_TEST_EXPORT __attribute__((visibility("default")))
#else
# define LIBGS_TEST_EXPORT
#endif

extern "C" LIBGS_TEST_EXPORT int libgs_test_twice(int value)
{
	return value * 2;
}
