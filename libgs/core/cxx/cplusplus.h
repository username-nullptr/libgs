// SPDX-FileCopyrightText: 2024 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_CPLUSPLUS_H
#define LIBGS_CORE_CXX_CPLUSPLUS_H

#ifdef _MSC_VER
# define LIBGS_CPLUSPLUS  _MSVC_LANG
#else
# define LIBGS_CPLUSPLUS  __cplusplus
#endif //_MSC_VER

#if LIBGS_CPLUSPLUS < 202002L
# error "libgs requires at least C++20"
#elif LIBGS_CPLUSPLUS < 202307L
# define LIBGS_STD_CXX 20
#elif LIBGS_CPLUSPLUS < 202600L
# define LIBGS_STD_CXX 23
#else /* experimental */
# define LIBGS_STD_CXX 2b
#endif //LIBGS_CPLUSPLUS

#if defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__arm64__) || defined(__aarch64__)
# define LIBGS_OS_64BIT
#else
# define LIBGS_OS_32BIT
#endif // 32bit & 64bit

#define LIBGS_UNUSED(x)  (void)(x)

#define LIBGS_SHARP_IMPL(a)  #a
#define LIBGS_SHARP(a)  LIBGS_SHARP_IMPL(a)

#define LIBGS_CAT_IMPL(a,b)  a##b
#define LIBGS_CAT(a,b)  LIBGS_CAT_IMPL(a,b)

#define LIBGS_AUTO_XX_NAME(_prefix)  LIBGS_CAT(_prefix,__LINE__)

#define LIBGS_DISABLE_COPY(_class) \
	explicit _class(const _class&) = delete; \
	void operator=(const _class&) = delete; \
	void operator=(const _class&) volatile = delete;

#define LIBGS_DISABLE_MOVE(_class) \
	explicit _class(_class&&) = delete; \
	void operator=(_class&&) = delete; \
	void operator=(_class&&) volatile = delete;

#define LIBGS_DISABLE_COPY_MOVE(_class) \
	LIBGS_DISABLE_COPY(_class) LIBGS_DISABLE_MOVE(_class)

#ifndef LIBGS_CRTP_VIRTUAL
# define LIBGS_CRTP_VIRTUAL
#endif //LIBGS_CRTP_VIRTUAL

#ifndef LIBGS_CRTP_OVERRIDE
# define LIBGS_CRTP_OVERRIDE
#endif //LIBGS_CRTP_OVERRIDE

#ifndef ASIO_HAS_CHRONO
# define ASIO_HAS_CHRONO
#endif //ASIO_HAS_CHRONO

#ifndef SPDLOG_USE_STD_FORMAT
# define SPDLOG_USE_STD_FORMAT
#endif //SPDLOG_USE_STD_FORMAT

#ifdef _MSC_VER
# pragma warning(disable: 4251)
# pragma warning(disable: 4819)
#endif //_MSC_VER


#endif //LIBGS_CORE_CXX_CPLUSPLUS_H
