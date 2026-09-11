// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_CXX_ATTRIBUTES_H
#define LIBGS_WEBSOCKET_CXX_ATTRIBUTES_H

#include <libgs/http/global.h>

#ifdef LIBGS_WEBSOCKET_SHARED
# ifdef gs_websocket_EXPORTS
#  define LIBGS_WEBSOCKET_API  LIBGS_DECL_EXPORT
# else //gs_websocket_EXPORTS
#  define LIBGS_WEBSOCKET_API  LIBGS_DECL_IMPORT
# endif //gs_websocket_EXPORTS

#else //LIBGS_WEBSOCKET_SHARED
# define LIBGS_WEBSOCKET_API
#endif //LIBGS_WEBSOCKET_SHARED

#define LIBGS_WEBSOCKET_VAPI  LIBGS_CORE_VAPI
#define LIBGS_WEBSOCKET_TAPI  LIBGS_CORE_TAPI


#endif //LIBGS_WEBSOCKET_CXX_ATTRIBUTES_H
