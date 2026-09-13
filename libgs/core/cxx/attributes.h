// SPDX-FileCopyrightText: 2024 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_ATTRIBUTES_H
#define LIBGS_CORE_CXX_ATTRIBUTES_H

#include <libgs/core/cxx/configs.h>

#ifdef _MSC_VER

# pragma execution_character_set("utf-8")

# define LIBGS_DECL_EXPORT  __declspec(dllexport)
# define LIBGS_DECL_IMPORT  __declspec(dllimport)
# define LIBGS_DECL_HIDDEN

# define LIBGS_CXX_ATTR_USED    __declspec(used)
# define LIBGS_CXX_ATTR_UNUSED  __declspec(unused)

# define LIBGS_CXX_ATTR_WEAK              __declspec(weak)
# define LIBGS_CXX_ATTR_WEAKREF(_symbol)  __declspec(weakref(_symbol))

#define LIBGS_CXX_ATTR_NOVTABLE  __declspec(novtable)

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

# define LIBGS_GNU_ATTR_INIT  __attribute__((constructor))
# define LIBGS_GNU_ATTR_EXIT  __attribute__((destructor))

#define LIBGS_CXX_ATTR_NOVTABLE  __attribute__(novtable)

#else // other compiler

# define LIBGS_DECL_EXPORT
# define LIBGS_DECL_IMPORT
# define LIBGS_DECL_HIDDEN

# define LIBGS_CXX_ATTR_USED
# define LIBGS_CXX_ATTR_UNUSED

# define LIBGS_CXX_ATTR_WEAK
# define LIBGS_CXX_ATTR_WEAKREF(_symbol)

#define LIBGS_CXX_ATTR_NOVTABLE

#endif //_MSC_VER

#if LIBGS_BUILD_STATIC
# define LIBGS_CORE_API
#elif defined(gs_core_EXPORTS)
# define LIBGS_CORE_API  LIBGS_DECL_EXPORT
#else //gs_core_EXPORTS
# define LIBGS_CORE_API  LIBGS_DECL_IMPORT
#endif //gs_core_EXPORTS

#define LIBGS_CORE_VAPI
#define LIBGS_CORE_TAPI

#define C_VIRTUAL_FUNC             LIBGS_CXX_ATTR_WEAK
#define C_VIRTUAL_SYMBOL(_symbol)  LIBGS_CXX_ATTR_WEAKREF(_symbol)


#endif //LIBGS_CORE_CXX_ATTRIBUTES_H
