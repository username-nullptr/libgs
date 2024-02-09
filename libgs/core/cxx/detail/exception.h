
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

#ifndef LIBGS_CORE_CXX_DETAIL_EXCEPTION_H
#define LIBGS_CORE_CXX_DETAIL_EXCEPTION_H

namespace libgs
{

inline exception::exception(std::string what) :
	m_what(std::move(what))
{

}

template <typename Arg0, typename...Args>
exception::exception(std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args) :
	m_what(std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

inline const char *exception::what() const noexcept
{
	return m_what.c_str();
}

} //namespace libgs

namespace std
{

template <>
class formatter<libgs::exception, char> : public libgs::no_parse_formatter<char>
{
public:
	auto format(const libgs::exception &ex, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<char>, ex.what());
	}
};

} //namespace std


#endif //LIBGS_CORE_CXX_DETAIL_EXCEPTION_H
