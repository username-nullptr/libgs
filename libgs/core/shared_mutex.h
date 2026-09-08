// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
	spin_shared_mutex() = default;
	~spin_shared_mutex();

public:
	void lock();
	[[nodiscard]] bool try_lock();
	void unlock();

public:
	void lock_shared();
	[[nodiscard]] bool try_lock_shared();
	void unlock_shared();

private:
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4324)
#endif
	alignas(64) std::atomic_bool m_write_flag {false};
	alignas(64) std::atomic_size_t m_read_count {0};
#ifdef _MSC_VER
# pragma warning(pop)
#endif
};

using spin_shared_shared_lock = std::shared_lock<spin_shared_mutex>;
using spin_shared_unique_lock = std::unique_lock<spin_shared_mutex>;

} //namesapace libgs
#include <libgs/core/detail/shared_mutex.h>


#endif //LIBGS_CORE_SHARED_MUTEX_H
