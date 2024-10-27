
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_CORE_CXX_INITIALIZE_H
#define LIBGS_CORE_CXX_INITIALIZE_H

#include <libgs/core/cxx/cplusplus.h>
#include <libgs/core/cxx/attributes.h>

#define LIBGS_AUTO_FUNC_NAME  LIBGS_AUTO_XX_NAME(__libgs_auto_xx_name_)

#define LIBGS_REGISTRATION \
	static void LIBGS_AUTO_FUNC_NAME(); \
	namespace { \
		struct LIBGS_AUTO_XX_NAME(__libgs_auto_register_) { \
			LIBGS_AUTO_XX_NAME(__libgs_auto_register_)() { \
				LIBGS_AUTO_FUNC_NAME(); \
			} \
		}; \
	} \
	static const LIBGS_AUTO_XX_NAME(__libgs_auto_register_) LIBGS_AUTO_XX_NAME(__auto_register_); \
	static void LIBGS_AUTO_FUNC_NAME()

#ifdef _MSC_VER
# define LIBGS_PLUGIN_REGISTRATION LIBGS_REGISTRATION
#else //GNU & Clang ...
# define LIBGS_PLUGIN_REGISTRATION \
	LIBGS_GNU_ATTR_INIT static void LIBGS_AUTO_XX_NAME(__libgs_auto_register_)()
#endif //_MSC_VER


#endif //LIBGS_CORE_CXX_INITIALIZE_H
