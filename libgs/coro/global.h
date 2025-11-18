
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
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

# define LIBGS_CORO_VAPI  LIBGS_CORE_VAPI
# define LIBGS_CORO_TAPI  LIBGS_CORE_TAPI


#endif //LIBGS_CORO_GLOBAL_H
