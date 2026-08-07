
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORO_MUTEX_H
#define LIBGS_CORO_MUTEX_H

#include <libgs/coro/global.h>

namespace libgs::coro
{

class LIBGS_CORO_VAPI mutex
{
	LIBGS_DISABLE_COPY_MOVE(mutex)

public:
	using native_handle_t = std::atomic_bool;

public:
	mutex();
	~mutex();

public:
	[[nodiscard]] awaitable<void> lock(concepts::sched auto &&exec);
	[[nodiscard]] awaitable<void> lock();

	[[nodiscard]] bool try_lock();
	void unlock();

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
	[[nodiscard]] bool is_locked() const noexcept;
	[[nodiscard]] native_handle_t &native_handle() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <typename Mutex = mutex>
class LIBGS_CORO_TAPI unique_lock
{
	LIBGS_DISABLE_COPY(unique_lock)

public: using mutex_t = Mutex;
private: mutex_t *m_mutex;

public:
	explicit unique_lock(mutex_t &mutex);
	~unique_lock() noexcept(noexcept(m_mutex->unlock()));

	unique_lock(unique_lock &&other) noexcept;
	unique_lock &operator=(unique_lock &&other) noexcept;

public:
	[[nodiscard]] awaitable<void> lock(concepts::sched auto &&exec);
	[[nodiscard]] awaitable<void> lock();

	[[nodiscard]] bool try_lock();
	void unlock();

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
	[[nodiscard]] bool is_locked() const noexcept;
	[[nodiscard]] mutex_t *mutex() noexcept;
};

} //namespace libgs::coro
#include <libgs/coro/detail/mutex.h>


#endif //LIBGS_CORO_MUTEX_H
