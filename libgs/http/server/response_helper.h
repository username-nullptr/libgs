
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

#ifndef LIBGS_HTTP_SERVER_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_RESPONSE_HELPER_H

#include <libgs/http/types.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_response_helper final
{
	LIBGS_DISABLE_COPY(basic_response_helper)
	using string_pool = detail::string_pool<CharT>;

public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = basic_value<char_t>;
	using value_set_t = basic_value_set<char_t>;

	using header_t = basic_header<char_t>;
	using headers_t = basic_headers<char_t>;

	using cookie_t = basic_cookie<char_t>;
	using cookies_t = basic_cookies<char_t>;

	using key_init_t = basic_key_init<char_t>;
	using attr_init_t = basic_attr_init<char_t>;
	using pair_init_t = basic_key_attr_init<char_t>;
	using cookie_init_t = basic_cookie_init<char_t>;

public:
	explicit basic_response_helper(version_t version, const headers_t &request_headers = {});
	explicit basic_response_helper(const headers_t &request_headers = {}); // default V1.1
	~basic_response_helper();

	basic_response_helper(basic_response_helper &&other) noexcept ;
	basic_response_helper &operator=(basic_response_helper &&other) noexcept;

public:
	basic_response_helper &set_status(status_t status);
	basic_response_helper &set_header(pair_init_t headers) noexcept;
	basic_response_helper &set_cookie(cookie_init_t headers) noexcept;
	basic_response_helper &set_chunk_attribute(attr_init_t attributes) noexcept;

	template <typename...Args>
	basic_response_helper &set_header(Args&&...args) noexcept requires
		concepts::set_key_attr_params<char_t,Args...>;

	template <typename...Args>
	basic_response_helper &set_cookie(Args&&...args) noexcept requires
		concepts::set_cookie_params<char_t,Args...>;

	template <typename...Args>
	basic_response_helper &set_chunk_attribute(Args&&...args) noexcept requires
		concepts::set_attr_params<char_t,Args...>;

	basic_response_helper &set_redirect (
		core_concepts::basic_string_type<char_t> auto &&url,
		redirect type = redirect::moved_permanently
	);

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

public:
	[[nodiscard]] version_t version() const noexcept;
	[[nodiscard]] status_t status() const noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] const value_set_t &chunk_attributes() const noexcept;

public:
	template <typename...Args>
	basic_response_helper &unset_header(Args&&...args) noexcept requires
		concepts::unset_pair_params<char_t,Args...>;

	template <typename...Args>
	basic_response_helper &unset_cookie(Args&&...args) noexcept requires
		concepts::unset_pair_params<char_t,Args...>;

	template <typename...Args>
	basic_response_helper &unset_chunk_attribute(Args&&...args) noexcept requires
		concepts::unset_attr_params<char_t,Args...>;

	basic_response_helper &unset_header(key_init_t headers) noexcept;
	basic_response_helper &clear_header() noexcept;

	basic_response_helper &unset_cookie(key_init_t headers) noexcept;
	basic_response_helper &clear_cookie() noexcept;

	basic_response_helper &unset_chunk_attribute(attr_init_t headers) noexcept;
	basic_response_helper &clear_chunk_attribute() noexcept;
	basic_response_helper &reset();

private:
	class impl;
	impl *m_impl;
};

using response_helper = basic_response_helper<char>;
using wresponse_helper = basic_response_helper<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/response_helper.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H