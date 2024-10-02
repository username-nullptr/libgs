
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_HELPER_H
#define LIBGS_HTTP_CLIENT_REQUEST_HELPER_H

#include <libgs/http/client/request.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_request_helper
{
	LIBGS_DISABLE_COPY(basic_request_helper)

public:
	using request_t = basic_client_request<CharT>;
	using string_view_t = std::basic_string_view<CharT>;
	using string_t = std::basic_string<CharT>;
	using headers_t = basic_headers<CharT>;

public:
	basic_request_helper(string_view_t version, request_t &request);
	explicit basic_request_helper(request_t &request); // default v1.1
	~basic_request_helper();

	basic_request_helper(basic_request_helper &&other) noexcept;
	basic_request_helper &operator=(basic_request_helper &&other) noexcept;

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] const request_t &request() const noexcept;
	[[nodiscard]] request_t &request() noexcept;

private:
	class impl;
	impl *m_impl;
};

using request_helper = basic_request_helper<char>;
using wrequest_helper = basic_request_helper<wchar_t>;

} //namespace libgs::http
#include <libgs/http/client/detail/request_helper.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_HELPER_H