
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

#ifndef LIBGS_HTTP_BASIC_HEADER_H
#define LIBGS_HTTP_BASIC_HEADER_H

#include <libgs/http/basic/container.h>

namespace libgs::http
{

#define LIBGS_HTTP_HEADER_KEY(_type, ...) \
	constexpr const _type *accept_language   = __VA_ARGS__##"Accept-Language"; \
	constexpr const _type *accept_encoding   = __VA_ARGS__##"Accept-Encoding"; \
	constexpr const _type *accept_ranges     = __VA_ARGS__##"Accept-Ranges"; \
	constexpr const _type *accept            = __VA_ARGS__##"Accept"; \
	constexpr const _type *age               = __VA_ARGS__##"Age"; \
	constexpr const _type *content_encoding  = __VA_ARGS__##"Content-Encoding"; \
	constexpr const _type *content_length    = __VA_ARGS__##"Content-Length"; \
	constexpr const _type *cache_control     = __VA_ARGS__##"Cache-Control"; \
	constexpr const _type *content_range     = __VA_ARGS__##"Content-Range"; \
	constexpr const _type *content_type      = __VA_ARGS__##"Content-Type"; \
	constexpr const _type *connection        = __VA_ARGS__##"Connection"; \
	constexpr const _type *expires           = __VA_ARGS__##"Expires"; \
	constexpr const _type *host              = __VA_ARGS__##"Host"; \
	constexpr const _type *last_modified     = __VA_ARGS__##"Last-Modified"; \
	constexpr const _type *location          = __VA_ARGS__##"Location"; \
	constexpr const _type *origin            = __VA_ARGS__##"Origin"; \
	constexpr const _type *referer           = __VA_ARGS__##"Referer"; \
	constexpr const _type *range             = __VA_ARGS__##"Range"; \
	constexpr const _type *transfer_encoding = __VA_ARGS__##"Transfer-Encoding"; \
	constexpr const _type *user_agent        = __VA_ARGS__##"User-Agent"; \
	constexpr const _type *upgrade           = __VA_ARGS__##"Upgrade"

namespace header { LIBGS_HTTP_HEADER_KEY(char); };
namespace wheader { LIBGS_HTTP_HEADER_KEY(wchar_t,L); };

template <concept_char_type CharT>
using basic_headers = std::map<std::basic_string<CharT>, basic_value<CharT>, less_case_insensitive>;

using headers = basic_headers<char>;
using wheaders = basic_headers<wchar_t>;

template <concept_char_type CharT>
using basic_headers_view = decltype (
	std::declval<basic_headers<CharT>>() |
	std::views::take (
		std::declval<basic_headers<CharT>>().size()
	)
);

using headers_view = basic_headers_view<char>;
using wheaders_view = basic_headers_view<wchar_t>;

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_HEADER_H