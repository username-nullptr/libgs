// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_CXX_CONCEPTS_H
#define LIBGS_WEBSOCKET_CXX_CONCEPTS_H

#include <libgs/http/global.h>

namespace libgs::websocket { namespace concepts
{

template <typename Token, typename...Args>
concept dis_detach_opt_token =
	libgs::concepts::tf_opt_token<Token,Args...> and
	not is_detached_v<token_unbound_t<Token>>;

} //namespace concepts

namespace core_concepts = libgs::concepts;

} //namespace libgs::websocket


#endif //LIBGS_WEBSOCKET_CXX_CONCEPTS_H
