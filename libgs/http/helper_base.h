
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

#ifndef LIBGS_HTTP_HELPER_BASE_H
#define LIBGS_HTTP_HELPER_BASE_H

#include <libgs/http/types.h>

namespace libgs::http
{

enum class helper_state {
	header, content_length, chunk, finish
};

class LIBGS_HTTP_API helper_base
{
	LIBGS_DISABLE_COPY_MOVE(helper_base)

public:
	using state_t = helper_state;
	using headers_t = http::headers;
	using value_t = libgs::value;

	helper_base();
	virtual ~helper_base() = 0;

public:
	helper_base &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	helper_base &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	helper_base &set_chunk_attribute(value_t attr) noexcept;
	helper_base &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

public:
	[[nodiscard]] virtual std::string header_data(size_t body_size);
	[[nodiscard]] virtual std::string body_data(const const_buffer &buffer);
	[[nodiscard]] virtual std::string chunk_end_data(const headers_t &headers);

	[[nodiscard]] std::string header_data();
	[[nodiscard]] std::string chunk_end_data();

public:
	[[nodiscard]] virtual version_enum version() const noexcept = 0;
	[[nodiscard]] state_t state() const noexcept;
	helper_base &reset();

protected:
	class impl;
	impl *m_impl;
};

class LIBGS_HTTP_API helper_base_v10 final : public helper_base
{
	LIBGS_DISABLE_COPY_MOVE(helper_base_v10)

public:
	using helper_base::helper_base;
	[[nodiscard]] version_enum version() const noexcept override;
};

class LIBGS_HTTP_API helper_base_v11 final : public helper_base
{
	LIBGS_DISABLE_COPY_MOVE(helper_base_v11)

public:
	using helper_base::helper_base;
	[[nodiscard]] std::string header_data(size_t body_size) override;
	[[nodiscard]] version_enum version() const noexcept override;
};

// ... ...
// class LIBGS_HTTP_API helper_base_v12 final : public helper_base
// class LIBGS_HTTP_API helper_base_v20 final : public helper_base

} //namespace libgs::http
#include <libgs/http/detail/helper_base.h>


#endif //LIBGS_HTTP_HELPER_BASE_H