
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H

#include <libgs/http_nt/protocol/utils/core/container_helper.h>

namespace libgs::http_nt
{

class LIBGS_HTTP_NT_API request_arg final :
	public mutable_headers<request_arg>,
	public mutable_cookies<value,request_arg>,
	public mutable_chunk_attributes<request_arg>
{
public:
	request_arg();
	~request_arg();

	request_arg(const request_arg &other) noexcept;
	request_arg &operator=(const request_arg &other) noexcept;

	request_arg(request_arg &&other) noexcept;
	request_arg &operator=(request_arg &&other) noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http_nt/protocol/utils/client/detail/request_arg.h>


#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_REQUEST_ARG_H