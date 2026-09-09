// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ALGORITHM_MISC_H
#define LIBGS_CORE_ALGORITHM_MISC_H

#include <libgs/core/global.h>

namespace libgs
{

[[nodiscard]] LIBGS_CORE_TAPI auto from_percent_encoding (
	concepts::any_string_p auto &&str, char percent = '%'
);

template <concepts::any_text_p Str, concepts::text_p<strtls::get_char_t<Str>> StrArg =
		  std::basic_string_view<strtls::get_char_t<Str>>>
[[nodiscard]] LIBGS_CORE_TAPI auto to_percent_encoding (
	const Str &str, StrArg &&exclude = {}, StrArg &&include = {}, char percent = '%'
);

/*
 * return:
 *   <0: Completely mismatched.
 *   =0: equality.
 *   >0: The smaller the value is, the higher the matching degree will be.
 */
template <concepts::any_text_p Str, concepts::text_p<strtls::get_char_t<Str>> StrArg>
[[nodiscard]] LIBGS_CORE_TAPI int32_t wildcard_match (
	const Str &rule, const StrArg &str
);

} //namespace libgs
#include <libgs/core/algorithm/detail/misc.h>


#endif //LIBGS_CORE_ALGORITHM_MISC_H
