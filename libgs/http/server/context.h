
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

#ifndef LIBGS_HTTP_SERVER_CONTEXT_H
#define LIBGS_HTTP_SERVER_CONTEXT_H

#include <libgs/http/server/response.h>
#include <libgs/http/server/session.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class basic_service_context
{
	LIBGS_DISABLE_COPY_MOVE(basic_service_context)

public:
	using socket_ptr = io::basic_socket_ptr<Exec>;
	using parser_type = basic_request_parser<CharT>;

	using request_type = basic_server_request<CharT,Exec>;
	using response_type = basic_server_response<CharT,Exec>;

	using session_type = basic_session<CharT>;
	using session_ptr = basic_session_ptr<CharT>;

public:
	basic_service_context(socket_ptr socket, parser_type &parser);
	~basic_service_context();

public:
	const request_type &request() const noexcept;
	request_type &request() noexcept;

	const response_type &response() const noexcept;
	response_type &response() noexcept;

public:
	template <detail::base_of_session<CharT> Session, typename...Args>
	std::shared_ptr<Session> session(Args&&...args)
		requires detail::can_construct<Session, Args...>;

	template <typename...Args>
	session_ptr session(Args&&...args) noexcept
		requires detail::can_construct<basic_session<CharT>, Args...>;

	template <detail::base_of_session<CharT> Session, typename...Args>
	std::shared_ptr<Session> session() const;

	template <typename...Args>
	session_ptr session() const;

	template <detail::base_of_session<CharT> Session, typename...Args>
	std::shared_ptr<Session> session_or();

	template <typename...Args>
	session_ptr session_or() noexcept;

private:
	class impl;
	impl *m_impl;
};

using service_context = basic_service_context<char>;
using service_wcontext = basic_service_context<wchar_t>;




template <concept_char_type CharT, concept_execution Exec>
class basic_service_context<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl(socket_ptr socket, parser_type &parser) :
		m_request(new request_type(std::move(socket), parser)),
		m_response(m_request) {}

public:
	std::shared_ptr<request_type> m_request;
	response_type m_response;
};

template <concept_char_type CharT, concept_execution Exec>
basic_service_context<CharT,Exec>::basic_service_context(socket_ptr socket, parser_type &parser) :
	m_impl(new impl(std::move(socket), parser))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_service_context<CharT,Exec>::~basic_service_context()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
const basic_server_request<CharT,Exec> &basic_service_context<CharT,Exec>::request() const noexcept
{
	return *m_impl->m_request;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request<CharT,Exec> &basic_service_context<CharT,Exec>::request() noexcept
{
	return *m_impl->m_request;
}

template <concept_char_type CharT, concept_execution Exec>
const basic_server_response<CharT,Exec> &basic_service_context<CharT,Exec>::response() const noexcept
{
	return m_impl->m_response;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_service_context<CharT,Exec>::response() noexcept
{
	return m_impl->m_response;
}

template <concept_char_type CharT, concept_execution Exec>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_service_context<CharT,Exec>::session(Args&&...args)
	requires detail::can_construct<Session, Args...>
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get_or_make<Session>(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename...Args>
basic_session_ptr<CharT> basic_service_context<CharT,Exec>::session(Args&&...args) noexcept
	requires detail::can_construct<basic_session<CharT>, Args...>
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get_or_make(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concept_char_type CharT, concept_execution Exec>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_service_context<CharT,Exec>::session() const
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get<Session>(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename...Args>
basic_session_ptr<CharT> basic_service_context<CharT,Exec>::session() const
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concept_char_type CharT, concept_execution Exec>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_service_context<CharT,Exec>::session_or()
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get_or<Session>(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename...Args>
basic_session_ptr<CharT> basic_service_context<CharT,Exec>::session_or() noexcept
{
	auto session_cookie = session_type::cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = session_type::template get_or(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

} //namespace libgs::http
#include <libgs/http/server/detail/context.h>


#endif //LIBGS_HTTP_SERVER_CONTEXT_H

