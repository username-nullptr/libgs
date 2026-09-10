// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_DETAIL_SPIN_MUTEX_H
#define LIBGS_CORE_DETAIL_SPIN_MUTEX_H

#include <libgs/core/system/cpu.h>

namespace libgs
{

inline spin_mutex::~spin_mutex()
{
#if 0
	if( m_native_handle )
	{
		runtime_error::loc_throw (
			"libgs::spin_mutex: Destruct a spin mutex that has not yet been unlocked."
		);
	}
#endif
}

inline void spin_mutex::lock()
{
	constexpr size_t spin_count = 64;
	for(;;)
	{
		bool expected = false;
		if( m_native_handle.compare_exchange_weak(expected, true,
			std::memory_order_acquire, std::memory_order_relaxed) )
			return ;

		for(size_t count = 0; count < spin_count; ++count)
		{
			if( not m_native_handle.load(std::memory_order_relaxed) )
				break;
			none_instruction();
		}
		if( m_native_handle.load(std::memory_order_relaxed) )
			m_native_handle.wait(true, std::memory_order_relaxed);
	}
}

inline bool spin_mutex::try_lock()
{
	bool expected = false;
	/*
		if( m_native_handle == expected )
	 	{
	 		m_native_handle = true;
	 		return true;
		}
	 	else
	 	{
	 		expected = m_native_handle;
			return false;
	 	}
	*/
	return m_native_handle.compare_exchange_strong(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed
	);
}

inline void spin_mutex::unlock()
{
	m_native_handle.store(false, std::memory_order_release);
	m_native_handle.notify_one();
}

inline spin_mutex::native_handle_t &spin_mutex::native_handle() noexcept
{
	return m_native_handle;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_SPIN_MUTEX_H
