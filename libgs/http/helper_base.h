
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

template <version_enum Version = version::v11>
class LIBGS_HTTP_TAPI basic_helper_base final
{
	LIBGS_DISABLE_COPY(basic_helper_base)

public:
	constexpr static version_enum version_v = Version;
	using state_t = helper_state;

	using headers_t = http::headers;
	using value_t = libgs::value;

public:
	basic_helper_base();
	~basic_helper_base();

	basic_helper_base(basic_helper_base &&other) noexcept;
	basic_helper_base &operator=(basic_helper_base &&other) noexcept;

public:
	basic_helper_base &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	basic_helper_base &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	basic_helper_base &set_chunk_attribute(value_t attr) noexcept;
	basic_helper_base &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &headers = {});

	[[nodiscard]] consteval version_enum version() const noexcept;
	[[nodiscard]] state_t state() const noexcept;
	basic_helper_base &reset();

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/detail/helper_base.h>


#endif //LIBGS_HTTP_HELPER_BASE_H