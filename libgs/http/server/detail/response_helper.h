
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H

namespace libgs::http
{

response_helper &response_helper::set_header
(core_concepts::text_p<char> auto &&key, value_t value) noexcept
{
	next_layer()->set_header(std::forward<decltype(key)>(key), std::move(value));
	return *this;
}

response_helper &response_helper::unset_header
(const core_concepts::text_p<char> auto &key) noexcept
{
	next_layer()->unset_header(key);
	return *this;
}

response_helper &response_helper::set_cookie
(core_concepts::text_p<char> auto &&key, http::cookie cookie) noexcept
{
	cookies()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(cookie);
	return *this;
}

response_helper &response_helper::unset_cookie
(const core_concepts::text_p<char> auto &key) noexcept
{
	cookies().erase(key);
	return *this;
}

response_helper &response_helper::set_redirect(core_concepts::text_p<char> auto &&url, redirect type)
{
	switch(type)
	{
#define X_MACRO(e,v,d) case redirect::e : set_status(v); break;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		default: throw runtime_error (
			"libgs::http::response_helper::redirect: Invalid redirect type: '{}'.", type
		);
	}
	set_header(header::location, std::forward<decltype(url)>(url));
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_HELPER_H
