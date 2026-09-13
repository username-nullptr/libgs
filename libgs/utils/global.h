// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_GLOBAL_H
#define LIBGS_UTILS_GLOBAL_H

#include <libgs/core/global.h>

#if LIBGS_BUILD_STATIC
# define LIBGS_UTILS_API
#elif defined(gs_utils_EXPORTS)
# define LIBGS_UTILS_API  LIBGS_DECL_EXPORT
#else //gs_utils_EXPORTS
# define LIBGS_UTILS_API  LIBGS_DECL_IMPORT
#endif //gs_utils_EXPORTS

#define LIBGS_UTILS_VAPI
#define LIBGS_UTILS_TAPI

namespace libgs::utils
{

[[nodiscard]] LIBGS_UTILS_API asio::thread_pool &thread_pool() noexcept;

} //namespace libgs::utils


#endif //LIBGS_UTILS_GLOBAL_H
