// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_STRING_DEQUE_H
#define LIBGS_CORE_STRING_DEQUE_H

#include <libgs/core/string_container.h>
#include <deque>

namespace libgs
{

template <concepts::character CharT, typename...Args>
using basic_string_deque = basic_string_container<CharT,std::deque,Args...>;

using string_deque    = basic_string_deque<char    >;
using wstring_deque   = basic_string_deque<wchar_t >;
using u8string_deque  = basic_string_deque<char8_t >;
using u16string_deque = basic_string_deque<char16_t>;
using u32string_deque = basic_string_deque<char32_t>;

} //namespace libgs


#endif //LIBGS_CORE_STRING_DEQUE_H
