
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_UTILITIES_H
#define LIBGS_CORE_CXX_UTILITIES_H

#include <libgs/core/cxx/remove_repeat.h>
#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/initialize.h>
#include <utility>

#ifdef __GNUC__
# include <cxxabi.h>
# define LIBGS_ABI_CXA_DEMANGLE(name)  abi::__cxa_demangle(name, nullptr, nullptr, nullptr)
#else //_MSVC
# define LIBGS_ABI_CXA_DEMANGLE(name)  name
#endif //__GNUC__

#define LIBGS_WCHAR(s)  LIBGS_CAT(L,s)

namespace libgs
{

template <typename...Args>
constexpr inline void ignore_unused(Args&&...) {}

using std_type_id = decltype(typeid(void).hash_code());

template <typename T>
constexpr T &remove_const(const T &v);

template <typename T>
constexpr T *remove_const(const T *v);

template <typename T>
constexpr const T &as_const(const T &v);

template <typename T>
constexpr const T *as_const(const T *v);

template <typename T>
LIBGS_CORE_TAPI const char *type_name();
LIBGS_CORE_TAPI const char *type_name(auto &&t);

LIBGS_CORE_VAPI std::string wcstombs(std::wstring_view str);
LIBGS_CORE_VAPI char wcstombs(wchar_t c);

LIBGS_CORE_VAPI std::wstring mbstowcs(std::string_view str);
LIBGS_CORE_VAPI wchar_t mbstowcs(char c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI std::string xxtombs(std::basic_string_view<CharT> str);

LIBGS_CORE_TAPI char xxtombs(concepts::char_type auto c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI std::wstring xxtowcs(std::basic_string_view<CharT> str);

LIBGS_CORE_TAPI wchar_t xxtowcs(concepts::char_type auto c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI std::basic_string<CharT> mbstoxx(std::string_view str);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI CharT mbstoxx(char c);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI std::basic_string<CharT> wcstoxx(std::wstring_view str);

template <concepts::char_type CharT>
LIBGS_CORE_TAPI CharT wcstoxx(wchar_t c);

struct LIBGS_CORE_VAPI string_wrapper
{
	std::string value;

	string_wrapper() = default;
	string_wrapper(const char *value);
	string_wrapper(const wchar_t *value);

	string_wrapper(const std::string &value);
	string_wrapper(std::string &&value);

	string_wrapper(std::string_view value);
	string_wrapper(std::wstring_view value);

	operator std::string&();
	operator const std::string&() const;

	std::string &operator*();
	std::string *operator->();
};

LIBGS_CORE_TAPI auto get_executor_helper(concepts::schedulable auto &&exec);

enum class ip_type {
	v4, v6, loopback
};

template <typename Protocol>
struct LIBGS_CORE_TAPI basic_endpoint_wrapper
{
	using endpoint_t = asio::ip::basic_endpoint<Protocol>;
	endpoint_t value;

	basic_endpoint_wrapper() = default;
	basic_endpoint_wrapper(string_wrapper address, uint16_t port);
	basic_endpoint_wrapper(string_wrapper address);

	basic_endpoint_wrapper(ip_type type, uint16_t port);
	basic_endpoint_wrapper(ip_type type);

	template <typename...Args>
	basic_endpoint_wrapper(Args&&...args) requires
		concepts::constructible<endpoint_t,Args&&...>;

	operator endpoint_t&();
	operator const endpoint_t&() const;

	endpoint_t &operator*();
	endpoint_t *operator->();
};

using tcp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::tcp>;
using udp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::udp>;

} //namespace libgs
#include <libgs/core/cxx/detail/utilities.h>


#endif //LIBGS_CORE_CXX_UTILITIES_H
