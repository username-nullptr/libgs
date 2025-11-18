
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

#ifndef LIBGS_CORE_DETAIL_SPIN_MUTEX_H
#define LIBGS_CORE_DETAIL_SPIN_MUTEX_H

namespace libgs
{

inline spin_mutex::~spin_mutex()
{
#if 0
	if( m_native_handle )
	{
		throw runtime_error (
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

inline void spin_mutex::none_instruction() noexcept
{
#ifdef _MSC_VER
	__asm pause;
#elif defined(__GNUC__)
# if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
	asm volatile("pause" : : : "memory");
# elif defined(__aarch64__) || defined(__arm64__) || defined(__arm__)
	asm volatile("yield" : : : "memory");
# elif defined(__powerpc__) || defined(__ppc__)
	asm volatile("or 0, 0, 0" : : : "memory");
# elif defined(__riscv)
	asm volatile("wfi" : : : "memory");
# else // Unknown
	asm volatile("" : : : "memory");
# endif // CPU Architecture
#endif //_MSC_VER
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_SPIN_MUTEX_H