
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
	impl(connection_ptr connection, session_manager &session_manager) :
		m_session_manager(session_manager), m_response(connection), m_request(connection) {
		m_response.auto_set(m_request);
	}

public:
	session_manager &m_session_manager;
	response_t m_response;
	request_t m_request;
};

template <concepts::connection Connection>
basic_service_context<Connection>::basic_service_context
(connection_ptr connection, session_manager &session_manager) :
	m_impl(new impl(std::move(connection), session_manager))
{

}

template <concepts::connection Connection>
basic_service_context<Connection>::~basic_service_context() = default;

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

template <concepts::connection Connection>
template <typename Session, typename...Args>
std::shared_ptr<Session> basic_service_context<Connection>::session(Args&&...args) requires
	core_concepts::base_of<Session,session_t> and core_concepts::constructible<Session, Args...>
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.template get_or_make<Session>(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

template <concepts::connection Connection>
template <typename...Args>
session_ptr basic_service_context<Connection>::session(Args&&...args) noexcept
	requires core_concepts::constructible<session_t, Args...>
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.get_or_make(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

template <concepts::connection Connection>
template <typename Session>
std::shared_ptr<Session> basic_service_context<Connection>::session()
	requires core_concepts::base_of<Session,session_t>
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.template get<Session>(session_id);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

template <concepts::connection Connection>
template <typename Session>
std::shared_ptr<Session> basic_service_context<Connection>::session_or()
	requires core_concepts::base_of<Session,session_t>
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.template get_or<Session>(session_id);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

template <concepts::connection Connection>
session_ptr basic_service_context<Connection>::session()
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.get(session_id);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

template <concepts::connection Connection>
session_ptr basic_service_context<Connection>::session_or() noexcept
{
	auto session_cookie = m_impl->m_session_manager.cookie_key();
	auto session_id = request().cookie(session_cookie).or_else()->to_string();
	auto session = m_impl->m_session_manager.get_or(session_id);
	response().set_cookie(session_cookie, cookie(session->id()));
	return session;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_SERVICE_CONTEXT_H
