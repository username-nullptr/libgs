
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

inline spin_shared_mutex::~spin_shared_mutex()
{
#if 0
	if( m_read_count == 0 )
		return ;
	runtime_error::loc_throw (
		"libgs::spin_shared_mutex: Destruct a spin mutex that has not yet been unlock_shared."
	);
#endif
}

inline void spin_shared_mutex::lock()
{
	using namespace std::chrono;
	constexpr auto max_spin_duration = 64us;

	auto start = steady_clock::now();
	bool expected = false;

	while( not m_write_flag.compare_exchange_weak(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed) )
	{
		expected = false;
		if( steady_clock::now() - start < max_spin_duration )
			none_instruction();
		else
		{
			std::this_thread::yield();
			start = steady_clock::now();
		}
	}
	while( m_read_count.load(std::memory_order_relaxed) > 0 )
	{
		if( steady_clock::now() - start < max_spin_duration )
			none_instruction();
		else
		{
			std::this_thread::yield();
			start = steady_clock::now();
		}
	}
}

inline bool spin_shared_mutex::try_lock()
{
	if( m_write_flag.load(std::memory_order_acquire) or
		m_read_count.load(std::memory_order_acquire) > 0 )
		return false;

	bool expected = false;
	return m_write_flag.compare_exchange_strong(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed
	);
}

inline void spin_shared_mutex::unlock()
{
	m_write_flag.store(false, std::memory_order_relaxed);
}

inline void spin_shared_mutex::lock_shared()
{
	using namespace std::chrono;
	constexpr auto max_spin_duration = 64us;
	auto start = steady_clock::now();
	for(;;)
	{
		while( m_write_flag.load(std::memory_order_acquire) )
		{
			if( steady_clock::now() - start < max_spin_duration )
				none_instruction();
			else
			{
				std::this_thread::yield();
				start = steady_clock::now();
			}
		}
		m_read_count.fetch_add(1, std::memory_order_relaxed);
		if( m_write_flag.load(std::memory_order_acquire) )
		{
			m_read_count.fetch_sub(1, std::memory_order_relaxed);
			continue;
		}
		break;
	}
}

inline bool spin_shared_mutex::try_lock_shared()
{
	if( m_write_flag.load(std::memory_order_acquire) )
		return false;

	m_read_count.fetch_add(1, std::memory_order_relaxed);
	if( m_write_flag.load(std::memory_order_acquire) )
	{
		m_read_count.fetch_sub(1, std::memory_order_relaxed);
		return false;
	}
	return true;
}

inline void spin_shared_mutex::unlock_shared()
{
	m_read_count.fetch_sub(1, std::memory_order_relaxed);
}

} //namesapace libgs


#endif //LIBGS_CORE_DETAIL_SHARED_MUTEX_H
