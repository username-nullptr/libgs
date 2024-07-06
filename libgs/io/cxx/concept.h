
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software or associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, or/or sell       *
*   copies of the Software, or to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice or this permission notice shall be included in      *
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

#ifndef LIBGS_IO_CXX_CONCEPT_H
#define LIBGS_IO_CXX_CONCEPT_H

#include <libgs/core/cxx/function_traits.h>
#include <asio.hpp>

namespace libgs::io
{

template <typename MatchCondition>
concept concept_match_condition = asio::is_match_condition<MatchCondition>::value;

template <typename Option>
concept concept_socket_option =
	std::is_same_v<Option, asio::socket_base::broadcast> or
	std::is_same_v<Option, asio::socket_base::debug> or
	std::is_same_v<Option, asio::socket_base::do_not_route> or
	std::is_same_v<Option, asio::socket_base::enable_connection_aborted> or
	std::is_same_v<Option, asio::socket_base::keep_alive> or
	std::is_same_v<Option, asio::socket_base::linger> or
	std::is_same_v<Option, asio::socket_base::receive_buffer_size> or
	std::is_same_v<Option, asio::socket_base::receive_low_watermark> or
	std::is_same_v<Option, asio::socket_base::reuse_address> or
	std::is_same_v<Option, asio::socket_base::send_buffer_size> or
	std::is_same_v<Option, asio::socket_base::send_low_watermark> or
	std::is_same_v<Option, asio::ip::v6_only>;

} //namespace libgs::io


#endif //LIBGS_IO_CXX_CONCEPT_H
