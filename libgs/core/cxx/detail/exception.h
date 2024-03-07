
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

template <typename Arg0, typename...Args>
runtime_error::runtime_error(std::format_string<Arg0, Args...> fmt_value, Arg0 &&arg0, Args&&...args) :
	std::runtime_error(std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <typename Arg0, typename...Args>
system_error::system_error(std::error_code ec, std::format_string<Arg0, Args...> fmt_value, Arg0 &&arg0, Args&&...args) :
	std::system_error(ec, std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <typename Arg0, typename...Args>
system_error::system_error(int v, const std::error_category &ecat, std::format_string<Arg0, Args...> fmt_value, Arg0 &&arg0, Args&&...args) :
	std::system_error(v, std::move(ecat), std::format(fmt_value, std::forward<Arg0>(arg0), std::forward<Args>(args)...)) 
{

}

} //namespace libgs

namespace std
{

template <>
class formatter<std::exception, char> 
{
public:
	auto format(const std::exception &ex, auto &context) const {
		return m_formatter.format(ex.what(), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<const char*, char> m_formatter;
};

template <libgs::concept_char_type CharT>
class formatter<libgs::system_error, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const libgs::system_error &ex, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{} ({})", ex.what(), ex.code().value());
		else
			return format_to(context.out(), L"{} ({})", ex.what(), ex.code().value());
	}
};

} //namespace std


#endif //LIBGS_CORE_CXX_DETAIL_EXCEPTION_H
