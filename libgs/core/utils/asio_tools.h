// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_ASIO_TOOLS_H
#define LIBGS_CORE_UTILS_ASIO_TOOLS_H

#include <libgs/core/utils/token_concepts.h>
#include <libgs/core/utils/asio_concepts.h>
#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/tools.h>

namespace libgs
{

using mutable_buffer = asio::mutable_buffer;

class LIBGS_CORE_VAPI const_buffer : public asio::const_buffer
{
public:
	using asio::const_buffer::const_buffer;
	const_buffer &operator=(const const_buffer&) = default;
	const_buffer(const asio::const_buffer &buf);
	const_buffer(const mutable_buffer &buf);
	const_buffer(const char *buf);
	const_buffer(const std::string &buf);
	const_buffer(std::string_view buf);
	const_buffer &operator=(const mutable_buffer &buf);
};

template <typename...Args>
[[nodiscard]] LIBGS_CORE_TAPI auto buffer(Args&&...args)
	requires (sizeof...(Args) > 0);

template <typename Token>
[[nodiscard]] LIBGS_CORE_TAPI
decltype(auto) unbound_token(Token &&token);

template <typename Token>
struct token_unbound
{
	using type = std::remove_cvref_t <
		decltype(unbound_token(std::declval<Token>()))
	>;
};

template <typename Token>
using token_unbound_t = token_unbound<Token>::type;

[[nodiscard]] LIBGS_CORE_TAPI
decltype(auto) get_executor_helper(concepts::sched auto &&exec);

} //namespace libgs
#include <libgs/core/utils/detail/asio_tools.h>


#endif //LIBGS_CORE_UTILS_ASIO_TOOLS_H
