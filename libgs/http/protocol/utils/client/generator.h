
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H

#include <libgs/http/protocol/utils/core/generator.h>
#include <libgs/http/protocol/utils/client/request_arg.h>
#include <libgs/http/protocol/utils/client/url.h>

namespace libgs::http::protocol
{

template <>
class LIBGS_HTTP_API generator<model::client> final
{
	LIBGS_DISABLE_COPY(generator)

public:
	using next_layer_t = std::shared_ptr<base_generator>;
	using version_t = protocol::version;

	using request_arg_t = request_arg;
	using url_t = protocol::url;

	using value_t = request_arg_t::value_t;
	using header_t = request_arg_t::header_t;

	using headers_t = request_arg_t::headers_t;
	using cookies_t = request_arg_t::cookies_t;

public:
	generator(version_enum version, url_t url, request_arg_t arg);
	generator(url_t url, request_arg_t arg);
	~generator();

	generator(generator &&other) noexcept;
	generator &operator=(generator &&other) noexcept;

public:
	generator &set_url(url_t url);
	generator &set_arg(request_arg_t arg);
	generator &set(url_t url, request_arg_t arg);

	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] url_t &url() noexcept;

	[[nodiscard]] const request_arg_t &arg() const noexcept;
	[[nodiscard]] request_arg_t &arg() noexcept;

public:
	template <method_enum Method>
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string header_data(method_enum method, size_t body_size = 0);

	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

public:
	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] generator_state pro_state() const noexcept;

	[[nodiscard]] next_layer_t next_layer() noexcept;
	generator &reset() noexcept;

private:
	class impl;
	impl *m_impl;
};

using client_generator = generator<model::client>;

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/client/detail/generator.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H