
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

template <core_concepts::char_type CharT, version_t Version = version::v11>
class LIBGS_HTTP_TAPI basic_helper_base final
{
	LIBGS_DISABLE_COPY(basic_helper_base)

public:
	using char_t = CharT;
	constexpr static version_t version_v = Version;

	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = basic_value<char_t>;
	using value_set_t = basic_value_set<char_t>;

	using header_t = basic_header<char_t>;
	using headers_t = basic_headers<char_t>;

	using attr_init_t = basic_attr_init<char_t>;
	using pair_init_t = basic_key_attr_init<char_t>;
	using key_init_t = basic_key_init<char_t>;
	using map_helper_t = basic_attr_map_helper<char_t>;

	enum class state_t {
		header, content_length, chunk, finish
	};

public:
	basic_helper_base();
	~basic_helper_base();

	basic_helper_base(basic_helper_base &&other) noexcept;
	basic_helper_base &operator=(basic_helper_base &&other) noexcept;

public:
	template <typename...Args>
	basic_helper_base &set_header(Args&&...args) noexcept requires
		concepts::set_key_attr_params<char_t,Args...>;

	basic_helper_base &set_header(pair_init_t headers) noexcept;

	template <typename...Args>
	basic_helper_base &set_chunk_attribute(Args&&...args) noexcept requires
		concepts::set_attr_params<char_t,Args...>;

	basic_helper_base &set_chunk_attribute(attr_init_t attributes) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const value_set_t &chunk_attributes() const noexcept;
	[[nodiscard]] consteval version_t version() const noexcept;
	[[nodiscard]] state_t state() const noexcept;

public:
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const map_helper_t &headers = {});

public:
	template <typename...Args>
	basic_helper_base &unset_header(Args&&...args) requires
		concepts::unset_pair_params<char_t,Args...>;

	basic_helper_base &unset_header(key_init_t headers) noexcept;
	basic_helper_base &clear_headers() noexcept;

	template <typename...Args>
	basic_helper_base &unset_chunk_attribute(Args&&...args) requires
		concepts::unset_attr_params<char_t,Args...>;

	basic_helper_base &unset_chunk_attribute(attr_init_t attributes) noexcept;
	basic_helper_base &clear_chunk_attributes() noexcept;

	basic_helper_base &reset();

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/detail/helper_base.h>


#endif //LIBGS_HTTP_HELPER_BASE_H