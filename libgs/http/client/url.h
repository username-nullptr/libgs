
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

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_url
{
	LIBGS_DISABLE_COPY(basic_url)
	using string_pool = detail::string_pool<CharT>;

public:
	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = basic_value<CharT>;
	using parameters_t = basic_parameters<CharT>;
	using endpoint_t = typename asio::ip::tcp::endpoint;

public:
	template <typename Arg0, typename...Args>
	basic_url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);

	basic_url(string_view_t url);
	~basic_url();

	basic_url(basic_url &&other) noexcept;
	basic_url &operator=(basic_url &&other) noexcept;

public:
	template <typename Arg0, typename...Args>
	basic_url &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	basic_url &set(string_view_t url);

	basic_url &set_path(string_view_t path);
	basic_url &set_address(string_view_t addr);
	basic_url &set_parameter(string_view_t key, value_t value) noexcept;

public:
	[[nodiscard]] string_view_t path() const noexcept;
	[[nodiscard]] string_view_t address() const noexcept;
	[[nodiscard]] const parameters_t &parameter() const noexcept;

public:
	[[nodiscard]] string_view_t protocol() const noexcept;
	[[nodiscard]] string_t url() const noexcept;

private:
	class impl;
	impl *m_impl;
};

using url = basic_url<char>;
using wurl = basic_url<wchar_t>;

} //namespace libgs::http
#include <libgs/http/client/detail/url.h>


#endif //LIBGS_HTTP_CLIENT_URL_H
