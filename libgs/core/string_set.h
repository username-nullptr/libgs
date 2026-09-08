// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_STRING_SET_H
#define LIBGS_CORE_STRING_SET_H

#include <libgs/core/string_container.h>
#include <set>

namespace libgs
{

template <concepts::character CharT, typename...Args>
using basic_string_set = basic_string_container<CharT,std::set,Args...>;

using string_set    = basic_string_set<char    >;
using wstring_set   = basic_string_set<wchar_t >;
using u8string_set  = basic_string_set<char8_t >;
using u16string_set = basic_string_set<char16_t>;
using u32string_set = basic_string_set<char32_t>;

} //namespace libgs


#endif //LIBGS_CORE_STRING_SET_H
