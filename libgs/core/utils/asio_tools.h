
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

#ifndef LIBGS_CORE_UTILS_ASIO_TOOLS_H
#define LIBGS_CORE_UTILS_ASIO_TOOLS_H

#include <libgs/core/utils/token_concepts.h>
#include <libgs/core/utils/asio_concepts.h>
#include <libgs/core/cxx/attributes.h>

namespace libgs
{

using mutable_buffer = asio::mutable_buffer;

class LIBGS_CORE_VAPI const_buffer : public asio::const_buffer
{
public:
	using asio::const_buffer::const_buffer;
	const_buffer &operator=(const const_buffer&) = default;
	const_buffer(const asio::const_buffer &buf);
	const_buffer(const mutable_buffer &buf);
	const_buffer(const char *buf);
	const_buffer(const std::string &buf);
	const_buffer(std::string_view buf);
	const_buffer &operator=(const mutable_buffer &buf);
};

template <typename...Args>
[[nodiscard]] LIBGS_CORE_TAPI auto buffer(Args&&...args);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) get_executor_helper (
	concepts::sched auto &&exec
);

[[nodiscard]] LIBGS_CORE_TAPI decltype(auto) unbound_token (
	concepts::any_tf_opt_token auto &&token
);

} //namespace libgs
#include <libgs/core/utils/detail/asio_tools.h>


#endif //LIBGS_CORE_UTILS_ASIO_TOOLS_H