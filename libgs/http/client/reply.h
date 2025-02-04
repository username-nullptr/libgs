
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

#ifndef LIBGS_HTTP_CLIENT_REPLY_H
#define LIBGS_HTTP_CLIENT_REPLY_H

#include <libgs/http/client/request_arg.h>
#include <libgs/http/client/session_pool.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, concepts::socket_session Session = session_pool::session_t>
class LIBGS_HTTP_TAPI basic_client_reply
{
	LIBGS_DISABLE_COPY(basic_client_reply)

public:
	using char_t = CharT;
	using session_t = Session;
	using executor_t = typename session_t::executor_t;

	using request_arg_t = basic_request_arg<char_t>;
	using url_t = typename request_arg_t::url_t;

public:
	basic_client_reply(session_t session, request_arg_t request_arg = {});
	~basic_client_reply();

	basic_client_reply(basic_client_reply &&other) noexcept;
	basic_client_reply &operator=(basic_client_reply &&other) noexcept;

public:

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/client/detail/reply.h>


#endif //LIBGS_HTTP_CLIENT_REPLY_H