// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_CXX_CONCEPTS_H
#define LIBGS_WEBSOCKET_CXX_CONCEPTS_H

#include <libgs/http/global.h>
#include <libgs/core/async_expected.h>

namespace libgs::websocket { namespace concepts
{

template <typename T>
concept buffer =
	libgs::concepts::buffer<T> and
	not libgs::concepts::array_buffer<T>;

} //namespace concepts

namespace core_concepts = libgs::concepts;

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_CXX_CONCEPTS_H
