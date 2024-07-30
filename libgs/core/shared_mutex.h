
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

#ifndef LIBGS_CORE_SHARED_MUTEX_H
#define LIBGS_CORE_SHARED_MUTEX_H

#include <libgs/core/spin_mutex.h>
#include <shared_mutex>
#include <mutex>

namespace libgs
{

using shared_mutex = std::shared_mutex;
using shared_timed_mutex = std::shared_timed_mutex;

using shared_shared_lock = std::shared_lock<shared_mutex>;
using shared_shared_timed_lock = std::shared_lock<shared_timed_mutex>;

using shared_unique_lock = std::unique_lock<shared_mutex>;
using shared_unique_timed_lock = std::unique_lock<shared_timed_mutex>;

class LIBGS_CORE_VAPI spin_shared_mutex
{
	LIBGS_DISABLE_COPY_MOVE(spin_shared_mutex)

public:
	using native_handle_t = spin_mutex;

public:
	spin_shared_mutex() = default;
	~spin_shared_mutex() noexcept(false);

public:
	void lock();
	[[nodiscard]] bool try_lock();
	void unlock();

public:
	void lock_shared();
	[[nodiscard]] bool try_lock_shared();
	void unlock_shared();

public:
	native_handle_t &native_handle() noexcept;

private:
	std::atomic_uint m_read_count {0};
	native_handle_t m_native_handle;
};

using spin_shared_shared_lock = std::shared_lock<spin_shared_mutex>;
using spin_shared_unique_lock = std::unique_lock<spin_shared_mutex>;

} //namesapace libgs
#include <libgs/core/detail/shared_mutex.h>


#endif //LIBGS_CORE_SHARED_MUTEX_H
