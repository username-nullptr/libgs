
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

#ifndef LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H
#define LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H

#include <libgs/http/client/session_pool.h>
#include <libgs/http/client/request.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, method_t Method,
          concepts::session_pool SessionPool = session_pool>
class LIBGS_HTTP_TAPI basic_request_context
{
	LIBGS_DISABLE_COPY_MOVE(basic_request_context)

public:
	using char_t = CharT;
	static constexpr auto method = Method;

	using request_t = basic_client_request<char_t>;

public:
	basic_request_context();
	~basic_request_context();

//	template <concepts::token Token = use_sync_type>
//	auto write(request req, const const_buffer &buf, Token &&token = {});

//	template <concepts::token Token = use_sync_type>
//	auto write(request req, Token &&token = {});

//	template <concepts::token Token = use_sync_type>
//	auto write(const const_buffer &buf, Token &&token = {});

//	template <concepts::token Token = use_sync_type>
//	auto write(Token &&token = {});

// reconnect ?

//	template <concepts::token Token = use_sync_type>
//	auto wait_reply(Token &&token = {});

private:
	class impl;
	impl *m_impl;
};

template <method_t Method>
using request_context = basic_request_context<char,Method>;

using get_request_context     = request_context<method_t::GET    >;
using put_request_context     = request_context<method_t::PUT    >;
using post_request_context    = request_context<method_t::POST   >;
using head_request_context    = request_context<method_t::HEAD   >;
using patch_request_context   = request_context<method_t::PATCH  >;
using delete_request_context  = request_context<method_t::DELETE >;
using options_request_context = request_context<method_t::OPTIONS>;
using trace_request_context   = request_context<method_t::TRACE  >;
using connect_request_context = request_context<method_t::CONNECT>;

template <method_t Method>
using wrequest_context = basic_request_context<wchar_t,Method>;

using wget_request_context     = wrequest_context<method_t::GET    >;
using wput_request_context     = wrequest_context<method_t::PUT    >;
using wpost_request_context    = wrequest_context<method_t::POST   >;
using whead_request_context    = wrequest_context<method_t::HEAD   >;
using wpatch_request_context   = wrequest_context<method_t::PATCH  >;
using wdelete_request_context  = wrequest_context<method_t::DELETE >;
using woptions_request_context = wrequest_context<method_t::OPTIONS>;
using wtrace_request_context   = wrequest_context<method_t::TRACE  >;
using wconnect_request_context = wrequest_context<method_t::CONNECT>;

} //namespace libgs::http
#include <libgs/http/client/detail/request_context.h>


#endif //LIBGS_HTTP_CLIENT_REQUEST_CONTEXT_H