
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

#ifndef LIBGS_CORO_DETAIL_WAKE_UP_H
#define LIBGS_CORO_DETAIL_WAKE_UP_H

#include <libgs/core/execution.h>

namespace libgs::coro::detail
{

class LIBGS_CORE_VAPI lock_wake_up final :
	public std::enable_shared_from_this<lock_wake_up>
{
	LIBGS_DISABLE_COPY_MOVE(lock_wake_up)

public:
	using ptr_t = std::shared_ptr<lock_wake_up>;
	using handler_t = async_work<bool>::handler_t;

	lock_wake_up(const asio::any_io_executor &exec, handler_t handler) :
		m_handler(std::move(handler)), m_exec(exec) {}

public:
	bool operator()(bool success)
	{
		if( m_finished.test_and_set() )
			return false;
		m_timer.cancel();

		dispatch(m_exec, [
			success, handler = std::make_shared<handler_t>(std::move(m_handler))
		]() mutable {
			std::move(*handler)(success);
		});
		return true;
	}

	void start_timer(const auto &timeout)
	{
		m_timer.expires_after(timeout);
		m_timer.async_wait([this](const error_code &error) mutable
		{
			if( not error )
				(*this)(false);
		});
	}

private:
	handler_t m_handler;
	asio::any_io_executor m_exec;
	asio::steady_timer m_timer{m_exec};
	std::atomic_flag m_finished;
};

using lock_wake_up_ptr = lock_wake_up::ptr_t;

} //namespace libgs::coro::detail


#endif //LIBGS_CORO_DETAIL_WAKE_UP_H