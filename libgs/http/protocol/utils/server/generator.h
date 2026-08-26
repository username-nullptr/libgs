
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#include <libgs/http/protocol/utils/core/container_helper.h>
#include <libgs/http/protocol/utils/core/generator_types.h>

namespace libgs::http
{

template <>
class LIBGS_HTTP_API generator<protocol_model::server> final :
	public mutable_headers<generator<protocol_model::server>>,
	public mutable_cookies<cookie,generator<protocol_model::server>>,
	public mutable_chunk_attributes<generator<protocol_model::server>>
{
	LIBGS_DISABLE_COPY(generator)

public:
	using version_t = http::version;

	explicit generator(version_enum version = version_t::v11);
	~generator() override;

	generator(generator &&other) noexcept;
	generator &operator=(generator &&other) noexcept;

public:
	generator &set_status(status_enum status);
	[[nodiscard]] status_enum status() const noexcept;

	generator &set_redirect (
		core_concepts::text_p<char> auto &&url,
		redirect_enum type = redirect::moved_permanently
	);

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string header_data(size_t body_size, method_enum request_method);

	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] generator_state pro_state() const noexcept;
	generator &reset() noexcept;

private:
	class impl;
	impl *m_impl;
};

using server_generator = generator<protocol_model::server>;

} //namespace libgs::http
#include <libgs/http/protocol/utils/server/detail/generator.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_SERVER_GENERATOR_H
