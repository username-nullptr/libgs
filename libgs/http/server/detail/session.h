
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SESSION_H
#define LIBGS_HTTP_SERVER_DETAIL_SESSION_H

#include <libgs/core/algorithm/uuid.h>
#include <libgs/core/coroutine.h>
#include <libgs/io/timer.h>
#include <spdlog/spdlog.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Rep, typename Period = std::ratio<1>>
	impl(basic_session *q_ptr, const duration<Rep,Period> &seconds) :
		q_ptr(q_ptr), m_second(seconds.count()) {}

public:
	void start()
	{
		if( m_restart )
		{
			m_timer.cancel();
			return ;
		}
		else if( m_valid )
			return ;

		q_ptr->_emplace();
		co_spawn_detached([this]() -> awaitable<void>
		{
			error_code error;
			for(;;)
			{
				m_restart = false;
				co_await m_timer.wait({std::chrono::seconds(m_second), error});

				if( error and error.value() != errc::operation_aborted )
					spdlog::error("libgs::http::session: timer error: '{}'.", error);
				if( m_restart )
					continue;

				m_valid = false;
				q_ptr->_erase();
				break;
			}
			co_return ;
		});
	}

public:
	basic_session *q_ptr = nullptr;
	const string_t m_id = basic_uuid<CharT>::generate();
	time_point m_create_time = std::chrono::system_clock::now();

	attributes_t m_attributes {};
	std::atomic<uint64_t> m_second;

	std::atomic_bool m_valid = false;
	std::atomic_bool m_restart = false;
	io::timer m_timer;
};

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT>::basic_session(const duration<Rep,Period> &seconds) :
	m_impl(new impl(this, seconds))
{

}

template <concept_char_type CharT>
basic_session<CharT>::basic_session() :
	basic_session(detail::session_duration_base::global_lifecycle())
{

}

template <concept_char_type CharT>
basic_session<CharT>::~basic_session()
{
	spdlog::debug("~session: '{}'", xxtombs<CharT>(id()));
	delete m_impl;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_session<CharT>::id() const noexcept
{
	return m_impl->m_id;
}

template <concept_char_type CharT>
typename basic_session<CharT>::time_point basic_session<CharT>::create_time() const noexcept
{
	return m_impl->m_create_time;
}

template <concept_char_type CharT>
bool basic_session<CharT>::is_valid() const noexcept
{
	return m_impl->m_valid;
}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute(string_view_t key) const
{
	auto it = m_impl->m_attributes.find({key.data(), key.size()});
	if( it == m_impl->m_attributes.end() )
		throw runtime_error("libgs::http::session::attribute: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute_or(string_view_t key, std::any default_value) const noexcept
{
	auto it = m_impl->m_attributes.find({key.data(), key.size()});
	return it == m_impl->m_attributes.end() ? std::move(default_value) : it->second;
}

template <concept_char_type CharT>
const basic_session_attributes<CharT> &basic_session<CharT>::attributes() const noexcept
{
	return m_impl->m_attributes;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(string_view_t key, const std::any &value)
{
	m_impl->m_attributes[{key.data(), key.size()}] = value;
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(string_view_t key, std::any &&value)
{
	m_impl->m_attributes[{key.data(), key.size()}] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::unset_attribute(string_view_t key)
{
	m_impl->m_attributes.erase(key);
	return *this;
}

template <concept_char_type CharT>
std::chrono::seconds basic_session<CharT>::lifecycle() const noexcept
{
	return std::chrono::seconds(m_impl->m_second);
}

template <concept_char_type CharT>
void basic_session<CharT>::invalidate()
{
	m_impl->m_valid = false;
	m_impl->m_timer.cancel();
}

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT> &basic_session<CharT>::set_lifecycle(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_second = sc::duration_cast<sc::seconds>(seconds);
	if( m_impl->m_second == 0 )
		m_impl->m_second == 1;
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_lifecycle()
{
	namespace sc = std::chrono;
	m_impl->m_second = detail::session_duration_base::global_lifecycle();
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT> &basic_session<CharT>::expand(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_second += sc::duration_cast<sc::seconds>(seconds);
	return expand();
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::expand()
{
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <concept_char_type CharT>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session<CharT>::make(Args&&...args) noexcept
	requires detail::can_construct<Session, Args...>
{
	auto ptr = std::make_shared<Session>(std::forward<Args>(args)...);
	ptr->m_impl->start();
	return ptr;
}

template <concept_char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session<CharT>::make(Args&&...args) noexcept
	requires detail::can_construct<basic_session<CharT>, Args...>
{
	auto ptr = std::make_shared<basic_session>(std::forward<Args>(args)...);
	ptr->m_impl->start();
	return ptr;
}

template <concept_char_type CharT>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session<CharT>::get_or_make(string_view_t id, Args&&...args)
	requires detail::can_construct<Session, Args...>
{
	auto ptr = _find(id, false);
	if( not ptr )
		return make<Session>(std::forward<Args>(args)...);

	auto rptr = std::dynamic_pointer_cast<Session>(ptr);
	if( not rptr )
		throw runtime_error("libgs::http::session::get_or_make: type error.", xxtombs<CharT>(id));
	return rptr;
}

template <concept_char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session<CharT>::get_or_make(string_view_t id, Args&&...args) noexcept
	requires detail::can_construct<basic_session<CharT>, Args...>
{
	auto ptr = _find(id, false);
	if( not ptr )
		return make(std::forward<Args>(args)...);
	return ptr;
}

template <concept_char_type CharT>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session<CharT>::get(string_view_t id)
{
	auto ptr = std::dynamic_pointer_cast<Session>(_find(id));
	if( not ptr )
		throw runtime_error("libgs::http::session::get: type error.", xxtombs<CharT>(id));
	return ptr;
}

template <concept_char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session<CharT>::get(string_view_t id)
{
	return _find(id);
}

template <concept_char_type CharT>
template <detail::base_of_session<CharT> Session, typename...Args>
std::shared_ptr<Session> basic_session<CharT>::get_or(string_view_t id)
{
	auto ptr = std::dynamic_pointer_cast<Session>(_find(id, false));
	if( not ptr )
		throw runtime_error("libgs::http::session::get_or: type error.", xxtombs<CharT>(id));
	return ptr;
}

template <concept_char_type CharT>
template <typename...Args>
basic_session_ptr<CharT> basic_session<CharT>::get_or(string_view_t id) noexcept
{
	return _find(id, false);
}

template <concept_char_type CharT>
void basic_session<CharT>::set_cookie_key(string_t key)
{
	if( key.empty() )
		throw runtime_error("libgs::http::session::set_cookie_key: key is empty.", xxtombs<CharT>(key));
	base_t::cookie_key() = std::move(key);
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_session<CharT>::cookie_key() noexcept
{
	return base_t::cookie_key();
}

template <concept_char_type CharT>
basic_session_ptr<CharT> basic_session<CharT>::_find(string_view_t id, bool _throw)
{
	shared_lock locker(base_t::map_rwlock()); LIBGS_UNUSED(locker);
	auto it = base_t::session_map().find(id);

	if( it == base_t::session_map().end() )
	{
		if( _throw )
			throw runtime_error("libgs::http::session: <map>: id '{}' not exists.", xxtombs<CharT>(id));
		return {};
	}
	it->second->expand();
	return it->second;
}

template <concept_char_type CharT>
auto basic_session<CharT>::_emplace()
{
	base_t::map_rwlock().lock();
	auto pair = base_t::session_map().emplace(id(), this->shared_from_this());
	base_t::map_rwlock().unlock();
	return pair;
}

template <concept_char_type CharT>
void basic_session<CharT>::_erase()
{
	base_t::map_rwlock().lock();
	base_t::session_map().erase(id());
	base_t::map_rwlock().unlock();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_H
