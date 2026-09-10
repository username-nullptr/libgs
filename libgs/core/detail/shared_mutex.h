// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
	constexpr size_t spin_count = 64;
	for(;;)
	{
		bool expected = false;
		if( m_write_flag.compare_exchange_weak(expected, true,
			std::memory_order_acquire, std::memory_order_relaxed) )
			break;

		for(size_t count = 0; count < spin_count; ++count)
		{
			if( not m_write_flag.load(std::memory_order_relaxed) )
				break;
			none_instruction();
		}
		if( m_write_flag.load(std::memory_order_relaxed) )
			m_write_flag.wait(true, std::memory_order_relaxed);
	}
	for(auto readers = m_read_count.load(std::memory_order_acquire); readers > 0;
		readers = m_read_count.load(std::memory_order_acquire))
	{
		for(size_t count = 0; count < spin_count; ++count)
		{
			if( m_read_count.load(std::memory_order_relaxed) == 0 )
				break;
			none_instruction();
		}
		readers = m_read_count.load(std::memory_order_acquire);
		if( readers > 0 )
			m_read_count.wait(readers, std::memory_order_relaxed);
	}
}

inline bool spin_shared_mutex::try_lock()
{
	bool expected = false;
	if( not m_write_flag.compare_exchange_strong(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed) )
		return false;

	if( m_read_count.load(std::memory_order_acquire) == 0 )
		return true;

	m_write_flag.store(false, std::memory_order_release);
	m_write_flag.notify_all();
	return false;
}

inline void spin_shared_mutex::unlock()
{
	m_write_flag.store(false, std::memory_order_release);
	m_write_flag.notify_all();
}

inline void spin_shared_mutex::lock_shared()
{
	constexpr size_t spin_count = 64;
	for(;;)
	{
		while( m_write_flag.load(std::memory_order_acquire) )
		{
			for(size_t count = 0; count < spin_count; ++count)
			{
				if( not m_write_flag.load(std::memory_order_relaxed) )
					break;
				none_instruction();
			}
			if( m_write_flag.load(std::memory_order_relaxed) )
				m_write_flag.wait(true, std::memory_order_relaxed);
		}
		m_read_count.fetch_add(1, std::memory_order_relaxed);
		if( m_write_flag.load(std::memory_order_acquire) )
		{
			if( m_read_count.fetch_sub(1, std::memory_order_release) == 1 )
				m_read_count.notify_all();
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
		if( m_read_count.fetch_sub(1, std::memory_order_release) == 1 )
			m_read_count.notify_all();
		return false;
	}
	return true;
}

inline void spin_shared_mutex::unlock_shared()
{
	if( m_read_count.fetch_sub(1, std::memory_order_release) == 1 )
		m_read_count.notify_all();
}

} //namesapace libgs


#endif //LIBGS_CORE_DETAIL_SHARED_MUTEX_H
