
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

#ifndef LIBGS_HTTP_DETAIL_OPT_TOKEN_H
#define LIBGS_HTTP_DETAIL_OPT_TOKEN_H

namespace libgs::http
{

op_base::op_base(error_code &error)
{

}

inline constexpr req_range::req_range()
{

}

inline req_range::req_range(size_t total) :
	total(total)
{

}

inline req_range::req_range(size_t begin, size_t total) :
	begin(begin), total(total)
{

}

inline constexpr resp_range::resp_range()
{

}

inline resp_range::resp_range(size_t end) :
	end(end)
{

}

inline resp_range::resp_range(size_t begin, size_t end) :
	begin(begin), end(end)
{

}

namespace detail::concepts
{

template <typename T>
concept has_ec_ = requires(T &&t) {
	t.ec_;
};

template <typename T>
concept has_get = requires(T &&t)
{
	t.get_cancellation_slot();
	t.get();
};

} //namespace detail::concepts

} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_OPT_TOKEN_H

