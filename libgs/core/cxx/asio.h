// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_ASIO_H
#define LIBGS_CORE_CXX_ASIO_H

#include <libgs/core/cxx/configs.h>

#if LIBGS_USING_BOOST_ASIO

# include <boost/asio.hpp>
# include <boost/asio/version.hpp>

# if LIBGS_OPENSSL_SUPPORT
#  include <boost/asio/ssl.hpp>
# endif //LIBGS_OPENSSL_SUPPORT

namespace asio = boost::asio;

#else //LIBGS_USING_BOOST_ASIO

# include <asio.hpp>
# include <asio/version.hpp>

# if LIBGS_OPENSSL_SUPPORT
#  include <asio/ssl.hpp>
# endif //LIBGS_OPENSSL_SUPPORT

#endif //LIBGS_USING_BOOST_ASIO

#if LIBGS_USING_BOOST_ASIO
# if BOOST_ASIO_VERSION < 103600
#  define LIBGS_ASIO_LEGACY_AWAITABLE_CONTEXT 1
# else //BOOST_ASIO_VERSION
#  define LIBGS_ASIO_LEGACY_AWAITABLE_CONTEXT 0
# endif //BOOST_ASIO_VERSION

#elif ASIO_VERSION < 103600
# define LIBGS_ASIO_LEGACY_AWAITABLE_CONTEXT 1
#else
# define LIBGS_ASIO_LEGACY_AWAITABLE_CONTEXT 0
#endif


#endif //LIBGS_CORE_CXX_ASIO_H
