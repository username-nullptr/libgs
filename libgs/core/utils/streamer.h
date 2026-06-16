
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_CXX_STREAMER_H
#define LIBGS_CORE_CXX_STREAMER_H

#include <libgs/core/cxx/type_traits.h>

namespace libgs
{

template <typename>
struct streamer {};

template <typename T>
struct decoder_data
{
	T data {};
	size_t size = 0;

	const T &operator*() const noexcept { return data; }
	T &operator*() noexcept { return data; }

	const T *operator->() const noexcept { return &data; }
	T *operator->() noexcept { return &data; }

	operator const T&() const noexcept { return data; }
	operator T&() noexcept { return data; }
};

#define LIBGS_SERIALIZE_FIELDS(...) \
	auto meta_fields() { return std::tie(__VA_ARGS__); } \
	auto meta_fields() const { return std::tie(__VA_ARGS__); }

#define LIBGS_META_FIELDS(...) \
	LIBGS_FIELD_MAP(LIBGS_FIELD_DECL, __VA_ARGS__) \
	LIBGS_SERIALIZE_FIELDS(LIBGS_FIELD_MAP_COMMA(LIBGS_FIELD_NAME,__VA_ARGS__))

} //namespace libgs
#include <libgs/core/utils/detail/streamer.h>
#include <libgs/core/utils/detail/streamer_container.h>
#include <libgs/core/utils/detail/streamer_stateful.h>
#include <libgs/core/utils/detail/streamer_chrono.h>
#include <libgs/core/utils/detail/streamer_custom.h>


#endif //LIBGS_CORE_CXX_STREAMER_H