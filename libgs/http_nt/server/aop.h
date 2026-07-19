
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_SERVER_AOP_H
#define LIBGS_HTTP_NT_SERVER_AOP_H

#include <libgs/http_nt/server/service_context.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class basic_aop
{
	LIBGS_DISABLE_COPY_MOVE(basic_aop)

public:
	using connection_t = Connection;
	using context_t = basic_service_context<connection_t>;

	basic_aop() = default;
	virtual ~basic_aop() = 0;

public:
	[[nodiscard]] virtual awaitable<bool> before(context_t &context);
	[[nodiscard]] virtual awaitable<bool> after(context_t &context);
	[[nodiscard]] virtual bool exception(context_t &context, const std::exception &ex);
};

} //namespace libgs::http_nt
#include <libgs/http_nt/server/detail/aop.h>


#endif //LIBGS_HTTP_NT_SERVER_AOP_H