
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

class LIBGS_HTTP_VAPI session_set::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;

	session_ptr find(std::string_view id, bool _throw = true)
	{
		spin_shared_shared_lock locker(m_map_mutex); LIBGS_UNUSED(locker);
		auto it = m_session_map.find(id);

		if( it == m_session_map.end() )
		{
			if( _throw )
			{
				throw runtime_error (
					"libgs::http::session_set: <map>: id '{}' not exists.", id
				);
			}
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

	void erase(std::string_view id)
	{
		m_map_mutex.lock();
		m_session_map.erase(std::string(id.data(), id.size()));
		m_map_mutex.unlock();
	}

public:
	std::chrono::seconds m_lifecycle {60};
	std::string m_cookie_key = "session";

	std::map<std::string_view, session_ptr> m_session_map {};
	spin_shared_mutex m_map_mutex;

	std::function<void(session_ptr,error_code)> m_error_handle {};
};

inline session_set::session_set() :
	m_impl(new impl())
{

}

inline session_set::~session_set()
{
	delete m_impl;
}

inline session_set::session_set(session_set &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

inline session_set &session_set::operator=(session_set &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <typename Session, typename...Args>
std::shared_ptr<Session> session_set::make(Args&&...args) noexcept requires
	core_concepts::base_of<Session,session> and core_concepts::constructible<Session,Args...>
{
	auto session = std::make_shared<Session>(
		std::forward<Args>(args)...
	);
	m_impl->emplace(session);
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
session_ptr session_set::make(Args&&...args) noexcept
	requires core_concepts::constructible<session,Args...>
{
	return make<session>(std::forward<Args>(args)...);
}

template <typename Session, typename...Args>
std::shared_ptr<Session> session_set::get_or_make
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
			"libgs::http::session_set::get_or_make: type error [id = {}].", id_view
		);
	}
	return rptr;
}

template <typename...Args>
session_ptr session_set::get_or_make
(const core_concepts::text_p<char> auto &id, Args&&...args) noexcept
	requires core_concepts::constructible<session,Args...>
{
	auto ptr = m_impl->find(strtls::to_view(id), false);
	if( not ptr )
		return make(std::forward<Args>(args)...);
	return ptr;
}

template <typename Session>
std::shared_ptr<Session> session_set::get(const core_concepts::text_p<char> auto &id)
	requires core_concepts::base_of<Session,session>
{
	auto id_view = strtls::to_view(id);
	auto ptr = std::dynamic_pointer_cast<Session>(
		m_impl->find(id_view)
	);
	if( not ptr )
	{
		throw runtime_error(
			"libgs::http::session_set::get: type error [id = {}].", id_view
		);
	}
	return ptr;
}

template <typename Session>
std::shared_ptr<Session> session_set::get_or(const core_concepts::text_p<char> auto &id)
	requires core_concepts::base_of<Session,session>
{
	auto id_view = strtls::to_view(id);
	auto ptr = std::dynamic_pointer_cast<Session>(
		m_impl->find(id_view, false)
	);
	if( not ptr )
	{
		throw runtime_error(
			"libgs::http::session_set::get_or: type error [id = {}].", id_view
		);
	}
	return ptr;
}

session_ptr session_set::get(const core_concepts::text_p<char> auto &id)
{
	return m_impl->find(strtls::to_view(id));
}

session_ptr session_set::get_or(const core_concepts::text_p<char> auto &id) noexcept
{
	return m_impl->find(strtls::to_view(id), false);
}

template <typename Rep, typename Period>
session_set &session_set::set_lifecycle(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	using namespace std::chrono_literals;

	m_impl->m_lifecycle = sc::duration_cast<sc::seconds>(seconds);
	if( m_impl->m_lifecycle.count() == 0 )
		m_impl->m_lifecycle = 1s;
	return *this;
}

inline std::chrono::seconds session_set::lifecycle() const noexcept
{
	return std::chrono::seconds(m_impl->m_lifecycle);
}

session_set &session_set::set_cookie_key(core_concepts::text_p<char> auto &&key)
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

inline std::string_view session_set::cookie_key() noexcept
{
	return m_impl->m_cookie_key;
}

template <core_concepts::callable<session_ptr,error_code> Func>
session_set &session_set::on_error(Func &&func)
{
	m_impl->m_error_handle = std::forward<Func>(func);
	return *this;
}

inline session_set &session_set::unbind_error()
{
	m_impl->m_error_handle = nullptr;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_SET_H
