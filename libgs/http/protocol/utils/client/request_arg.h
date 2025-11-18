
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H

#include <libgs/http/protocol/utils/client/url.h>

namespace libgs::http::protocol
{

class LIBGS_HTTP_API request_arg
{
public:
	using url_t = protocol::url;
	using value_t = libgs::value;

	using header_t = protocol::header;
	using headers_t = protocol::headers;
	using cookies_t = protocol::cookie_values;

public:
	request_arg(url_t url);
	request_arg();
	~request_arg();

	request_arg(const request_arg &other) noexcept;
	request_arg &operator=(const request_arg &other) noexcept;

	request_arg(request_arg &&other) noexcept;
	request_arg &operator=(request_arg &&other) noexcept;

public:
	request_arg &set_url(url_t url);
	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] url_t &url() noexcept;

public:
	request_arg &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	request_arg &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	request_arg &set_cookie (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	request_arg &unset_cookie (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &cookies() const noexcept;
	[[nodiscard]] headers_t &cookies() noexcept;

public:
	request_arg &set_chunk_attribute(value_t attr) noexcept;
	request_arg &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/protocol/utils/client/detail/request_arg.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H