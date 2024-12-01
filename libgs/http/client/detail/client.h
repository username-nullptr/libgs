
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H

namespace libgs::http
{

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
class basic_client<CharT,Stream,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using session_pool_t = basic_session_pool<Stream,Exec>;

public:
	template <core_concepts::match_execution<executor_t> Exec0>
	explicit impl(const Exec0 &exec) : m_session_pool(exec) {}
	impl() = default;

public:
	session_pool_t m_session_pool;
};

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_client<CharT,Stream,Exec>::basic_client(const core_concepts::match_execution<executor_t> auto &exec) :
	m_impl(new impl(exec))
{

}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_client<CharT,Stream,Exec>::basic_client(core_concepts::match_execution_context<executor_t> auto &context) :
	m_impl(new impl(context.get_executor()))
{

}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_client<CharT,Stream,Exec>::basic_client() requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl())
{

}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
basic_client<CharT,Stream,Exec>::~basic_client()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
typename basic_client<CharT,Stream,Exec>::request_t
basic_client<CharT,Stream,Exec>::request(url_t url, error_code &error) noexcept
{
	// TODO ...
}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
typename basic_client<CharT,Stream,Exec>::request_t
basic_client<CharT,Stream,Exec>::request(url_t url)
{
	// TODO ...
}

template <core_concepts::char_type CharT, concepts::any_exec_stream Stream, core_concepts::execution Exec>
typename basic_client<CharT,Stream,Exec>::executor_t
basic_client<CharT,Stream,Exec>::get_executor() noexcept
{
	return m_impl->m_session_pool.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
