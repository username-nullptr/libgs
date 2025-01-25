
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

#ifndef LIBGS_CORE_CORO_DETAIL_WAKE_UP_H
#define LIBGS_CORE_CORO_DETAIL_WAKE_UP_H

#include <libgs/core/execution.h>

namespace libgs::detail
{

class LIBGS_CORE_VAPI co_lock_wake_up
{
	LIBGS_DISABLE_COPY_MOVE(co_lock_wake_up)

public:
	using handler_t = async_work<bool>::handler_t;

	co_lock_wake_up(const asio::any_io_executor &exec, handler_t handler) :
		m_handler(std::move(handler)), m_exec(exec) {}

	void operator()(bool success)
	{
		if( not m_is_valid )
			return ;
		m_is_valid = false;

		dispatch(m_exec, [
			success, handler = std::make_shared<handler_t>(std::move(m_handler))
		]() mutable {
			std::move(*handler)(success);
		});
	}

private:
	handler_t m_handler;
	asio::any_io_executor m_exec;
	std::atomic_bool m_is_valid {true};
};

using co_lock_wake_up_ptr = std::shared_ptr<co_lock_wake_up>;

} //namespace libgs::detail


#endif //LIBGS_CORE_CORO_DETAIL_WAKE_UP_H