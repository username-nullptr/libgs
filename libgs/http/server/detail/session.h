
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
#include <libgs/coro.h>

namespace libgs::http
{

class LIBGS_HTTP_VAPI session::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Rep, typename Period = std::ratio<1>>
	impl(session *q_ptr, const duration<Rep,Period> &seconds, const executor_t &exec) :
		q_ptr(q_ptr), m_second(seconds.count()), m_timer(exec) {}

public:
	void start()
	{
		if( m_restart )
			m_timer.cancel();
		else if( not m_valid )
			dispatch(work());
	}

private:
	[[nodiscard]] awaitable<void> work()
	{
		auto self = q_ptr->shared_from_this();
		error_code error;
		for(;;)
		{
			self->m_impl->m_restart = false;
			self->m_impl->m_timer.expires_after(std::chrono::seconds(self->m_impl->m_second));

			using namespace libgs::operators;
			co_await self->m_impl->m_timer.async_wait(use_awaitable|error);
			if( self.use_count() == 1 )
				break;

			if( error and error.value() != errc::operation_aborted )
			{
				if( self->m_impl->m_error_handle )
					self->m_impl->m_error_handle(error);
			}
			if( self->m_impl->m_restart )
				continue;

			self->m_impl->m_valid = false;
			self->m_impl->m_timeout_handle();
			break;
		}
		co_return ;
	}

public:
	session *q_ptr = nullptr;
	const std::string m_id = uuid::generate();
	time_point_t m_create_time = std::chrono::system_clock::now();

	attributes_t m_attributes {};
	std::atomic<uint64_t> m_second;

	std::atomic_bool m_valid = false;
	std::atomic_bool m_restart = false;

	asio::steady_timer m_timer;
	std::function<void()> m_timeout_handle {};
	std::function<void(error_code)> m_error_handle {};
};

template <typename Rep, typename Period>
session::session(const duration<Rep,Period> &seconds, const executor_t &exec) :
	m_impl(new impl(this, seconds, exec))
{
	m_impl->start();
}

std::any session::attribute(const core_concepts::text_p<char> auto &key) const
{
	auto it = m_impl->m_attributes.find(strtls::to_string(key));
	if( it == m_impl->m_attributes.end() )
	{
		throw runtime_error (
			"libgs::http::session::attribute: key '{}' not exists.", key
		);
	}
	return it->second;
}

std::any session::attribute_or(const core_concepts::text_p<char> auto &key, std::any default_value) const noexcept
{
	auto it = m_impl->m_attributes.find(strtls::to_string(key));
	return it == m_impl->m_attributes.end() ? std::move(default_value) : it->second;
}

session &session::set_attribute(core_concepts::text_p<char> auto &&key, std::any value) noexcept
{
	m_impl->m_attributes[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return *this;
}

session &session::unset_attribute(const core_concepts::text_p<char> auto &key) noexcept
{
	m_impl->m_attributes.erase(strtls::to_string(key));
	return *this;
}

template <typename Rep, typename Period>
session &session::set_lifecycle(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_second = sc::duration_cast<sc::seconds>(seconds).count();
	if( m_impl->m_second == 0 )
		m_impl->m_second = 1;
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

template <typename Rep, typename Period>
session &session::expand(const duration<Rep,Period> &seconds)
{
	namespace sc = std::chrono;
	m_impl->m_second += sc::duration_cast<sc::seconds>(seconds).count();
	return expand();
}

template <core_concepts::callable Func>
session &session::on_timeout(Func &&func)
{
	m_impl->m_timeout_handle = std::forward<Func>(func);
	return *this;
}

template <core_concepts::callable<error_code> Func>
session &session::on_error(Func &&func)
{
	m_impl->m_error_handle = std::forward<Func>(func);
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_H
