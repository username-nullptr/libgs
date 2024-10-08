
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

#ifndef LIBGS_HTTP_SERVER_AOP_H
#define LIBGS_HTTP_SERVER_AOP_H

#include <libgs/http/server/context.h>

namespace libgs::http
{

template <concepts::stream_requires Stream, core_concepts::char_type CharT>
class basic_aop
{
	LIBGS_DISABLE_COPY_MOVE(basic_aop)

public:
	using context_t = basic_service_context<Stream,CharT>;
	basic_aop() = default;
	virtual ~basic_aop() = 0;

public:
	[[nodiscard]] virtual awaitable<bool> before(context_t &context);
	[[nodiscard]] virtual awaitable<bool> after(context_t &context);
	[[nodiscard]] virtual bool exception(context_t &context, std::exception &ex);
};

template <core_concepts::execution Exec>
using basic_tcp_aop = basic_aop<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_aop = basic_aop<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_aop = basic_tcp_aop<asio::any_io_executor>;
using wtcp_aop = wbasic_tcp_aop<asio::any_io_executor>;

template <concepts::stream_requires Stream, core_concepts::char_type CharT>
using basic_aop_ptr = std::shared_ptr<basic_aop<Stream,CharT>>;

template <core_concepts::execution Exec>
using basic_tcp_aop_ptr = basic_aop_ptr<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_aop_ptr = basic_aop_ptr<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_aop_ptr = basic_tcp_aop_ptr<asio::any_io_executor>;
using wtcp_aop_ptr = wbasic_tcp_aop_ptr<asio::any_io_executor>;

template <concepts::stream_requires Stream, core_concepts::char_type CharT>
class basic_ctrlr_aop : public basic_aop<Stream,CharT>
{
public:
	using context_t = basic_service_context<Stream,CharT>;
	[[nodiscard]] virtual awaitable<void> service(context_t &context) = 0;
};

template <core_concepts::execution Exec>
using basic_tcp_ctrlr_aop = basic_ctrlr_aop<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_ctrlr_aop = basic_ctrlr_aop<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_ctrlr_aop = basic_tcp_ctrlr_aop<asio::any_io_executor>;
using wtcp_ctrlr_aop = wbasic_tcp_ctrlr_aop<asio::any_io_executor>;

template <concepts::stream_requires Stream, core_concepts::char_type CharT>
using basic_ctrlr_aop_ptr = std::shared_ptr<basic_ctrlr_aop<Stream,CharT>>;

template <core_concepts::execution Exec>
using basic_tcp_ctrlr_aop_ptr = basic_ctrlr_aop_ptr<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_ctrlr_aop_ptr = basic_ctrlr_aop_ptr<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_ctrlr_aop_ptr = basic_tcp_ctrlr_aop_ptr<asio::any_io_executor>;
using wtcp_ctrlr_aop_ptr = wbasic_tcp_ctrlr_aop_ptr<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/aop.h>


#endif //LIBGS_HTTP_SERVER_AOP_H
