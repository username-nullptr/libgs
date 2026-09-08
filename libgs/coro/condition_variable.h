// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORO_CONDITION_VARIABLE_H
#define LIBGS_CORO_CONDITION_VARIABLE_H

#include <libgs/coro/mutex.h>

namespace libgs::coro
{

class LIBGS_CORO_VAPI condition_variable
{
	LIBGS_DISABLE_COPY_MOVE(condition_variable)

public:
	condition_variable();
	~condition_variable();

public:
	template <typename Mutex>
	[[nodiscard]] awaitable<void> wait(unique_lock<Mutex> &lock) noexcept;

	template <typename Mutex>
	[[nodiscard]] awaitable<void> wait(unique_lock<Mutex> &lock, auto pred);

	template <typename Mutex>
	[[nodiscard]] awaitable<void> wait (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock
	) noexcept;

	template <typename Mutex>
	[[nodiscard]] awaitable<void> wait (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock, auto pred
	);

	void notify_one() noexcept;
	void notify_all() noexcept;

public:
	template <typename Mutex, typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> wait_for (
		unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime
	);
	template <typename Mutex, typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> wait_for (
		unique_lock<Mutex> &lock, const duration<Rep,Period> &rtime, auto pred
	);
	template <typename Mutex, typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> wait_until (
		unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime
	);
	template <typename Mutex, typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> wait_until (
		unique_lock<Mutex> &lock, const time_point<Clock,Duration> &atime, auto pred
	);

public:
	template <typename Mutex, typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> wait_for (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock,
		const duration<Rep,Period> &rtime
	);
	template <typename Mutex, typename Rep, typename Period>
	[[nodiscard]] awaitable<bool> wait_for (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock,
		const duration<Rep,Period> &rtime, auto pred
	);
	template <typename Mutex, typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> wait_until (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock,
		const time_point<Clock,Duration> &atime
	);
	template <typename Mutex, typename Clock, typename Duration>
	[[nodiscard]] awaitable<bool> wait_until (
		concepts::sched auto &&exec, unique_lock<Mutex> &lock,
		const time_point<Clock,Duration> &atime, auto pred
	);

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::coro
#include <libgs/coro/detail/condition_variable.h>


#endif //LIBGS_CORO_CONDITION_VARIABLE_H
