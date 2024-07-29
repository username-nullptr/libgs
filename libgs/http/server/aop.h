
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

template <typename Stream, concept_char_type CharT>
class basic_aop
{
public:
	using context_t = basic_service_context<Stream,CharT>;
	virtual ~basic_aop() = 0;
	virtual awaitable<bool> before(context_t &context);
	virtual awaitable<bool> after(context_t &context);
	virtual bool exception(context_t &context, std::exception &ex);
};

template <concept_execution Exec>
using basic_tcp_aop = basic_aop<asio::basic_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using basic_tcp_waop = basic_aop<asio::basic_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_aop = basic_tcp_aop<asio::any_io_executor>;
using tcp_waop = basic_tcp_waop<asio::any_io_executor>;

template <typename Stream, concept_char_type CharT>
using basic_aop_ptr = std::shared_ptr<basic_aop<Stream,CharT>>;

template <concept_execution Exec>
using basic_tcp_aop_ptr = basic_aop_ptr<asio::basic_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using basic_tcp_waop_ptr = basic_aop_ptr<asio::basic_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_aop_ptr = basic_tcp_aop_ptr<asio::any_io_executor>;
using tcp_waop_ptr = basic_tcp_waop_ptr<asio::any_io_executor>;

template <typename Stream, concept_char_type CharT>
class basic_ctrlr_aop : public basic_aop<Stream,CharT>
{
public:
	using context_t = basic_service_context<Stream,CharT>;
	virtual awaitable<void> service(context_t &context) = 0;
};

template <concept_execution Exec>
using basic_tcp_ctrlr_aop = basic_ctrlr_aop<asio::basic_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using basic_tcp_wctrlr_aop = basic_ctrlr_aop<asio::basic_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_ctrlr_aop = basic_tcp_ctrlr_aop<asio::any_io_executor>;
using tcp_wctrlr_aop = basic_tcp_wctrlr_aop<asio::any_io_executor>;

template <typename Stream, concept_char_type CharT>
using basic_ctrlr_aop_ptr = std::shared_ptr<basic_ctrlr_aop<Stream,CharT>>;

template <concept_execution Exec>
using basic_tcp_ctrlr_aop_ptr = basic_ctrlr_aop_ptr<asio::basic_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using basic_tcp_wctrlr_aop_ptr = basic_ctrlr_aop_ptr<asio::basic_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_ctrlr_aop_ptr = basic_tcp_ctrlr_aop_ptr<asio::any_io_executor>;
using tcp_wctrlr_aop_ptr = basic_tcp_wctrlr_aop_ptr<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/aop.h>


#endif //LIBGS_HTTP_SERVER_AOP_H
