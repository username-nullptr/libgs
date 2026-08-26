
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

#ifndef LIBGS_HTTP_SERVER_AOP_H
#define LIBGS_HTTP_SERVER_AOP_H

#include <libgs/http/server/service_context.h>

namespace libgs::http
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

template <core_concepts::exec Exec>
using basic_tcp_aop = basic_aop<basic_tcp_connection<Exec>>;

using tcp_aop = basic_tcp_aop<asio::any_io_executor>;

template <concepts::connection Connection>
using basic_aop_ptr = std::shared_ptr<basic_aop<Connection>>;

template <core_concepts::exec Exec>
using basic_tcp_aop_ptr = basic_aop_ptr<basic_tcp_connection<Exec>>;

using tcp_aop_ptr = basic_tcp_aop_ptr<asio::any_io_executor>;

template <concepts::connection Connection>
class basic_ctrlr_aop : public basic_aop<Connection>
{
public:
	using context_t = basic_service_context<Connection>;
	[[nodiscard]] virtual awaitable<void> service(context_t &context) = 0;
};

template <core_concepts::exec Exec>
using basic_tcp_ctrlr_aop = basic_ctrlr_aop<basic_tcp_connection<Exec>>;

using tcp_ctrlr_aop = basic_tcp_ctrlr_aop<asio::any_io_executor>;

template <concepts::connection Connection>
using basic_ctrlr_aop_ptr = std::shared_ptr<basic_ctrlr_aop<Connection>>;

template <core_concepts::exec Exec>
using basic_tcp_ctrlr_aop_ptr = basic_ctrlr_aop_ptr<basic_tcp_connection<Exec>>;

using tcp_ctrlr_aop_ptr = basic_tcp_ctrlr_aop_ptr<asio::any_io_executor>;

namespace concepts
{

template <typename Connection, typename...Args>
concept aop_ptr_list = requires(Args&&...args) {
	std::vector<basic_aop_ptr<Connection>> {
		basic_aop_ptr<Connection>(std::forward<Args>(args))...
	};
};

template <typename Connection, typename...Args>
concept ctrlr_aop_ptr_list = requires(Args&&...args) {
	std::vector<basic_ctrlr_aop_ptr<Connection>> {
		basic_ctrlr_aop_ptr<Connection>(std::forward<Args>(args))...
	};
};

template <typename Func, typename Connection>
concept request_handler = requires(Func &&func, basic_service_context<Connection> &context) {
	std::is_same_v<awaitable_ret_t<decltype(func(context))>,void>;
};

}} //namespace libgs::http
#include <libgs/http/server/detail/aop.h>


#endif //LIBGS_HTTP_SERVER_AOP_H
