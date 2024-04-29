
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

#ifndef LIBGS_HTTP_SERVER_SESSION_H
#define LIBGS_HTTP_SERVER_SESSION_H

#include <libgs/http/basic/types.h>
#include <libgs/http/server/detail/session_base.h>

namespace libgs::http
{

template <concept_char_type CharT>
using basic_session_attributes = std::map<std::basic_string<CharT>, std::any>;

using session_attributes = basic_session_attributes<char>;
using wsession_attributes = basic_session_attributes<wchar_t>;

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_session :
	public std::enable_shared_from_this<basic_session<CharT>>,
	protected detail::session_base<CharT>
{
	LIBGS_DISABLE_COPY_MOVE(basic_session)
	using base_type = detail::session_base<CharT>;

public:
	template<typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::duration<Rep,Period>;
	using time_point = decltype(std::chrono::system_clock::now());

	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;
	using value_type = basic_value<CharT>;

	using attributes_type = basic_session_attributes<CharT>;
	using ptr = std::shared_ptr<basic_session>;

	template <typename Session>	requires std::is_base_of_v<basic_session,Session>
	using dt_ptr = std::shared_ptr<Session>;

public:
	template <typename Rep, typename Period = std::ratio<1>>
	explicit basic_session(const duration<Rep,Period> &seconds);

	basic_session();
	virtual ~basic_session();

public:
	[[nodiscard]] str_view_type id() const noexcept;
	[[nodiscard]] time_point create_time() const noexcept;
	[[nodiscard]] bool is_valid() const noexcept;

public:
	[[nodiscard]] std::any attribute(str_view_type key) const;
	[[nodiscard]] std::any attribute_or(str_view_type key, std::any default_value = {}) const noexcept;
	[[nodiscard]] const attributes_type &attributes() const noexcept;

public:
	basic_session &set_attribute(str_view_type key, const std::any &value);
	basic_session &set_attribute(str_view_type key, std::any &&value);
	basic_session &unset_attribute(str_view_type key);

public:
	[[nodiscard]] std::chrono::seconds lifecycle() const noexcept;
	void invalidate();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &set_lifecycle(const duration<Rep,Period> &seconds);
	basic_session &set_lifecycle();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &expand(const duration<Rep,Period> &seconds);

public:
	template <typename Session, typename...Args>
	static dt_ptr<Session> make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	static ptr make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename Session, typename...Args>
	static dt_ptr<Session> get(str_view_type id);

	template <typename...Args>
	static ptr get(str_view_type id);

	template <typename Session, typename...Args>
	static dt_ptr<Session> get_or_make(str_view_type id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	static ptr get_or_make(str_view_type id, Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

private:
	static ptr _find(str_view_type id, bool _throw = true);
	auto _emplace();
	void _erase();

private:
	class impl;
	friend class impl;
	impl *m_impl;
};

using session = basic_session<char>;
using wsession = basic_session<wchar_t>;

template <concept_char_type CharT>
using basic_session_ptr = typename basic_session<CharT>::ptr;

using session_ptr = basic_session_ptr<char>;
using wsession_ptr = basic_session_ptr<wchar_t>;

} //namespace libgs::http
//#include <libgs/http/context/detail/session.h>


#include <libgs/core/algorithm/uuid.h>
#include <libgs/core/coroutine.h>
#include <libgs/io/timer.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(basic_session *q_ptr) :
		impl(q_ptr, detail::session_duration_base::global_lifecycle()) {}

	template <typename Rep, typename Period = std::ratio<1>>
	impl(basic_session *q_ptr, const duration<Rep,Period> &seconds) :
		q_ptr(q_ptr), m_second(seconds.count())
	{
		start();
	}

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
					libgs_log_error("libgs::http::session: timer error: '{}'.", error);
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
	basic_session *q_ptr;
	const str_type m_id = basic_uuid<CharT>::generate();
	time_point m_create_time = std::chrono::system_clock::now();

	attributes_type m_attributes;
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
	m_impl(new impl(this))
{

}

template <concept_char_type CharT>
basic_session<CharT>::~basic_session()
{
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
std::any basic_session<CharT>::attribute(str_view_type key) const
{
	auto it = m_impl->m_attributes.find(key);
	if( it == m_impl->m_attributes.end() )
		throw runtime_error("libgs::http::session::attribute: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute_or(str_view_type key, std::any default_value) const noexcept
{
	auto it = m_impl->m_attributes.find(key);
	return it == m_impl->m_attributes.end() ? std::move(default_value) : it->second;
}

template <concept_char_type CharT>
const basic_session_attributes<CharT> &basic_session<CharT>::attributes() const noexcept
{
	return m_impl->m_attributes;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_view_type key, const std::any &value)
{
	m_impl->m_attributes[key] = value;
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_view_type key, std::any &&value)
{
	m_impl->m_attributes[key] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::unset_attribute(str_view_type key)
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
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <concept_char_type CharT>
template <typename Session, typename...Args>
typename basic_session<CharT>::template dt_ptr<Session> basic_session<CharT>::make(Args&&...args) noexcept
	requires requires{ session_type(std::forward<Args>(args)...); }
{
	return std::make_shared<Session>(std::forward<Args>(args)...);
}

template <concept_char_type CharT>
template <typename...Args>
typename basic_session<CharT>::ptr basic_session<CharT>::make(Args&&...args) noexcept
	requires requires{ session_type(std::forward<Args>(args)...); }
{
	return std::make_shared<basic_session>(std::forward<Args>(args)...);
}

template <concept_char_type CharT>
template <typename Session, typename...Args>
typename basic_session<CharT>::template dt_ptr<Session> basic_session<CharT>::get(str_view_type id)
{
	auto ptr = std::dynamic_pointer_cast<Session>(_find(id));
	if( not ptr )
		throw runtime_error("libgs::http::session::get_or_make: type error.", xxtombs<CharT>(id));
	return ptr;
}

template <concept_char_type CharT>
template <typename...Args>
typename basic_session<CharT>::ptr basic_session<CharT>::get(str_view_type id)
{
	return _find(id);
}

template <concept_char_type CharT>
template <typename Session, typename...Args>
typename basic_session<CharT>::template dt_ptr<Session> basic_session<CharT>::get_or_make(str_view_type id, Args&&...args)
	requires requires{ session_type(std::forward<Args>(args)...); }
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
typename basic_session<CharT>::ptr basic_session<CharT>::get_or_make(str_view_type id, Args&&...args) noexcept
	requires requires{ session_type(std::forward<Args>(args)...); }
{
	auto ptr = _find(id, false);
	if( not ptr )
		return make(std::forward<Args>(args)...);
	return ptr;
}

template <concept_char_type CharT>
typename basic_session<CharT>::ptr basic_session<CharT>::_find(str_view_type id, bool _throw)
{
	shared_lock locker(base_type::rwlock);
	auto it = base_type::session_map.find(id);

	if( it == base_type::session_map.end() )
	{
		if( _throw )
			throw runtime_error("libgs::http::session: <map>: id '{}' not exists.", xxtombs<CharT>(id));
		return {};
	}
	return it->second;
}

template <concept_char_type CharT>
auto basic_session<CharT>::_emplace()
{
	base_type::rwlock.lock();
	auto pair = base_type::session_map.emplace(id(), this->shared_from_this());
	base_type::rwlock.unlock();
	return pair;
}

template <concept_char_type CharT>
void basic_session<CharT>::_erase()
{
	base_type::rwlock.lock();
	base_type::session_map.erase(id());
	base_type::rwlock.unlock();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_SESSION_H
