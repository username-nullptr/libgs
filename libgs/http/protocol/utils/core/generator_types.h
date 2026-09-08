// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_GENERATOR_TYPES_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_GENERATOR_TYPES_H

#include <libgs/http/protocol/types.h>

namespace libgs::http
{

template <protocol_model>
class generator {};

enum class generator_state {
	header, content_length, chunk, finish
};

} //namespace libgs::http


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_GENERATOR_TYPES_H
