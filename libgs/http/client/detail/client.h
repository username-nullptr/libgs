
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

#include <libgs/http/client/session_pool.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_stream_requires Stream>
class basic_client<CharT,Stream>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using session_pool_t = basic_session_pool<Stream>;

public:
	template <concept_schedulable Exec>
	impl(Exec &exec)
	{
		if constexpr( is_execution_context_v<Exec> )
			m_executor = exec.get_executor();
		else
			m_executor = exec;
	}

public:
	session_pool_t m_session_pool;
	executor_t m_executor;
};

template <concept_char_type CharT, concept_stream_requires Stream>
template <concept_schedulable Exec>
basic_client<CharT,Stream>::basic_client(Exec &exec) :
	m_impl(new impl(exec))
{

}

template <concept_char_type CharT, concept_stream_requires Stream>
basic_client<CharT,Stream>::~basic_client()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_stream_requires Stream>
typename basic_client<CharT,Stream>::request_t 
basic_client<CharT,Stream>::request(url_t url, error_code &error) noexcept
{
	// TODO ...
}

template <concept_char_type CharT, concept_stream_requires Stream>
typename basic_client<CharT,Stream>::request_t
basic_client<CharT,Stream>::request(url_t url)
{
	// TODO ...
}

template <concept_char_type CharT, concept_stream_requires Stream>
const typename  basic_client<CharT,Stream>::executor_t&
basic_client<CharT,Stream>::get_executor() noexcept
{
	return m_impl->m_executor;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
