
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

#ifndef LIBGS_CORE_DETAIL_SHARED_MUTEX_H
#define LIBGS_CORE_DETAIL_SHARED_MUTEX_H

#include <libgs/core/spin_mutex.h>
#include <shared_mutex>

namespace libgs
{

inline spin_shared_mutex::~spin_shared_mutex() noexcept(false)
{
	if( m_read_count == 0 )
		return ;
	throw runtime_error (
		"libgs::spin_shared_mutex: Destruct a spin mutex that has not yet been unlock_shared."
	);
}

inline void spin_shared_mutex::lock()
{
	m_native_handle.lock();
}

inline bool spin_shared_mutex::try_lock()
{
	return m_native_handle.try_lock();
}

inline void spin_shared_mutex::unlock()
{
	m_native_handle.unlock();
}

inline void spin_shared_mutex::lock_shared()
{
	if( ++m_read_count == 1 )
		m_native_handle.lock();
}

inline bool spin_shared_mutex::try_lock_shared()
{
	if( ++m_read_count == 1 )
		return m_native_handle.try_lock();
	return true;
}

inline void spin_shared_mutex::unlock_shared()
{
	auto counter = m_read_count.load();
	if( counter == 0 )
		return m_native_handle.unlock();
	/*
		if( m_counter == counter )
		{
			m_counter = counter - 1;
			return true;
		}
		else
		{
			counter = m_counter;
			return false;
		}
	*/
	m_read_count.compare_exchange_weak(counter, counter - 1);
}

inline spin_shared_mutex::native_handle_t &spin_shared_mutex::native_handle() noexcept
{
	return m_native_handle;
}

} //namesapace libgs


#endif //LIBGS_CORE_DETAIL_SHARED_MUTEX_H
