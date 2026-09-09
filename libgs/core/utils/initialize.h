// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_INITIALIZE_H
#define LIBGS_CORE_UTILS_INITIALIZE_H

#include <libgs/core/cxx/cplusplus.h>
#include <libgs/core/cxx/attributes.h>

#define LIBGS_AUTO_FUNC_NAME  LIBGS_AUTO_XX_NAME(__libgs_auto_xx_name_)

#define LIBGS_DEFAULT_REGISTRATION \
	static void LIBGS_AUTO_FUNC_NAME(); \
	namespace { \
		struct LIBGS_DECL_HIDDEN LIBGS_AUTO_XX_NAME(__libgs_auto_register_) { \
			LIBGS_AUTO_XX_NAME(__libgs_auto_register_)() { \
				LIBGS_AUTO_FUNC_NAME(); \
			} \
		}; \
	} \
	static const LIBGS_AUTO_XX_NAME(__libgs_auto_register_) LIBGS_AUTO_XX_NAME(__auto_register_); \
	static void LIBGS_AUTO_FUNC_NAME()

#ifdef _MSC_VER
# define LIBGS_REGISTRATION LIBGS_DEFAULT_REGISTRATION
#else //GNU & Clang ...
# define LIBGS_REGISTRATION \
	LIBGS_GNU_ATTR_INIT static void LIBGS_AUTO_XX_NAME(__libgs_auto_register_)()
#endif //_MSC_VER


#endif //LIBGS_CORE_UTILS_INITIALIZE_H
