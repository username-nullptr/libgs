// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_CXX_ATTRIBUTES_H
#define LIBGS_HTTP_CXX_ATTRIBUTES_H

#include <libgs/core/global.h>

#ifdef gs_http_EXPORTS
# define LIBGS_HTTP_API  LIBGS_DECL_EXPORT
#else //gs_http_EXPORTS
# define LIBGS_HTTP_API  LIBGS_DECL_IMPORT
#endif //gs_http_EXPORTS

#define LIBGS_HTTP_VAPI  LIBGS_CORE_VAPI
#define LIBGS_HTTP_TAPI  LIBGS_CORE_TAPI


#endif //LIBGS_HTTP_CXX_ATTRIBUTES_H
