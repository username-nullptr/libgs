
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#include "session_base.h"

namespace libgs::http::detail
{

static std::atomic<uint64_t> g_lifecycle = 60;

std::chrono::seconds session_duration_base::global_lifecycle() noexcept
{
	return std::chrono::seconds(g_lifecycle);
}

std::atomic<uint64_t> &session_duration_base::_lifecycle() noexcept
{
	return g_lifecycle;
}

template <concept_char_type CharT>
using ptr = std::shared_ptr<basic_session<CharT>>;

std::string &session_base<char>::cookie_key() noexcept
{
	static std::string key = "session";
	return key;
}

std::map<std::string_view, ptr<char>> &session_base<char>::session_map() noexcept
{
	static std::map<std::string_view, ptr> map;
	return map;
}

shared_mutex &session_base<char>::map_rwlock() noexcept
{
	static shared_mutex rwlock;
	return rwlock;
}

std::wstring &session_base<wchar_t>::cookie_key() noexcept
{
	static std::wstring key = L"session";
	return key;
}

std::map<std::wstring_view, ptr<wchar_t>> &session_base<wchar_t>::session_map() noexcept
{
	static std::map<std::wstring_view, ptr> map;
	return map;
}

shared_mutex &session_base<wchar_t>::map_rwlock() noexcept
{
	static shared_mutex rwlock;
	return rwlock;
}

} //namespace libgs::http::detail