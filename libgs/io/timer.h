
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
class LIBGS_CORE_TAPI basic_timer : public device_base<Exec>
{
	LIBGS_DISABLE_COPY(basic_timer)
	using base_type = device_base<Exec>;

public:
	using executor_type = base_type::executor_type;
	using time_point = asio_steady_timer::time_point;
	using duration = asio_steady_timer::duration;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(Context &context = execution::io_context());

	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(const duration &rtime, Context &context = execution::io_context());

	template <concept_execution_context Context = asio::io_context>
	explicit basic_timer(const time_point &atime, Context &context = execution::io_context());

	basic_timer(basic_timer &&other) noexcept;
	basic_timer &operator=(basic_timer &&other) noexcept;

	using base_type::base_type;
	~basic_timer() override;

public:
	void expires_after(const duration &rtime) noexcept;
	void expires_at(const time_point &atime) noexcept;

public:
	void async_wait(opt_token<callback_t<error_code>> tk) noexcept;
	void async_wait(opt_token<callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<void> co_wait(opt_token<error_code&> tk = {});
	void wait(opt_token<error_code&> tk = {});

public:
	void cancel() noexcept override;
	[[nodiscard]] bool is_run() const;

	void set_non_block(bool flag, error_code &error) noexcept override;
	bool is_non_block() const noexcept override;

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

