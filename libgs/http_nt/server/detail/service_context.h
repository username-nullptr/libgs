
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_SERVICE_CONTEXT_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_SERVICE_CONTEXT_H

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_service_context<Connection>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl(connection_ptr connection, session_manager &ss_mgr) :
		m_ss_mgr(ss_mgr), m_response(connection), m_request(connection)
	{
		m_response.auto_set(m_request);
	}

public:
	session_manager &m_ss_mgr;
	response_t m_response;
	request_t m_request;
};

template <concepts::connection Connection>
basic_service_context<Connection>::basic_service_context
(connection_ptr connection, session_manager &ss_mgr) :
	m_impl(new impl(std::move(connection), ss_mgr))
{

}

template <concepts::connection Connection>
basic_service_context<Connection>::~basic_service_context()
{

}

template <concepts::connection Connection>
const basic_service_context<Connection>::request_t&
basic_service_context<Connection>::request() const noexcept
{
	return m_impl->m_request;
}

template <concepts::connection Connection>
basic_service_context<Connection>::request_t&
basic_service_context<Connection>::request() noexcept
{
	return m_impl->m_request;
}

template <concepts::connection Connection>
const basic_service_context<Connection>::response_t&
basic_service_context<Connection>::response() const noexcept
{
	return m_impl->m_response;
}

template <concepts::connection Connection>
basic_service_context<Connection>::response_t&
basic_service_context<Connection>::response() noexcept
{
	return m_impl->m_response;
}

template <concepts::connection Connection>
basic_service_context<Connection>::executor_t
basic_service_context<Connection>::get_executor() noexcept
{
	return request().get_executor();
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_SERVICE_CONTEXT_H
