
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

#include <libgs/http/helper_base.h>

namespace libgs::http
{

class LIBGS_HTTP_API response_helper final
{
	LIBGS_DISABLE_COPY(response_helper)

public:
	using next_layer_t = std::shared_ptr<helper_base>;
	using headers_t = http::headers;
	using cookies_t = http::cookies;
	using value_t = libgs::value;

public:
	explicit response_helper(version_enum version, const headers_t &req_headers = {});
	explicit response_helper(const headers_t &req_headers = {}); // default V1.1
	~response_helper();

	response_helper(response_helper &&other) noexcept;
	response_helper &operator=(response_helper &&other) noexcept;

public:
	response_helper &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	response_helper &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	response_helper &set_cookie (
		http::cookie cookie
	) noexcept;

	response_helper &unset_cookie (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] cookies_t &cookies() noexcept;

public:
	response_helper &set_chunk_attribute(value_t attr) noexcept;
	response_helper &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

public:
	response_helper &set_status(status_enum status);
	[[nodiscard]] status_enum status() const noexcept;

	response_helper &set_redirect (
		core_concepts::text_p<char> auto &&url,
		redirect type = redirect::moved_permanently
	);

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] helper_state pro_state() const noexcept;

	[[nodiscard]] next_layer_t next_layer() noexcept;
	response_helper &reset() noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/server/detail/response_helper.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H