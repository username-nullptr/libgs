// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
	~spin_mutex();

public:
	void lock();
	[[nodiscard]] bool try_lock();
	void unlock();

public:
	native_handle_t &native_handle() noexcept;

private:
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4324)
#endif
	alignas(64) native_handle_t m_native_handle {false};
#ifdef _MSC_VER
# pragma warning(pop)
#endif
};

using spin_unique_lock = std::unique_lock<spin_mutex>;

} //namespace libgs
#include <libgs/core/detail/spin_mutex.h>


#endif //LIBGS_CORE_SPIN_MUTEX_H
