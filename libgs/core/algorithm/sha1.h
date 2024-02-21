
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                     *
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

#ifndef LIBGS_CORE_ALGORITHM_SHA1_H
#define LIBGS_CORE_ALGORITHM_SHA1_H

#include <libgs/core/global.h>

namespace libgs
{

class sha1_impl;

class LIBGS_CORE_API sha1
{
public:
	sha1(std::string_view text = {});
	sha1(std::wstring_view text = {});
	sha1(const sha1 &other);
	sha1(sha1 &&other) noexcept;
	~sha1();

public:
	sha1 &operator=(const sha1 &other);
	sha1 &operator=(sha1 &&other) noexcept;

public:
	sha1 &append(uint8_t x);
	sha1 &append(char c);
	sha1 &append(wchar_t c);
	sha1 &append(const void *data, size_t size);
	sha1 &append(std::string_view text);
	sha1 &append(std::wstring_view text);

public:
	void operator+=(uint8_t x);
	void operator+=(char c);
	void operator+=(wchar_t c);
	void operator+=(const std::string &text);
	void operator+=(const std::wstring &text);

public:
	sha1 &finalize();
	[[nodiscard]] std::string hex(bool upper_case = true) const;
	[[nodiscard]] std::string base64() const;

public:
	[[nodiscard]] std::wstring whex(bool upper_case = true) const;
	[[nodiscard]] std::wstring wbase64() const;

private:
	sha1_impl *m_impl;
};

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_SHA1_H
