// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_FORMATTER_H
#define LIBGS_CORE_CXX_FORMATTER_H

#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/attributes.h>
#include <algorithm>
#include <format>

namespace libgs
{

template <concepts::character CharT>
struct LIBGS_CORE_TAPI no_parse_formatter
{
	constexpr auto parse(std::basic_format_parse_context<CharT> &context) noexcept {
		return std::ranges::find(context, static_cast<CharT>(0x7D));
	}
};

} //namespace libgs


#endif //LIBGS_CORE_CXX_FORMATTER_H
