
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_ARG_H
#define LIBGS_HTTP_CLIENT_REQUEST_ARG_H

#include <libgs/http/client/url.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_request_arg
{
public:
	using char_t = CharT;
	using url_t = basic_url<char_t>;

	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = basic_value<char_t>;
	using value_set_t = basic_value_set<char_t>;

	using header_t = basic_header<char_t>;
	using headers_t = basic_headers<char_t>;
	using cookies_t = basic_cookie_values<char_t>;

	using set_init_list_t = basic_set_init_list<char_t>;
	using map_init_list_t = basic_map_init_list<char_t>;
	using map_key_init_list_t = basic_map_key_init_list<char_t>;

public:
	basic_request_arg(url_t url);
	basic_request_arg();
	~basic_request_arg();

	basic_request_arg(const basic_request_arg &other) noexcept;
	basic_request_arg &operator=(const basic_request_arg &other) noexcept;

	basic_request_arg(basic_request_arg &&other) noexcept;
	basic_request_arg &operator=(basic_request_arg &&other) noexcept;

public:
	basic_request_arg &set_url(url_t url);
	basic_request_arg &set_header(map_init_list_t headers) noexcept;
	basic_request_arg &set_cookie(map_init_list_t cookies) noexcept;
	basic_request_arg &set_chunk_attribute(set_init_list_t attributes) noexcept;

	template <typename...Args>
	basic_request_arg &set_header(Args&&...args) noexcept requires
		concepts::set_map_params<char_t,Args...>;

	template <typename...Args>
	basic_request_arg &set_cookie(Args&&...args) noexcept requires
		concepts::set_map_params<char_t,Args...>;

	template <typename...Args>
	basic_request_arg &set_chunk_attribute(Args&&...args) noexcept requires
		concepts::set_set_params<char_t,Args...>;

public:
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] const value_set_t &chunk_attributes() const noexcept;

	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] url_t &url() noexcept;

public:
	template <typename...Args>
	basic_request_arg &unset_header(Args&&...args) requires
		concepts::unset_map_params<char_t,Args...>;

	template <typename...Args>
	basic_request_arg &unset_cookie(Args&&...args) requires
		concepts::unset_map_params<char_t,Args...>;

	template <typename...Args>
	basic_request_arg &unset_chunk_attribute(Args&&...args) requires
		concepts::unset_set_params<char_t,Args...>;

	basic_request_arg &unset_header(map_key_init_list_t headers) noexcept;
	basic_request_arg &unset_cookie(map_key_init_list_t cookies) noexcept;
	basic_request_arg &unset_chunk_attribute(set_init_list_t attributes) noexcept;
	basic_request_arg &reset();

private:
	class impl;
	impl *m_impl;
};

using request_arg  = basic_request_arg<char>;
using wrequest_arg = basic_request_arg<wchar_t>;

} //namespace libgs::http
#include <libgs/http/client/detail/request_arg.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_ARG_H