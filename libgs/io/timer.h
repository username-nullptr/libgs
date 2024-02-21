
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
#include <libgs/io/detail/timer.h>


#endif //LIBGS_IO_TIMER_H

