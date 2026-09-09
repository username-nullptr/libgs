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
	using namespace std::chrono;
	constexpr auto max_spin_duration = 64us;

	auto start = high_resolution_clock::now();
	bool expected = false;

	while( not m_native_handle.compare_exchange_weak(expected, true,
		std::memory_order_acquire, std::memory_order_relaxed) )
	{
		expected = false;
		if( high_resolution_clock::now() - start < max_spin_duration )
			none_instruction();
		else
		{
			std::this_thread::yield();
			start = high_resolution_clock::now();
		}
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
	m_native_handle.store(false, std::memory_order_relaxed);
}

inline spin_mutex::native_handle_t &spin_mutex::native_handle() noexcept
{
	return m_native_handle;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_SPIN_MUTEX_H
