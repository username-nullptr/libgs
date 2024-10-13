
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

#ifndef LIBGS_CORE_CXX_ATTRIBUTES_H
#define LIBGS_CORE_CXX_ATTRIBUTES_H

#ifdef _MSC_VER

# pragma execution_character_set("utf-8")

# define LIBGS_DECL_EXPORT  __declspec(dllexport)
# define LIBGS_DECL_IMPORT  __declspec(dllimport)
# define LIBGS_DECL_HIDDEN

# define LIBGS_CXX_ATTR_USED    __declspec(used)
# define LIBGS_CXX_ATTR_UNUSED  __declspec(unused)

# define LIBGS_CXX_ATTR_WEAK              __declspec(weak)
# define LIBGS_CXX_ATTR_WEAKREF(_symbol)  __declspec(weakref(_symbol))

#elif defined(__GNUC__)

# if defined(__MINGW32__) || defined(__MINGW32__)
#  define LIBGS_DECL_EXPORT  __declspec(dllexport)
#  define LIBGS_DECL_IMPORT  __declspec(dllimport)
# else
#  define LIBGS_DECL_EXPORT  __attribute__((visibility("default")))
#  define LIBGS_DECL_IMPORT
# endif //__MINGW

# define LIBGS_DECL_HIDDEN  __attribute__((visibility("hidden")))

# define LIBGS_CXX_ATTR_USED    __attribute__((used))
# define LIBGS_CXX_ATTR_UNUSED  __attribute__((unused))

# define LIBGS_CXX_ATTR_WEAK              __attribute__((weak))
# define LIBGS_CXX_ATTR_WEAKREF(_symbol)  __attribute__((weakref(_symbol)))

#else // other compiler

# define LIBGS_DECL_EXPORT
# define LIBGS_DECL_IMPORT
# define LIBGS_DECL_HIDDEN

# define LIBGS_CXX_ATTR_USED
# define LIBGS_CXX_ATTR_UNUSED

# define LIBGS_CXX_ATTR_WEAK
# define LIBGS_CXX_ATTR_WEAKREF(_symbol)

#endif

#ifdef gs_core_EXPORTS
# define LIBGS_CORE_API  LIBGS_DECL_EXPORT
#else //gs_core_EXPORTS
# define LIBGS_CORE_API  LIBGS_DECL_IMPORT
#endif //gs_core_EXPORTS

#define LIBGS_CORE_VAPI
#define LIBGS_CORE_TAPI

#define C_VIRTUAL_FUNC             __attribute_weak__
#define C_VIRTUAL_SYMBOL(_symbol)  __attribute_weakref__(_symbol)


#endif //LIBGS_CORE_CXX_ATTRIBUTES_H
