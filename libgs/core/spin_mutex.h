
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

#ifndef LIBGS_CORE_SPIN_MUTEX_H
#define LIBGS_CORE_SPIN_MUTEX_H

#include <libgs/core/global.h>

namespace libgs
{

class LIBGS_CORE_VAPI spin_mutex
{
	LIBGS_DISABLE_COPY_MOVE(spin_mutex)

public:
	using native_handle_t = std::atomic_bool;

public:
	spin_mutex() = default;
	~spin_mutex() noexcept(false);

public:
	void lock();
	[[nodiscard]] bool try_lock();
	void unlock();

public:
	native_handle_t &native_handle() noexcept;

private:
	native_handle_t m_native_handle {false};
};

using spin_unique_lock = std::unique_lock<spin_mutex>;

} //namespace libgs
#include <libgs/core/detail/spin_mutex.h>


#endif //LIBGS_CORE_SPIN_MUTEX_H