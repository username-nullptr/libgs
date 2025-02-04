
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

#ifndef LIBGS_HTTP_CLIENT_URL_H
#define LIBGS_HTTP_CLIENT_URL_H

#include <libgs/http/types.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_url
{
	using string_pool = detail::string_pool<CharT>;

public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = basic_value<char_t>;
	using parameters_t = basic_parameters<char_t>;

	using pair_init_t = basic_key_attr_init<char_t>;
	using key_init_t = basic_key_init<char_t>;

public:
	template <typename Arg0, typename...Args>
	basic_url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	basic_url(string_view_t url);
	basic_url();
	~basic_url();

	basic_url(const basic_url &other);
	basic_url &operator=(const basic_url &other);

	basic_url(basic_url &&other) noexcept;
	basic_url &operator=(basic_url &&other) noexcept;

public:
	template <typename Arg0, typename...Args>
	basic_url &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	basic_url &set(string_view_t url);

	basic_url &set_address(string_view_t addr);
	basic_url &set_port(uint16_t port);
	basic_url &set_path(string_view_t path);

public:
	template <typename...Args>
	basic_url &set_parameter(Args&&...args) noexcept requires
		concepts::set_key_attr_params<char_t,Args...>;

	basic_url &set_parameter(pair_init_t headers) noexcept;

	template <typename...Args>
	basic_url &unset_parameter(Args&&...args) noexcept requires
		concepts::unset_pair_params<char_t,Args...>;

	basic_url &unset_parameter(key_init_t keys) noexcept;
	basic_url &clear_parameter() noexcept;

public:
	[[nodiscard]] string_view_t protocol() const noexcept;
	[[nodiscard]] string_view_t address() const noexcept;
	[[nodiscard]] uint16_t port() const noexcept;
	[[nodiscard]] string_view_t path() const noexcept;
	[[nodiscard]] const parameters_t &parameters() const noexcept;

public:
	[[nodiscard]] string_t to_string() const noexcept;
	[[nodiscard]] operator string_t() const noexcept;

private:
	class impl;
	impl *m_impl;
};

using url = basic_url<char>;
using wurl = basic_url<wchar_t>;

} //namespace libgs::http
#include <libgs/http/client/detail/url.h>


#endif //LIBGS_HTTP_CLIENT_URL_H
