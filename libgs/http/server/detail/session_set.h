
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SESSION_SET_H
#define LIBGS_HTTP_SERVER_DETAIL_SESSION_SET_H

#include <libgs/http/server/session.h>
#include <libgs/core/shared_mutex.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
class basic_session_set<CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using session_ptr = basic_session_ptr<char_t>;

public:
	impl()
	{
		if constexpr( is_char_v<char_t> )
			m_cookie_key = "session";
		else
			m_cookie_key = L"session";
	}

public:
	session_ptr find(string_view_t id, bool _throw = true)
	{
		spin_shared_shared_lock locker(m_map_mutex); LIBGS_UNUSED(locker);
		auto it = m_session_map.find(id);

		if( it == m_session_map.end() )
		{
			if( _throw )
				throw runtime_error("libgs::http::session_set: <map>: id '{}' not exists.", xxtombs<char_t>(id));
			return {};
		}
		it->second->expand();
		return it->second;
	}

	auto emplace(session_ptr session)
	{
		m_map_mutex.lock();
		auto pair = m_session_map.emplace(session->id(), std::move(session));
		m_map_mutex.unlock();
		return pair;
	}

	void erase(string_view_t id)
	{
		m_map_mutex.lock();
		m_session_map.erase(string_t(id.data(), id.size()));
		m_map_mutex.unlock();
	}

public:
	std::chrono::seconds m_lifecycle {60};
	string_t m_cookie_key {};
	std::map<string_view_t, session_ptr> m_session_map {};
	spin_shared_mutex m_map_mutex;
};

template <core_concepts::char_type CharT>
basic_session_set<CharT>::basic_session_set() :
	m_impl(new impl())
{

}

template <core_concepts::char_type CharT>
basic_session_set<CharT>::~basic_session_set()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_session_set<CharT>::basic_session_set(basic_session_set &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <core_concepts::char_type CharT>
basic_session_set<CharT> &basic_session_set<CharT>::operator=(basic_session_set &&other) noexcept
{
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <core_concepts::char_type CharT>
template <base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session_set<CharT>::make(Args&&...args) noexcept
	requires core_concepts::constructible<Session, Args...>
{
	auto session = std::make_shared<Session>(std::forward<Args>(args)...);
	m_impl->emplace(session);
	auto id = session->id();

	session->on_timeout([this, id = std::move(id)]{
		m_impl->erase(id);
	});
	session->set_lifecycle(lifecycle());
	return session;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session_set<CharT>::make(Args&&...args) noexcept
	requires core_concepts::constructible<basic_session<CharT>, Args...>
{
	auto session = std::make_shared<session_t>(std::forward<Args>(args)...);
	m_impl->emplace(session);
	auto id = session->id();

	session->on_timeout([this, id = std::move(id)]{
		m_impl->erase(id);
	});
	session->set_lifecycle(lifecycle());
	return session;
}

template <core_concepts::char_type CharT>
template <base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session_set<CharT>::get_or_make(string_view_t id, Args&&...args)
	requires core_concepts::constructible<Session, Args...>
{
	auto ptr = m_impl->find(id, false);
	if( not ptr )
		return make<Session>(std::forward<Args>(args)...);

	auto rptr = std::dynamic_pointer_cast<Session>(ptr);
	if( not rptr )
		throw runtime_error("libgs::http::session_set::get_or_make: type error.", xxtombs<char_t>(id));
	return rptr;
}

template <core_concepts::char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session_set<CharT>::get_or_make(string_view_t id, Args&&...args) noexcept
	requires core_concepts::constructible<basic_session<char_t>, Args...>
{
	auto ptr = m_impl->find(id, false);
	if( not ptr )
		return make(std::forward<Args>(args)...);
	return ptr;
}

template <core_concepts::char_type CharT>
template <base_of_session<CharT> Session>
std::shared_ptr<Session> basic_session_set<CharT>::get(string_view_t id)
{
	auto ptr = std::dynamic_pointer_cast<Session>(m_impl->find(id));
	if( not ptr )
		throw runtime_error("libgs::http::session_set::get: type error.", xxtombs<char_t>(id));
	return ptr;
}

template <core_concepts::char_type CharT>
basic_session_ptr<CharT> basic_session_set<CharT>::get(string_view_t id)
{
	return m_impl->find(id);
}

template <core_concepts::char_type CharT>
template <base_of_session<CharT> Session>
std::shared_ptr<Session> basic_session_set<CharT>::get_or(string_view_t id)
{
	auto ptr = std::dynamic_pointer_cast<Session>(m_impl->find(id, false));
	if( not ptr )
		throw runtime_error("libgs::http::session_set::get_or: type error.", xxtombs<char_t>(id));
	return ptr;
}

template <core_concepts::char_type CharT>
basic_session_ptr<CharT> basic_session_set<CharT>::get_or(string_view_t id) noexcept
{
	return m_impl->find(id, false);
}

template <core_concepts::char_type CharT>
template <typename Rep, typename Period>
basic_session_set<CharT> &basic_session_set<CharT>::set_lifecycle(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_lifecycle = sc::duration_cast<sc::seconds>(seconds);
	if( m_impl->m_lifecycle == 0 )
		m_impl->m_lifecycle = 1;
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <core_concepts::char_type CharT>
std::chrono::seconds basic_session_set<CharT>::lifecycle() const noexcept
{
	return std::chrono::seconds(m_impl->m_lifecycle);
}

template <core_concepts::char_type CharT>
basic_session_set<CharT> &basic_session_set<CharT>::set_cookie_key(string_view_t key)
{
	if( key.empty() )
		throw runtime_error("libgs::http::session::set_cookie_key: key is empty.", xxtombs<char_t>(key));
	m_impl->m_cookie_key = std::string(key.data(), key.size());
	return *this;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_session_set<CharT>::cookie_key() noexcept
{
	return m_impl->m_cookie_key;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_SET_H
