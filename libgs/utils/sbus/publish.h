
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

#ifndef LIBGS_UTILS_UTILS_SBUS_PUBLISH_H
#define LIBGS_UTILS_UTILS_SBUS_PUBLISH_H

#include <libgs/utils/sbus/interface.h>

namespace libgs::utils::sbus
{

namespace concepts
{

template <typename T>
concept unregistered_type =
	not std::is_pointer_v<std::remove_cvref_t<T>> and
	not libgs::concepts::any_string<T> and
	not topic_type<T>;

template <typename T>
concept unregistered_type_p = unregistered_type<std::remove_cvref_t<T>>;

} //namespace concepts

template <concepts::interface Interface>
LIBGS_UTILS_TAPI void publish (
	std::string_view topic, const void *buffer, size_t size
);

template <concepts::interface Interface, libgs::concepts::any_string_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::interface Interface, concepts::unregistered_type_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::interface Interface, concepts::topic_type...Args>
LIBGS_UTILS_TAPI void publish(Args&&...args)
	requires (sizeof...(Args) > 0);

LIBGS_UTILS_API void publish (
	std::string_view topic, const void *buffer, size_t size
);

template <libgs::concepts::any_string_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::unregistered_type_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::topic_type...Args>
LIBGS_UTILS_TAPI void publish(Args&&...args)
	requires (sizeof...(Args) > 0);

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/publish.h>


#endif //LIBGS_UTILS_UTILS_SBUS_PUBLISH_H