
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

#ifndef LIBGS_HTTP_TOOLS_CORE_GENERATOR_H
#define LIBGS_HTTP_TOOLS_CORE_GENERATOR_H

#include <libgs/http/protocol/types.h>

namespace libgs::http::protocol
{

template <model> class generator {};

enum class generator_state {
	header, content_length, chunk, finish
};

template <>
class LIBGS_HTTP_API generator<model::base>
{
	LIBGS_DISABLE_COPY_MOVE(generator)

public:
	using state_t = generator_state;
	using headers_t = protocol::headers;
	using value_t = libgs::value;

	generator();
	virtual ~generator() = 0;

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
	generator &set_chunk_attribute(value_t attr) noexcept;
	generator &unset_chunk_attribute(const value_t &attr) noexcept;

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
	generator &reset();

protected:
	class impl;
	impl *m_impl;
};

template <model> class generator_v10 {};
template <model> class generator_v11 {};

template <>
class LIBGS_HTTP_API generator_v10<model::base> final : public generator<model::base>
{
	LIBGS_DISABLE_COPY_MOVE(generator_v10)

public:
	using generator::generator;
	[[nodiscard]] version_enum version() const noexcept override;
};

template <>
class LIBGS_HTTP_API generator_v11<model::base> final : public generator<model::base>
{
	LIBGS_DISABLE_COPY_MOVE(generator_v11)

public:
	using generator::generator;
	[[nodiscard]] std::string header_data(size_t body_size) override;
	[[nodiscard]] version_enum version() const noexcept override;
};

// ... ...
// class LIBGS_HTTP_API generator_v12 final : public generator
// class LIBGS_HTTP_API generator_v20 final : public generator

using base_generator = generator<model::base>;
using base_generator_v10 = generator_v10<model::base>;
using base_generator_v11 = generator_v11<model::base>;

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/core/detail/generator.h>


#endif //LIBGS_HTTP_TOOLS_CORE_GENERATOR_H