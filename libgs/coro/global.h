// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_GLOBAL_H
#define LIBGS_CORO_GLOBAL_H

#include <libgs/core/execution.h>

#ifdef LIBGS_CORO_SHARED
# ifdef gs_coro_EXPORTS
#  define LIBGS_CORO_API  LIBGS_DECL_EXPORT
# else //gs_coro_EXPORTS
#  define LIBGS_CORO_API  LIBGS_DECL_IMPORT
# endif //gs_coro_EXPORTS

#else //LIBGS_CORO_SHARED
# define LIBGS_CORO_API
#endif //LIBGS_CORO_SHARED

#define LIBGS_CORO_VAPI  LIBGS_CORE_VAPI
#define LIBGS_CORO_TAPI  LIBGS_CORE_TAPI


#endif //LIBGS_CORO_GLOBAL_H
