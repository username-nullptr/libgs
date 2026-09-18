// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

// Provide the single Asio implementation used by gs.core, the other LibGS
// modules, and consumers.  In shared builds ASIO_DYN_LINK exports these symbols
// from gs.core; static builds use ASIO_SEPARATE_COMPILATION instead.
#include <libgs/core/cxx/configs.h>

#if defined(_WIN32) && !defined(_WIN32_WINNT)
# define _WIN32_WINNT 0x0601
#endif

#if !LIBGS_BUILD_STATIC && !defined(ASIO_DYN_LINK)
# define ASIO_DYN_LINK  1
#endif

#include <asio/impl/src.hpp>

#if LIBGS_OPENSSL_SUPPORT
# include <asio/ssl/impl/src.hpp>
#endif
