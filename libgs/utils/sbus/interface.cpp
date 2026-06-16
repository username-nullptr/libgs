
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#include "interface.h"

#include <libgs/core/lock_free_queue.h>
#include <libgs/core/execution.h>

#include <libgs/utils/signal_slot.h>
#include <libgs/utils/logger.h>

namespace libgs::utils::sbus { namespace detail
{

using payload_t = std::vector<std::byte>;
constexpr size_t g_queue_max_size = 128;

[[noreturn]] static void uncaught_exception(const std::exception &ex) noexcept
{
	libgs_utils_clog_critical("LibGS.Utils", "Uncaught exception: {}", ex);
	forced_termination();
}

template <typename Derived>
class /* LIBGS_DECL_HIDDEN */ subscriber_thread : public std::enable_shared_from_this<Derived>
{
	LIBGS_DISABLE_COPY_MOVE(subscriber_thread)

protected:
	explicit subscriber_thread
	(std::function<awaitable<bool>()> task) :
		m_thread([this]() mutable noexcept { libgs::exec(m_exec); })
	{
		libgs::dispatch(m_exec,
		[this, task = std::move(task)]() mutable noexcept -> awaitable<void>
		{
			try {
				co_await do_task(std::move(task));
			}
			catch(const std::exception &ex) {
				uncaught_exception(ex);
			}
			co_return ;
		});
	}

	void notify() noexcept
	{
		m_flag.store(true, std::memory_order_relaxed);
		std::atomic_notify_one(&m_flag);
	}

	[[nodiscard]] asio::io_context &exec() noexcept {
		return m_exec;
	}

public:
	virtual ~subscriber_thread()
	{
		m_run = false;
		notify();
		if( m_thread.joinable() )
			m_thread.join();
	}

private:
	[[nodiscard]] awaitable<void> do_task(std::function<awaitable<bool>()> task)
	{
		while( m_run )
		{
			while( not m_flag.load(std::memory_order_relaxed) )
				std::atomic_wait(&m_flag, false);

			if( not co_await task() )
				m_flag.store(false, std::memory_order_relaxed);

			if( not m_run )
				break;
		}
		co_return ;
	}

	alignas(64) std::atomic_bool m_flag {false};
	alignas(64) std::atomic_bool m_run {true};
	/*
	 * The support for std::jthread by clang requires at least version 20.
	 * So, it is still advisable to use the traditional std::thread.
	 */
	asio::io_context m_exec {};
	std::thread m_thread {};
};

class /* LIBGS_DECL_HIDDEN */ global_subscriber : public subscriber_thread<global_subscriber>
{
	LIBGS_DISABLE_COPY_MOVE(global_subscriber)

	circular_lock_free_queue <
		std::pair<std::string,payload_t>, g_queue_max_size
	> m_queue {};

public:
	global_subscriber() :
	subscriber_thread([this]() -> awaitable<bool>
	{
		using opt_t = decltype(libgs::dispatch (
			exec(), std::declval<std::function<awaitable<void>()>>(),
			deferred
		));
		std::vector<opt_t> tasks {};
		for(;;)
		{
			while( auto event = m_queue.dequeue() )
			{
				std::function emit =
						[this, key = std::move(event->first), value = std::move(event->second)]
				() mutable -> awaitable<void> {
					co_return co_await received(std::move(key), std::move(value));
				};
				tasks.emplace_back (
					libgs::dispatch(exec(), std::move(emit), deferred)
				);
			}
			if( tasks.empty() )
				co_return false;

			auto [unused, exs] = co_await asio::experimental::make_parallel_group(std::move(tasks))
				.async_wait(asio::experimental::wait_for_all(), use_awaitable);

			std::exception_ptr first_ex {};
			for(auto &ex : exs)
			{
				if( not ex )
					continue;
				else if( first_ex )
					throw asio::multiple_exceptions(first_ex);
				first_ex = ex;
			}
			if( first_ex )
				std::rethrow_exception(first_ex);
		}
		co_return true;
	}) {}

	void tigger(std::string_view topic, const void *data, size_t size) noexcept
	{
		std::span view {
			static_cast<const std::byte*>(data), size
		};
		m_queue.force_emplace (
			std::make_pair(std::string(topic),
				payload_t{ view.begin(), view.end() }
			)
		);
		notify();
	}

	signal<awaitable<void>(
		std::string_view, payload_t
	)> received;
};

using global_subscriber_ptr = std::shared_ptr<global_subscriber>;

class /* LIBGS_DECL_HIDDEN */ subscriber : public subscriber_thread<subscriber>
{
	LIBGS_DISABLE_COPY_MOVE(subscriber)
	circular_lock_free_queue<payload_t,g_queue_max_size> m_queue {};

public:
	subscriber() :
	subscriber_thread([this]() -> awaitable<bool>
	{
		using opt_t = decltype(libgs::dispatch (
			exec(), std::declval<std::function<awaitable<void>()>>(),
			deferred
		));
		std::vector<opt_t> tasks {};
		for(;;)
		{
			while( auto event = m_queue.dequeue() )
			{
				std::function emit =
						[this, value = std::move(*event)]() mutable -> awaitable<void> {
					co_return co_await received(std::move(value));
				};
				tasks.emplace_back (
					libgs::dispatch(exec(), std::move(emit), deferred)
				);
			}
			if( tasks.empty() )
				co_return false;

			auto [unused, exs] = co_await asio::experimental::make_parallel_group(std::move(tasks))
				.async_wait(asio::experimental::wait_for_all(), use_awaitable);

			std::exception_ptr first_ex {};
			for(auto &ex : exs)
			{
				if( not ex )
					continue;
				else if( first_ex )
					throw asio::multiple_exceptions(first_ex);
				first_ex = ex;
			}
			if( first_ex )
				std::rethrow_exception(first_ex);
		}
		co_return true;
	}) {}

	void tigger(const void *data, size_t size) noexcept
	{
		std::span view {
			static_cast<const std::byte*>(data), size
		};
		m_queue.force_emplace(view.begin(), view.end());
		notify();
	}

	signal<awaitable<void>(payload_t)> received;
};

using subscriber_ptr = std::shared_ptr<subscriber>;

} //namespace detail

class LIBGS_DECL_HIDDEN local_interface::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;

	[[nodiscard]] std::pair<uint64_t,detail::subscriber_ptr>
	make_subscriber(std::string_view topic) noexcept
	{
		auto id = m_id_seq++;
		auto obj = std::make_shared<detail::subscriber>();
		std::unique_lock lock(m_subscribers_lock);

		auto it = m_subscribers.emplace (
			std::string(topic), std::unordered_map<uint64_t,detail::subscriber_ptr>()
		);
		it.first->second.emplace(id, obj);
		return { id, obj };
	}

	[[nodiscard]] std::pair<uint64_t,detail::global_subscriber_ptr> make_subscriber() noexcept
	{
		auto id = m_id_seq++;
		auto obj = std::make_shared<detail::global_subscriber>();
		std::unique_lock lock(m_global_subscribers_lock);
		m_global_subscribers.emplace(id, obj);
		return { id, obj };
	}

	static void global_broadcast(auto &subscribers, std::string_view topic, const void *data, size_t size) noexcept
	{
		if( subscribers.empty() )
			return ;

		for(auto &[id, subscriber] : subscribers)
			subscriber->tigger(topic, data, size);
	}

	static void broadcast(auto &subscribers, std::string_view topic, const void *data, size_t size) noexcept
	{
		auto it = subscribers.find(std::string(topic));
		if( it == subscribers.end() or it->second.empty() )
			return ;

		for(auto &[id, subscriber] : it->second)
			subscriber->tigger(data, size);
	}

public:
	std::atomic_uint64_t m_id_seq {0};

	std::unordered_map <
		std::string, std::unordered_map <
			uint64_t, detail::subscriber_ptr
		>
	> m_subscribers {};
	spin_shared_mutex m_subscribers_lock {};

	std::unordered_map<uint64_t,
		detail::global_subscriber_ptr
	> m_global_subscribers {};
	spin_shared_mutex m_global_subscribers_lock {};
};

static std::map<local_interface*,
	std::shared_ptr<local_interface>
> g_obj_map {};

static spin_shared_mutex m_objs_lock {};

local_interface::local_interface() :
	m_impl(std::make_unique<impl>())
{

}

local_interface::~local_interface()
{

}

void local_interface::publish(std::string_view topic, const void *buffer, size_t size)
{
	m_objs_lock.lock_shared();
	auto objs = g_obj_map;
	m_objs_lock.unlock_shared();

	for(auto &obj : objs | std::views::values)
	{
		obj->m_impl->m_global_subscribers_lock.lock_shared();
		auto glob_map = obj->m_impl->m_global_subscribers;
		obj->m_impl->m_global_subscribers_lock.unlock_shared();

		obj->m_impl->m_subscribers_lock.lock_shared();
		auto map = obj->m_impl->m_subscribers;
		obj->m_impl->m_subscribers_lock.unlock_shared();

		if( map.empty() )
			return obj->m_impl->global_broadcast(glob_map, topic, buffer, size);

		obj->m_impl->global_broadcast(glob_map, topic, buffer, size);
		obj->m_impl->broadcast(map, topic, buffer, size);
	}
}

uint64_t local_interface::subscribe(std::string_view topic, std::function<void(const void*, size_t)> func)
{
	auto [id, subr] = m_impl->make_subscriber(topic);
	subr->received.connect (
	[func = std::move(func)](const detail::payload_t &payload) {
		func(payload.data(), payload.size());
	});
	m_objs_lock.lock();
	g_obj_map.emplace(this, shared_from_this());
	m_objs_lock.unlock();
	return id;
}

uint64_t local_interface::subscribe(std::function<void(std::string_view topic, const void*, size_t)> func)
{
	auto [id, subr] = m_impl->make_subscriber();
	subr->received.connect (
	[func = std::move(func)](std::string_view topic, const detail::payload_t &payload) {
		func(topic, payload.data(), payload.size());
	});
	m_objs_lock.lock();
	g_obj_map.emplace(this, shared_from_this());
	m_objs_lock.unlock();
	return id;
}

void local_interface::cancel_topic(std::string_view topic)
{
	m_impl->m_subscribers_lock.lock();
	m_impl->m_subscribers.erase(std::string(topic));
	auto size = m_impl->m_subscribers.size();
	m_impl->m_subscribers_lock.unlock();

	m_impl->m_global_subscribers_lock.lock_shared();
	size += m_impl->m_global_subscribers.size();
	m_impl->m_global_subscribers_lock.unlock_shared();

	if( size > 0 )
		return ;

	m_objs_lock.lock();
	g_obj_map.erase(this);
	m_objs_lock.unlock();
}

void local_interface::cancel_sid(uint64_t sid)
{
	size_t size = 0;
	{
		m_impl->m_global_subscribers_lock.lock();
		bool erased = !!m_impl->m_global_subscribers.erase(sid);
		size = m_impl->m_global_subscribers.size();
		m_impl->m_global_subscribers_lock.unlock();

		if( erased )
		{
			if( size > 0 )
				return ;

			m_objs_lock.lock();
			g_obj_map.erase(this);
			m_objs_lock.unlock();
			return ;
		}
	}
	m_impl->m_subscribers_lock.lock();
	for(auto &map : m_impl->m_subscribers | std::views::values)
	{
		if( !!map.erase(sid) )
		{
			size += m_impl->m_subscribers.size();
			break;
		}
	}
	m_impl->m_subscribers_lock.unlock();
	if( size > 0 )
		return ;

	m_objs_lock.lock();
	g_obj_map.erase(this);
	m_objs_lock.unlock();
}

void local_interface::cancel()
{
	m_impl->m_global_subscribers_lock.lock();
	m_impl->m_global_subscribers.clear();
	m_impl->m_global_subscribers_lock.unlock();

	m_impl->m_subscribers_lock.lock();
	m_impl->m_subscribers.clear();
	m_impl->m_subscribers_lock.unlock();

	m_objs_lock.lock();
	g_obj_map.erase(this);
	m_objs_lock.unlock();
}

} //namespace libgs::utils::sbus