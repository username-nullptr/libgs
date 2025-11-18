
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_UTILS_UTILITIES_H
#define LIBGS_CORE_UTILS_UTILITIES_H

#include <libgs/core/utils/asio_concepts.h>
#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/attributes.h>

namespace libgs
{

enum class ip_type {
	v4, v6, loopback
};

template <typename Protocol>
struct LIBGS_CORE_TAPI basic_endpoint_wrapper
{
	using protocol_t = Protocol;
	using endpoint_t = asio::ip::basic_endpoint<protocol_t>;
	endpoint_t value;

	basic_endpoint_wrapper() = default;
	basic_endpoint_wrapper(const concepts::any_string_p auto &address, uint16_t port);
	basic_endpoint_wrapper(const concepts::any_string_p auto &address);

	basic_endpoint_wrapper(ip_type type, uint16_t port);
	basic_endpoint_wrapper(ip_type type);

	template <typename...Args>
	basic_endpoint_wrapper(Args&&...args) requires
		concepts::constructible<endpoint_t,Args&&...>;

	operator endpoint_t&();
	operator const endpoint_t&() const;

	endpoint_t &operator*();
	endpoint_t *operator->();
};

using tcp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::tcp>;
using udp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::udp>;

} //namespace libgs
#include <libgs/core/utils/detail/utils.h>


#endif //LIBGS_CORE_UTILS_UTILITIES_H
