
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

#ifndef LIBGS_HTTP_CONTEXT_DETAIL_SESSION_H
#define LIBGS_HTTP_CONTEXT_DETAIL_SESSION_H

#include <libgs/io/timer.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session<CharT>::impl
{
public:
	explicit impl(const duration &s) {
		set_lifecycle(s);
	}

public:
	void set_lifecycle(const duration &s)
	{
		if( s.count() > 0 )
			m_lifecycle = s.count();
		else
			m_lifecycle = g_global_lifecycle.load();
	}

	awaitable<void> restart(uint64_t s = 0)
	{
		using namespace std::chrono;
		m_timer.cancel();

		if( s == 0 )
			m_timer.expires_after(seconds(m_lifecycle));
		else
			m_timer.expires_after(seconds(s));

		error_code error;
		co_await m_timer.co_wait(error);

		if( error )
		{
			if( m_is_valid )
				co_return ;
		}
		else
		{
			m_is_valid = false;
			m_attrs_mutex.lock();
			m_attributes.clear();
			m_attrs_mutex.unlock();
		}
		g_rw_mutex.lock();
		g_session_hash.erase(m_id);
		g_rw_mutex.unlock();
		co_return ;
	}

public:
	std::atomic<uint64_t> m_lifecycle {0};
	io::timer m_timer;

	std::string m_id { uuid::generate() };
	uint64_t m_create_time = duration_cast<std::chrono::seconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();

	shared_mutex m_attrs_mutex;
	session_attributes m_attributes;
	std::atomic_bool m_is_valid {true};

	inline static std::atomic<uint64_t> g_global_lifecycle {1800}; //s
	inline static std::unordered_map<std::string, ptr> g_session_hash;
	inline static shared_mutex g_rw_mutex;
};

template <concept_char_type CharT>
basic_session<CharT>::basic_session(const duration &s) :
	m_impl(new impl(s))
{

}

template <concept_char_type CharT>
basic_session<CharT>::~basic_session()
{
	delete_later(m_impl);
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_session<CharT>::id() const
{
	return m_impl->m_id;
}

template <concept_char_type CharT>
uint64_t basic_session<CharT>::create_time() const
{
	return m_impl->m_create_time;
}

template <concept_char_type CharT>
bool basic_session<CharT>::is_valid() const
{
	return m_impl->m_is_valid;
}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute(const str_type &key) const
{
	shared_lock locker(m_impl->m_attrs_mutex); LIBGS_UNUSED(locker);
	auto it = m_impl->m_attributes.find(key);

	if( it == m_impl->m_attributes.end() )
	{
		if constexpr( is_char_v<CharT> )
			throw runtime_error("libgs::http::session::attribute: key '{}' does not exist.", key);
		else
			throw runtime_error("libgs::http::session::attribute: key '{}' does not exist.", wcstombs(key));
	}
	return it->second;
}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute_or(const str_type &key, std::any default_value) const noexcept
{
	shared_lock locker(m_impl->m_attrs_mutex); LIBGS_UNUSED(locker);
	auto it = m_impl->m_attributes.find(key);
	return it == m_impl->m_attributes.end()? std::move(default_value) : it->second;
}

template <concept_char_type CharT>
template <typename T>
T basic_session<CharT>::attribute(const str_type &key)
	requires is_dsame_v<T, std::any>
{
	return attribute(key).template get<T>();
}

template <concept_char_type CharT>
template <typename T>
T basic_session<CharT>::attribute_or(const str_type &key, T default_value) const noexcept
	requires is_dsame_v<T, std::any>
{
	return attribute_or(key, default_value).template get<T>();
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_type key, const std::any &v)
{
	m_impl->m_attrs_mutex.lock();
	m_impl->m_attributes[std::move(key)] = v;
	m_impl->m_attrs_mutex.unlock();
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_type key, std::any &&v)
{
	m_impl->m_attrs_mutex.lock();
	m_impl->m_attributes[std::move(key)] = std::move(v);
	m_impl->m_attrs_mutex.unlock();
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::unset_attribute(const str_type &key)
{
	m_impl->m_attrs_mutex.lock();
	m_impl->m_attributes.erase(key);
	m_impl->m_attrs_mutex.unlock();
	return *this;
}

template <concept_char_type CharT>
size_t basic_session<CharT>::attribute_count() const
{
	shared_lock locker(m_impl->m_attrs_mutex); LIBGS_UNUSED(locker);
	return m_impl->m_attributes.size();
}

template <concept_char_type CharT>
session_attributes basic_session<CharT>::attributes() const
{
	shared_lock locker(m_impl->m_attrs_mutex); LIBGS_UNUSED(locker);
	return m_impl->m_attributes;
}

template <concept_char_type CharT>
string_list basic_session<CharT>::attribute_key_list() const
{
	string_list list;
	m_impl->m_attrs_mutex.lock_shared();

	for(auto &pair : m_impl->m_attributes)
		list.emplace_back(pair.first);

	m_impl->m_attrs_mutex.unlock_shared();
	return list;
}

template <concept_char_type CharT>
std::set<std::basic_string<CharT>> basic_session<CharT>::attribute_key_set() const
{
	std::set<std::string> set;
	m_impl->m_attrs_mutex.lock_shared();

	for(auto &pair : m_impl->m_attributes)
		set.emplace(pair.first);

	m_impl->m_attrs_mutex.unlock_shared();
	return set;
}

template <concept_char_type CharT>
uint64_t basic_session<CharT>::lifecycle() const
{
	return m_impl->m_lifecycle;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_lifecycle(const duration &s)
{
	using namespace std::chrono;
	m_impl->set_lifecycle(s);

	uint64_t ctime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
	uint64_t cat = m_impl->m_create_time + s.count();

	if( ctime < cat )
		co_spawn_detached(m_impl->restart(ctime - cat));
	else
		invalidate();
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::expand(const duration &s)
{
	if( s.count() > 0 )
		m_impl->m_lifecycle = s.count();
	co_spawn_detached(m_impl->restart());
	return *this;
}

template <concept_char_type CharT>
void basic_session<CharT>::invalidate()
{
	m_impl->m_is_valid = false;
	m_impl->m_attrs_mutex.lock();
	m_impl->m_attributes.clear();
	m_impl->m_attrs_mutex.unlock();
	m_impl->m_timer.cancel();
}

template <concept_char_type CharT>
typename basic_session<CharT>::ptr basic_session<CharT>::make_shared(const duration &s)
{
	auto obj = new session(s); set(obj);
	return session::get(obj->id());
}

template <concept_char_type CharT>
typename basic_session<CharT>::ptr basic_session<CharT>::get(const str_type &id)
{
	shared_lock locker(impl::g_rw_mutex); LIBGS_UNUSED(locker);
	auto it = impl::g_session_hash.find(id);
	return it == impl::g_session_hash.end()? session_ptr() : it->second;
}

template <concept_char_type CharT>
void basic_session<CharT>::set(basic_session *obj)
{
	if( obj == nullptr )
		return ;

	impl::g_rw_mutex.lock();
	impl::g_session_hash.emplace(obj->id(), session_ptr(obj));

	impl::g_rw_mutex.unlock();
	co_spawn_detached(obj->m_impl->restart());
}

template <concept_char_type CharT>
template <class Sesn>
std::shared_ptr<Sesn> basic_session<CharT>::get(const std::string &id)
	requires std::is_base_of_v<basic_session, Sesn>
{
	auto ptr = get(id);
	if( ptr == nullptr )
		return std::shared_ptr<Sesn>();

	auto dy_ptr = std::dynamic_pointer_cast<Sesn>(ptr);
	if( dy_ptr == nullptr )
		throw exception("gts::http::session::get<{}>: The type of 'session' is incorrect.", type_name<Sesn>());
	return dy_ptr;
}

template <concept_char_type CharT>
void basic_session<CharT>::set_global_lifecycle(const duration &seconds)
{
	if( seconds.count() > 0 )
		impl::g_global_lifecycle = seconds.count();
}

template <concept_char_type CharT>
std::chrono::seconds basic_session<CharT>::global_lifecycle()
{
	return duration(impl::g_global_lifecycle);
}

template <class Sesn>
std::shared_ptr<Sesn> make_session(const std::chrono::seconds &seconds)
	requires std::is_base_of_v<session,Sesn> or std::is_base_of_v<wsession,Sesn>
{
	auto obj = new Sesn(seconds); obj->set(obj);
	return std::dynamic_pointer_cast<Sesn>(session::get(obj->id()));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CONTEXT_DETAIL_SESSION_H
