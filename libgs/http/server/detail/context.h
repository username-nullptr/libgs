
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_CONTEXT_H
#define LIBGS_HTTP_SERVER_DETAIL_CONTEXT_H

namespace libgs::http
{

template <concepts::stream Stream, core_concepts::char_type CharT>
class basic_service_context<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl(stream_t &&stream, parser_t &parser, session_set &sss) :
		m_response(request_t(std::move(stream), parser)), m_sss(&sss) {}

	template<typename Stream0>
	impl(typename basic_service_context<Stream0,char_t>::impl &&other) noexcept :
		m_response(std::move(other)), m_sss(other.m_sss) {}

	impl(impl &&other) noexcept :
		m_response(std::move(other)), m_sss(other.m_sss) {}

	template<typename Stream0>
	impl &operator=(typename basic_service_context<Stream0,char_t>::impl &&other) noexcept
	{
		m_response = std::move(other);
		m_sss = other.m_sss;
		return *this;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_response = std::move(other);
		m_sss = other.m_sss;
		return *this;
	}

public:
	response_t m_response;
	session_set *m_sss;
};

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_service_context<Stream,CharT>::basic_service_context(stream_t &&stream, parser_t &parser, session_set &sss) :
	m_impl(new impl(std::move(stream), parser, sss))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_service_context<Stream,CharT>::~basic_service_context()
{
	delete m_impl;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_service_context<Stream,CharT>::basic_service_context(basic_service_context &&other) noexcept :
	m_impl(new impl(*other.m_impl))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_service_context<Stream,CharT> &basic_service_context<Stream,CharT>::operator=
(basic_service_context &&other) noexcept
{
	*m_impl = *other.m_impl;
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template<typename Stream0>
basic_service_context<Stream,CharT>::basic_service_context(basic_service_context<Stream0,char_t> &&other) noexcept
	requires core_concepts::constructible<Stream,Stream0&&> :
	m_impl(new impl(*other.m_impl))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
template<typename Stream0>
basic_service_context<Stream,CharT> &basic_service_context<Stream,CharT>::operator=
(basic_service_context<Stream0,CharT> &&other) noexcept requires core_concepts::assignable<Stream,Stream0&&>
{
	*m_impl = *other.m_impl;
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const basic_server_request<Stream,CharT> &basic_service_context<Stream,CharT>::request() const noexcept
{
	return m_impl->m_response.next_layer();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_request<Stream,CharT> &basic_service_context<Stream,CharT>::request() noexcept
{
	return m_impl->m_response.next_layer();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const basic_server_response<Stream,CharT> &basic_service_context<Stream,CharT>::response() const noexcept
{
	return m_impl->m_response;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_response<Stream,CharT> &basic_service_context<Stream,CharT>::response() noexcept
{
	return m_impl->m_response;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_service_context<Stream,CharT>::executor_t
basic_service_context<Stream,CharT>::get_executor() noexcept
{
	return request().get_executor();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_service_context<Stream,CharT>::session(Args&&...args)
	requires core_concepts::constructible<Session, Args...>
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->template get_or_make<Session>(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_service_context<Stream,CharT>::session(Args&&...args) noexcept
	requires core_concepts::constructible<basic_session<char_t>, Args...>
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->get_or_make(session_id, std::forward<Args>(args)...);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <base_of_session<CharT> Session>
std::shared_ptr<Session> basic_service_context<Stream,CharT>::session() const
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->template get<Session>(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_session_ptr<CharT> basic_service_context<Stream,CharT>::session() const
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->get(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <base_of_session<CharT> Session>
std::shared_ptr<Session> basic_service_context<Stream,CharT>::session_or()
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->template get_or<Session>(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_session_ptr<CharT> basic_service_context<Stream,CharT>::session_or() noexcept
{
	auto session_cookie = m_impl->m_sss->cookie_key();
	auto session_id = request().cookie_or(session_cookie).to_string();
	auto session = m_impl->m_sss->get_or(session_id);
	response().set_cookie(session_cookie, {session->id()});
	return session;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_CONTEXT_H
