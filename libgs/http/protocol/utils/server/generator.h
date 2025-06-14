
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_SERVER_GENERATOR_H
#define LIBGS_HTTP_PROTOCOL_UTILS_SERVER_GENERATOR_H

#include <libgs/http/protocol/utils/core/generator.h>

namespace libgs::http::protocol
{

template <>
class LIBGS_HTTP_API generator<model::server> final
{
	LIBGS_DISABLE_COPY(generator)

public:
	using next_layer_t = std::shared_ptr<base_generator>;
	using version_t = protocol::version;

	using headers_t = protocol::headers;
	using value_t = libgs::value;

	using cookies_t = protocol::cookies;
	using cookie_t = protocol::cookie;

public:
	explicit generator(version_enum version, const headers_t &req_headers = {});
	explicit generator(const headers_t &req_headers = {}); // default V1.1
	~generator();

	generator(generator &&other) noexcept;
	generator &operator=(generator &&other) noexcept;

public:
	generator &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	generator &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	generator &set_cookie (
		core_concepts::text_p<char> auto &&key, cookie_t cookie
	) noexcept;

	generator &unset_cookie (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] cookies_t &cookies() noexcept;

public:
	generator &set_chunk_attribute(value_t attr) noexcept;
	generator &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

public:
	generator &set_status(status_enum status);
	[[nodiscard]] status_enum status() const noexcept;

	generator &set_redirect (
		core_concepts::text_p<char> auto &&url,
		redirect type = redirect::moved_permanently
	);

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] generator_state pro_state() const noexcept;

	[[nodiscard]] next_layer_t next_layer() noexcept;
	generator &reset() noexcept;

private:
	class impl;
	impl *m_impl;
};

using server_generator = generator<model::server>;

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/server/detail/generator.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_SERVER_GENERATOR_H