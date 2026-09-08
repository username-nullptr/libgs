// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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

public:
	using mutex_t = Mutex;

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

private:
	mutex_t *m_mutex;
	bool m_owns = false;
};

} //namespace libgs::coro
#include <libgs/coro/detail/mutex.h>


#endif //LIBGS_CORO_MUTEX_H
