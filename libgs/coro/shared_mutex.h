
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

#ifndef LIBGS_CORO_SHARED_MUTEX_H
#define LIBGS_CORO_SHARED_MUTEX_H

#include <libgs/coro/mutex.h>

namespace libgs::coro
{

class LIBGS_CORE_VAPI shared_mutex
{
	LIBGS_DISABLE_COPY_MOVE(shared_mutex)

public:
	using native_handle_t = mutex;
	shared_mutex() = default;
	~shared_mutex();

public:
	[[nodiscard]] awaitable<void> lock(concepts::sched auto &&exec);
	[[nodiscard]] awaitable<void> lock();

	[[nodiscard]] bool try_lock();
	void unlock();

public:
	[[nodiscard]] awaitable<void> lock_shared(concepts::sched auto &&exec);
	[[nodiscard]] awaitable<void> lock_shared();

	[[nodiscard]] bool try_lock_shared();
	void unlock_shared();

public:
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_for (
		concepts::sched auto &&exec, const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_until (
		concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout
	);
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_for (
		const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_until (
		const time_point<Clock,Duration> &timeout
	);

public:
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_shared_for (
		concepts::sched auto &&exec, const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_shared_until (
		concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout
	);
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_shared_for (
		const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_shared_until (
		const time_point<Clock,Duration> &timeout
	);

public:
	[[nodiscard]] bool is_locked() const noexcept;
	[[nodiscard]] native_handle_t &native_handle() noexcept;

private:
	std::atomic_uint m_read_count {0};
	native_handle_t m_native_handle;
};

class LIBGS_CORE_VAPI shared_lock
{
	LIBGS_DISABLE_COPY(shared_lock)

public:
	using mutex_t = shared_mutex;
	explicit shared_lock(mutex_t &mutex);
	~shared_lock() noexcept(noexcept(m_mutex->unlock_shared()));

	shared_lock(shared_lock &&other) noexcept;
	shared_lock &operator=(shared_lock &&other) noexcept;

public:
	[[nodiscard]] awaitable<void> lock_shared(concepts::sched auto &&exec);
	[[nodiscard]] awaitable<void> lock_shared();

	[[nodiscard]] bool try_lock_shared();
	void unlock_shared();

public:
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_shared_for (
		concepts::sched auto &&exec, const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_shared_until (
		concepts::sched auto &&exec, const time_point<Clock,Duration> &timeout
	);
	template<typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> try_lock_shared_for (
		const duration<Rep,Period> &timeout
	);
	template<typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> try_lock_shared_until (
		const time_point<Clock,Duration> &timeout
	);

public:
	[[nodiscard]] bool is_locked() const noexcept;
	[[nodiscard]] mutex_t *mutex() noexcept;

private:
	mutex_t *m_mutex;
};

using shared_unique_lock = unique_lock<shared_mutex>;

} //namespace libgs::coro
#include <libgs/coro/detail/shared_mutex.h>


#endif //LIBGS_CORO_SHARED_MUTEX_H