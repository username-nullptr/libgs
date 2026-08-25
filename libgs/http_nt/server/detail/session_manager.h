
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_SESSION_MANAGER_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_SESSION_MANAGER_H

#include <libgs/core/shared_mutex.h>

namespace libgs::http_nt
{

class LIBGS_HTTP_NT_VAPI session_manager::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;

	[[nodiscard]] session_ptr find (
		std::string_view id, bool _throw = true
	);
	[[nodiscard]] std::pair <
		std::map<std::string_view,session_ptr>::iterator, bool
	> emplace(session_ptr session);

	void erase(std::string_view id);

public:
	std::chrono::seconds m_lifecycle {60};
	std::string m_cookie_key = "session";

	std::map<std::string_view, session_ptr> m_session_map {};
	spin_shared_mutex m_map_mutex;

	std::function<void(session_ptr,error_code)> m_error_handle {};
};

template <typename Session, typename...Args>
std::shared_ptr<Session> session_manager::make(Args&&...args) requires
	core_concepts::base_of<Session,session> and core_concepts::constructible<Session,Args...>
{
	auto session = std::make_shared<Session>(
		std::forward<Args>(args)...
	);
	auto [it, inserted] = m_impl->emplace(session);
	if( not inserted )
	{
		runtime_error::loc_throw (
			"Session-id duplicated."
		);
	}
	auto id = session->id();

	session->on_timeout([this, id]{
		m_impl->erase(id);
	});
	session->set_lifecycle(lifecycle());

	session->on_error([this, id = std::move(id)](const error_code &error)
	{
		if( m_impl->m_error_handle )
			m_impl->m_error_handle(get(id), error);
	});
	return session;
}

template <typename...Args>
session_ptr session_manager::make(Args&&...args)
	requires core_concepts::constructible<session,Args...>
{
	return make<session>(std::forward<Args>(args)...);
}

template <typename Session, typename...Args>
std::shared_ptr<Session> session_manager::get_or_make
(const core_concepts::text_p<char> auto &id, Args&&...args) requires
	core_concepts::base_of<Session,session> and
	core_concepts::constructible<Session,Args...>
{
	auto id_view = strtls::to_view(id);
	auto ptr = m_impl->find(id_view, false);
	if( not ptr )
		return make<Session>(std::forward<Args>(args)...);

	auto rptr = std::dynamic_pointer_cast<Session>(ptr);
	if( not rptr )
	{
		throw runtime_error (
			"libgs::http::session_manager::get_or_make: type error [id = {}].", id_view
		);
	}
	return rptr;
}

template <typename...Args>
session_ptr session_manager::get_or_make
(const core_concepts::text_p<char> auto &id, Args&&...args)
	requires core_concepts::constructible<session,Args...>
{
	auto ptr = m_impl->find(strtls::to_view(id), false);
	if( not ptr )
		return make(std::forward<Args>(args)...);
	return ptr;
}

template <typename Session>
std::shared_ptr<Session> session_manager::get(const core_concepts::text_p<char> auto &id)
	requires core_concepts::base_of<Session,session>
{
	auto id_view = strtls::to_view(id);
	auto ptr = std::dynamic_pointer_cast<Session>(
		m_impl->find(id_view)
	);
	if( not ptr )
	{
		throw runtime_error(
			"libgs::http::session_manager::get: type error [id = {}].", id_view
		);
	}
	return ptr;
}

template <typename Session>
std::shared_ptr<Session> session_manager::get_or(const core_concepts::text_p<char> auto &id)
	requires core_concepts::base_of<Session,session>
{
	auto id_view = strtls::to_view(id);
	auto session = m_impl->find(id_view, false);
	if( not session )
		return {};

	auto ptr = std::dynamic_pointer_cast<Session>(session);
	if( not ptr )
	{
		throw runtime_error(
			"libgs::http::session_manager::get_or: type error [id = {}].", id_view
		);
	}
	return ptr;
}

session_ptr session_manager::get(const core_concepts::text_p<char> auto &id)
{
	return m_impl->find(strtls::to_view(id));
}

session_ptr session_manager::get_or(const core_concepts::text_p<char> auto &id) noexcept
{
	return m_impl->find(strtls::to_view(id), false);
}

template <typename Rep, typename Period>
session_manager &session_manager::set_lifecycle(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	using namespace std::chrono_literals;

	m_impl->m_lifecycle = sc::duration_cast<sc::seconds>(seconds);
	if( m_impl->m_lifecycle.count() == 0 )
		m_impl->m_lifecycle = 1s;
	return *this;
}

session_manager &session_manager::set_cookie_key(core_concepts::text_p<char> auto &&key)
{
	auto key_str = strtls::to_string(std::forward<decltype(key)>(key));
	if( key_str.empty() )
	{
		throw runtime_error (
			"libgs::http::session::set_cookie_key: key is empty."
		);
	}
	m_impl->m_cookie_key = std::move(key_str);
	return *this;
}

template <core_concepts::callable<session_ptr,error_code> Func>
session_manager &session_manager::on_error(Func &&func)
{
	m_impl->m_error_handle = std::forward<Func>(func);
	return *this;
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_SESSION_MANAGER_H
