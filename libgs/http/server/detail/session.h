
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
#include <spdlog/spdlog.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Rep, typename Period = std::ratio<1>>
	impl(basic_session *q_ptr, const duration<Rep,Period> &seconds, const executor_t &exec) :
		q_ptr(q_ptr), m_second(seconds.count()), m_timer(exec) {}

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

		co_spawn_detached([_self = this->q_ptr]() -> awaitable<void>
		{
			auto self = _self->shared_from_this();
			error_code error;
			do {
				self->m_impl->m_restart = false;
				self->m_impl->m_timer.expires_after(std::chrono::seconds(self->m_impl->m_second));

				co_await self->m_impl->m_timer.async_wait(use_awaitable|error);
				if( self.use_count() == 1 )
					break;

				if( error and error.value() != errc::operation_aborted )
					spdlog::error("libgs::http::session: timer error: '{}'.", error);
				if( self->m_impl->m_restart )
					continue;

				self->m_impl->m_valid = false;
				self->m_impl->m_timeout_handle();
			}
			while(false);
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

	asio::steady_timer m_timer;
	std::function<void()> m_timeout_handle {};
};

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT>::basic_session(const duration<Rep,Period> &seconds, const executor_t &exec) :
	m_impl(new impl(this, seconds, exec))
{
	m_impl->start();
}

template <concept_char_type CharT>
basic_session<CharT>::basic_session(const executor_t &exec) :
	basic_session(std::chrono::seconds(60), exec)
{

}

template <concept_char_type CharT>
basic_session<CharT>::~basic_session()
{
	spdlog::debug("libgs::http::basic_session::~basic_session: '{}'", xxtombs<CharT>(id()));
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
	m_impl->m_second = sc::duration_cast<sc::seconds>(seconds).count();
	if( m_impl->m_second == 0 )
		m_impl->m_second = 1;
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT> &basic_session<CharT>::expand(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_second += sc::duration_cast<sc::seconds>(seconds).count();
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
template <concept_callable Func>
basic_session<CharT> &basic_session<CharT>::on_timeout(Func &&func)
{
	m_impl->m_timeout_handle = std::forward<Func>(func);
	return *this;
}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::unbind_timeout()
{
	m_impl->m_timeout_handle = nullptr;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_H
