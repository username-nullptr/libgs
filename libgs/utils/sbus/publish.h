
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

template <concepts::interface Interface>
LIBGS_UTILS_TAPI void publish (
	std::string_view topic, const void *buffer, size_t size
);

template <concepts::interface Interface>
LIBGS_UTILS_TAPI void publish (
	std::string_view topic, const char *str
);

template <concepts::interface Interface, typename T>
LIBGS_UTILS_TAPI void publish(std::string_view topic, T &&value)
	requires (not std::is_pointer_v<std::remove_cvref_t<T>>);

template <concepts::interface Interface>
LIBGS_UTILS_TAPI void publish (
	concepts::topic_type auto &&value
);

LIBGS_UTILS_API void publish (
	std::string_view topic, const void *buffer, size_t size
);

LIBGS_UTILS_API void publish (
	std::string_view topic, const char *str
);

template <typename T>
LIBGS_UTILS_TAPI void publish(std::string_view topic, T &&value) requires (
	not std::is_pointer_v<std::remove_cvref_t<T>> and not concepts::topic_type<T>
);

LIBGS_UTILS_TAPI void publish (
	concepts::topic_type auto &&value
);

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/publish.h>


#endif //LIBGS_UTILS_UTILS_SBUS_PUBLISH_H