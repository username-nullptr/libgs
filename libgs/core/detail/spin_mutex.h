
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

#ifndef LIBGS_CORE_DETAIL_SPIN_MUTEX_H
#define LIBGS_CORE_DETAIL_SPIN_MUTEX_H

namespace libgs
{

inline spin_mutex::~spin_mutex() noexcept(false)
{
	if( m_native_handle )
		throw runtime_error("libgs::spin_mutex: Destruct a spin mutex that has not yet been unlocked.");
}

inline void spin_mutex::lock()
{
	while( m_native_handle )
		std::this_thread::yield();
	m_native_handle = true;
}

inline bool spin_mutex::try_lock()
{
	bool flag = false;
	/*
		if( m_native_handle == flag )
	 	{
	 		m_native_handle = true;
	 		return true;
		}
	 	else
	 	{
	 		flag = m_native_handle;
			return false;
	 	}
	*/
	return m_native_handle.compare_exchange_weak(flag, true);
}

inline void spin_mutex::unlock()
{
	m_native_handle	= false;
}

inline typename spin_mutex::native_handle_t &spin_mutex::native_handle() noexcept
{
	return m_native_handle;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_SPIN_MUTEX_H
