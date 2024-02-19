
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

#ifndef LIBGS_IO_TIMER_H
#define LIBGS_IO_TIMER_H

#include <libgs/io/device_base.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_waitable_timer = asio::basic_waitable_timer
<
	asio::chrono::steady_clock, 
	asio::wait_traits<asio::chrono::steady_clock>,
	Exec
>;

using asio_steady_timer = asio_basic_waitable_timer<>;

namespace io
{

template <concept_execution Exec = asio::any_io_executor>
class basic_timer : public device_base<Exec>
{
	LIBGS_DISABLE_COPY(basic_timer)
	using base_type = device_base<Exec>;

public:
	using executor_type = base_type::executor_type;
	using time_point = asio_steady_timer::time_point;
	using duration = asio_steady_timer::duration;

	template <typename...Args>
	using cb_token = opt_token<callback_t<Args...>>;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(Context &context = io_context());

	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(const duration &rtime, Context &context = io_context());

	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(const time_point &atime, Context &context = io_context());

	basic_timer(basic_timer &&other) noexcept;
	basic_timer &operator=(basic_timer &&other) noexcept;

	using base_type::base_type;
	~basic_timer() override;

public:
	void expires_after(const duration &rtime) noexcept;
	void expires_at(const time_point &atime) noexcept;

public:
	void wait(cb_token<error_code> tk);
	[[nodiscard]] awaitable<void> wait(opt_token<ua_redirect_error_t> tk);

public:
	void wait(const duration &rtime, error_code &error);
	void wait(const time_point &atime, error_code &error);
	void wait(error_code &error);

public:
	void cancel() noexcept override;
	bool is_run() const;

private:
	asio_steady_timer m_timer;
	std::atomic_bool m_run {false};
};

using timer = basic_timer<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_timer_ptr = std::shared_ptr<basic_timer<Exec>>;

using timer_ptr = basic_timer_ptr<>;

template <concept_execution Exec, typename...Args>
basic_timer_ptr<Exec> make_basic_timer(Args&&...args);

template <typename...Args>
timer_ptr make_timer(Args&&...args);

}} //namespace libgs::io

namespace libgs::io
{

template <concept_execution Exec>
template <concept_execution_context Context>
basic_timer<Exec>::basic_timer(Context &context) :
	base_type(context.get_executor()),
	m_timer(context)
{

}

template <concept_execution Exec>
template <concept_execution_context Context>
basic_timer<Exec>::basic_timer(const duration &rtime, Context &context) :
	base_type(context.get_executor()),
	m_timer(context)
{
	expires_after(rtime);
}

template <concept_execution Exec>
template <concept_execution_context Context>
basic_timer<Exec>::basic_timer(const time_point &atime, Context &context) :
	base_type(context.get_executor()),
	m_timer(context)
{
	expires_at(atime);
}

template <concept_execution Exec>
basic_timer<Exec>::basic_timer(basic_timer<Exec> &&other) noexcept :
	m_timer(std::move(other.m_timer)),
	m_run(other.m_run)
{
	this->m_exec = std::move(other.m_exec);
	this->m_valid = std::move(other.m_valid);
}

template <concept_execution Exec>
basic_timer<Exec> &basic_timer<Exec>::operator=(basic_timer<Exec> &&other) noexcept
{
	this->m_exec = std::move(other.m_exec);
	this->m_valid = std::move(other.m_valid);

	m_timer = std::move(other.m_timer);
	m_run = other.m_run;

	return *this;
}

template <concept_execution Exec>
basic_timer<Exec>::~basic_timer()
{
	m_run = false;
	m_timer.cancel();
}

template <concept_execution Exec>
void basic_timer<Exec>::expires_after(const duration &rtime) noexcept
{
	m_run = true;
	m_timer.expires_after(rtime);
}

template <concept_execution Exec>
void basic_timer<Exec>::expires_at(const time_point &atime) noexcept
{
	m_run = true;
	m_timer.expires_at(atime);
}

template <concept_execution Exec>
void basic_timer<Exec>::wait(cb_token<error_code> tk)
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		co_await wait({tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<void> basic_timer<Exec>::wait(opt_token<ua_redirect_error_t> tk)
{
	using namespace std::chrono;
	if( not is_run() )
	{
		if( tk.rtime == 0s )
		{
			tk.uare.ec_ = errc::timed_out;
			co_return ;
		}
		expires_after(tk.rtime);
	}
	co_await m_timer.async_wait(tk.uare);
	co_return ;
}

template <concept_execution Exec>
void basic_timer<Exec>::wait(const duration &rtime, error_code &error)
{
	if( not is_run() )
		expires_after(rtime);
	m_timer.wait(error);
}

template <concept_execution Exec>
void basic_timer<Exec>::wait(const time_point &atime, error_code &error)
{
	if( not is_run() )
		expires_at(atime);
	m_timer.wait(error);
}

template <concept_execution Exec>
void basic_timer<Exec>::wait(error_code &error)
{
	if( is_run() )
		m_timer.wait(error);
	else
		error = errc::timed_out;
}

template <concept_execution Exec>
void basic_timer<Exec>::cancel() noexcept
{
	m_run = false;
	m_timer.cancel();
}

template <concept_execution Exec>
bool basic_timer<Exec>::is_run() const
{
	return m_run;
}

template <concept_execution Exec, typename...Args>
basic_timer_ptr<Exec> make_basic_timer(Args&&...args)
{
	return std::make_shared<basic_timer<Exec>>(std::forward<Args>(args)...);
}

template <typename...Args>
timer_ptr make_timer(Args&&...args)
{
	return std::make_shared<timer>(std::forward<Args>(args)...);
}

} //namespace libgs::io


#endif //LIBGS_IO_TIMER_H

