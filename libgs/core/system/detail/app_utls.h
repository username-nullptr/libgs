
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_SYSTEM_DETAIL_APP_UTILS_H
#define LIBGS_CORE_SYSTEM_DETAIL_APP_UTILS_H

namespace libgs::app:: inline literals
{

inline path_t operator""_abs(const char *path, size_t len)
{
	auto expected = absolute_path(std::string(path, len));
	if( not expected )
	{
		system_error::loc_throw (
			expected.error(), R"(libgs::app::operator""_abs<char>)"
		);
	}
	return *expected;
}

inline path_t operator""_abs(const wchar_t *path, size_t len)
{
	auto expected = absolute_path(std::wstring(path, len));
	if( not expected )
	{
		system_error::loc_throw (
			expected.error(), R"(libgs::app::operator""_abs<char>)"
		);
	}
	return *expected;
}

} //namespace libgs::app::literals


#endif //LIBGS_CORE_SYSTEM_DETAIL_APP_UTILS_H
