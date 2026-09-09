// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "interface.h"
#include <libgs/core/lock_free_queue.h>
#include <libgs/utils/signal_slot.h>
#include <libgs/utils/logger.h>

namespace libgs::utils::sbus { namespace detail
{

using payload_buffer_t = std::vector<std::byte>;
using shared_payload_t = std::shared_ptr<const payload_buffer_t>;

class payload_t
{
public:
	payload_t(const void *data, size_t size) :
		m_shared(false)
	{
		if( size == 0 )
			new (&m_storage.owned) payload_buffer_t();
		else
		{
			auto begin = static_cast<const std::byte*>(data);
			new (&m_storage.owned) payload_buffer_t(begin, begin + size);
		}
	}

	explicit payload_t(shared_payload_t payload) noexcept :
		m_shared(true)
	{
		new (&m_storage.shared) shared_payload_t(std::move(payload));
	}

	payload_t(const payload_t &other) : m_shared(other.m_shared)
	{
		if( m_shared )
			new (&m_storage.shared) shared_payload_t(other.m_storage.shared);
		else
			new (&m_storage.owned) payload_buffer_t(other.m_storage.owned);
	}

	payload_t(payload_t &&other) noexcept : m_shared(other.m_shared)
	{
		if( m_shared )
			new (&m_storage.shared) shared_payload_t(std::move(other.m_storage.shared));
		else
			new (&m_storage.owned) payload_buffer_t(std::move(other.m_storage.owned));
	}

	~payload_t()
	{
		if( m_shared )
			m_storage.shared.~shared_payload_t();
		else
			m_storage.owned.~payload_buffer_t();
	}

	[[nodiscard]] const std::byte *data() const noexcept {
		return m_shared ? m_storage.shared->data() : m_storage.owned.data();
	}

	[[nodiscard]] size_t size() const noexcept {
		return m_shared ? m_storage.shared->size() : m_storage.owned.size();
	}

private:
	union storage_t
	{
		storage_t() noexcept {}
		~storage_t() {}

		payload_buffer_t owned;
		shared_payload_t shared;
	}
	m_storage;
	bool m_shared;
};

static_assert(sizeof(payload_t) <= sizeof(payload_buffer_t) + sizeof(void*));

constexpr size_t g_queue_max_size = 128;
constexpr size_t g_shared_payload_threshold = 64 * 1'024;

[[noreturn]] static void uncaught_exception(const std::exception &ex) noexcept
{
	libgs_utils_clog_critical("LibGS.Utils", "Uncaught exception: {}", ex);
	forced_termination();
}

namespace
{

class /* LIBGS_DECL_HIDDEN */ subscriber_thread
{
	LIBGS_DISABLE_COPY_MOVE(subscriber_thread)

protected:
	subscriber_thread() = default;

	void start(std::function<void()> task_arg)
	{
		m_run.store(true, std::memory_order_release);
		m_thread = std::thread([this, task = std::move(task_arg)]() mutable noexcept
		{
			try {
				do_task(task);
			}
			catch(const std::exception &ex) {
				uncaught_exception(ex);
			}
		});
	}

	void notify() noexcept
	{
		// A monotonic generation cannot be cleared over a concurrent enqueue.
		m_epoch.fetch_add(1, std::memory_order_release);
		std::atomic_notify_one(&m_epoch);
	}

	void stop() noexcept
	{
		m_run.store(false, std::memory_order_release);
		notify();
		if( m_thread.joinable() )
			m_thread.join();
	}

public:
	virtual ~subscriber_thread() {
		stop();
	}

private:
	void do_task(const std::function<void()> &task)
	{
		uint64_t observed_epoch = 0;
		while( m_run.load(std::memory_order_acquire) )
		{
			while( m_epoch.load(std::memory_order_acquire) == observed_epoch )
			{
				std::atomic_wait_explicit (
					&m_epoch, observed_epoch, std::memory_order_acquire
				);
			}
			if( not m_run.load(std::memory_order_acquire) )
				break;
			do {
				observed_epoch = m_epoch.load(std::memory_order_acquire);
				task();
			}
			while( m_epoch.load(std::memory_order_acquire) != observed_epoch and
				m_run.load(std::memory_order_acquire) );
		}
	}

	alignas(64) std::atomic_uint64_t m_epoch {0};
	alignas(64) std::atomic_bool m_run {false};
	/*
	 * The support for std::jthread by clang requires at least version 20.
	 * So, it is still advisable to use the traditional std::thread.
	 */
	std::thread m_thread {};
};

class /* LIBGS_DECL_HIDDEN */ global_subscriber : public subscriber_thread
{
	LIBGS_DISABLE_COPY_MOVE(global_subscriber)

	circular_lock_free_queue <
		std::pair<std::string,payload_t>, g_queue_max_size
	> m_queue {};

public:
	global_subscriber()
	{
		start([this]
		{
			while( auto event = m_queue.dequeue() )
				received(event->first, std::move(event->second));
		});
	}

	~global_subscriber() override {
		stop();
	}

	void tigger(std::string_view topic, const void *data, size_t size) noexcept
	{
		m_queue.force_emplace (
			std::make_pair(std::string(topic), payload_t(data, size))
		);
		notify();
	}

	void tigger(std::string_view topic, const shared_payload_t &payload) noexcept
	{
		m_queue.force_emplace(
			std::make_pair(std::string(topic), payload_t(payload))
		);
		notify();
	}

	signal<void(std::string_view,payload_t)> received;
};

using global_subscriber_ptr = std::shared_ptr<global_subscriber>;

class /* LIBGS_DECL_HIDDEN */ subscriber : public subscriber_thread
{
	LIBGS_DISABLE_COPY_MOVE(subscriber)
	circular_lock_free_queue<payload_t,g_queue_max_size> m_queue {};

public:
	subscriber()
	{
		start([this]
		{
			while( auto event = m_queue.dequeue() )
				received(std::move(*event));
		});
	}

	~subscriber() override {
		stop();
	}

	void tigger(const void *data, size_t size) noexcept
	{
		m_queue.force_emplace(data, size);
		notify();
	}

	void tigger(const shared_payload_t &payload) noexcept
	{
		m_queue.force_emplace(payload);
		notify();
	}

	signal<void(payload_t)> received;
};

using subscriber_ptr = std::shared_ptr<subscriber>;

}} //namespace detail

class LIBGS_DECL_HIDDEN local_interface::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	struct transparent_string_hash
	{
		using is_transparent = void;

		[[nodiscard]] size_t operator()(std::string_view value) const noexcept {
			return std::hash<std::string_view>{}(value);
		}

		[[nodiscard]] size_t operator()(const std::string &value) const noexcept {
			return operator()(std::string_view(value));
		}
	};

	using subscriber_map = std::unordered_map<uint64_t,detail::subscriber_ptr>;
	using topic_map = std::unordered_map<
		std::string, subscriber_map, transparent_string_hash, std::equal_to<>
	>;

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

	void global_broadcast(std::string_view topic, const void *data, size_t size) noexcept
	{
		std::shared_lock lock(m_global_subscribers_lock);
		if( m_global_subscribers.empty() )
			return ;

		for(auto &[id, subscriber] : m_global_subscribers)
			subscriber->tigger(topic, data, size);
	}

	void global_broadcast(std::string_view topic, const detail::shared_payload_t &payload) noexcept
	{
		std::shared_lock lock(m_global_subscribers_lock);
		for(auto &[id, subscriber] : m_global_subscribers)
			subscriber->tigger(topic, payload);
	}

	void broadcast(std::string_view topic, const void *data, size_t size) noexcept
	{
		std::shared_lock lock(m_subscribers_lock);
		auto it = m_subscribers.find(topic);
		if( it == m_subscribers.end() or it->second.empty() )
			return ;

		for(auto &[id, subscriber] : it->second)
			subscriber->tigger(data, size);
	}

	void broadcast(std::string_view topic, const detail::shared_payload_t &payload) noexcept
	{
		std::shared_lock lock(m_subscribers_lock);
		auto it = m_subscribers.find(topic);
		if( it == m_subscribers.end() )
			return ;

		for(auto &[id, subscriber] : it->second)
			subscriber->tigger(payload);
	}

	void broadcast_large(std::string_view topic, const void *data, size_t size) noexcept
	{
		std::shared_lock global_lock(m_global_subscribers_lock);
		std::shared_lock topic_lock(m_subscribers_lock);

		auto topic_it = m_subscribers.find(topic);
		const auto topic_subscriber_count = topic_it == m_subscribers.end() ?
			0 : topic_it->second.size();

		const auto count = m_global_subscribers.size() + topic_subscriber_count;
		if( count == 0 )
			return ;

		if( count == 1 )
		{
			if( not m_global_subscribers.empty() )
				m_global_subscribers.begin()->second->tigger(topic, data, size);
			else
				topic_it->second.begin()->second->tigger(data, size);
			return ;
		}
		auto begin = static_cast<const std::byte*>(data);

		detail::shared_payload_t payload =
			std::make_shared<detail::payload_buffer_t>(begin, begin + size);

		for(auto &[id, subscriber] : m_global_subscribers)
			subscriber->tigger(topic, payload);

		if( topic_it != m_subscribers.end() )
		{
			for(auto &[id, subscriber] : topic_it->second)
				subscriber->tigger(payload);
		}
	}

	[[nodiscard]] size_t subscriber_count(std::string_view topic) const noexcept
	{
		size_t result = 0;
		{
			std::shared_lock lock(m_global_subscribers_lock);
			result += m_global_subscribers.size();
		}
		{
			std::shared_lock lock(m_subscribers_lock);
			if( auto it = m_subscribers.find(topic); it != m_subscribers.end() )
				result += it->second.size();
		}
		return result;
	}

public:
	std::atomic_uint64_t m_id_seq {0};

	topic_map m_subscribers {};
	mutable spin_shared_mutex m_subscribers_lock {};

	std::unordered_map<uint64_t,
		detail::global_subscriber_ptr
	> m_global_subscribers {};

	mutable spin_shared_mutex m_global_subscribers_lock {};
};

static std::map<local_interface*,
	std::shared_ptr<local_interface>
> g_obj_map {};

static spin_shared_mutex m_objs_lock {};

local_interface::local_interface() :
	m_impl(std::make_unique<impl>())
{

}

local_interface::~local_interface() = default;

void local_interface::publish(std::string_view topic, const void *buffer, size_t size)
{
	std::shared_lock lock(m_objs_lock);
	if( size >= detail::g_shared_payload_threshold )
	{
		if( g_obj_map.size() == 1 )
		{
			g_obj_map.begin()->second->m_impl->broadcast_large(topic, buffer, size);
			return ;
		}
		size_t subscriber_count = 0;
		for(auto &obj : g_obj_map | std::views::values)
			subscriber_count += obj->m_impl->subscriber_count(topic);

		if( subscriber_count > 1 )
		{
			auto begin = static_cast<const std::byte*>(buffer);
			detail::shared_payload_t payload =
				std::make_shared<detail::payload_buffer_t>(begin, begin + size);

			for(auto &obj : g_obj_map | std::views::values)
			{
				obj->m_impl->global_broadcast(topic, payload);
				obj->m_impl->broadcast(topic, payload);
			}
			return ;
		}
	}
	for(auto &obj : g_obj_map | std::views::values)
	{
		obj->m_impl->global_broadcast(topic, buffer, size);
		obj->m_impl->broadcast(topic, buffer, size);
	}
}

uint64_t local_interface::subscribe(std::string_view topic, std::function<void(const void*, size_t)> callback)
{
	auto [id, subr] = m_impl->make_subscriber(topic);
	subr->received.connect (
	[func = std::move(callback)](const detail::payload_t &payload) {
		func(payload.data(), payload.size());
	});
	m_objs_lock.lock();
	g_obj_map.emplace(this, shared_from_this());
	m_objs_lock.unlock();
	return id;
}

uint64_t local_interface::subscribe(std::function<void(std::string_view topic, const void*, size_t)> callback)
{
	auto [id, subr] = m_impl->make_subscriber();
	subr->received.connect (
	[func = std::move(callback)](std::string_view topic, const detail::payload_t &payload) {
		func(topic, payload.data(), payload.size());
	});
	m_objs_lock.lock();
	g_obj_map.emplace(this, shared_from_this());
	m_objs_lock.unlock();
	return id;
}

void local_interface::cancel_topic(std::string_view topic)
{
	{
		std::unique_lock lock(m_impl->m_subscribers_lock);
		if( auto it = m_impl->m_subscribers.find(topic); it != m_impl->m_subscribers.end() )
			m_impl->m_subscribers.erase(it);
	}
	std::unique_lock objs_lock(m_objs_lock);
	std::shared_lock global_lock(m_impl->m_global_subscribers_lock);
	std::shared_lock topic_lock(m_impl->m_subscribers_lock);

	if( m_impl->m_global_subscribers.empty() and m_impl->m_subscribers.empty() )
		g_obj_map.erase(this);
}

void local_interface::cancel_sid(uint64_t sid)
{
	bool erased = false;
	{
		std::unique_lock lock(m_impl->m_global_subscribers_lock);
		erased = m_impl->m_global_subscribers.erase(sid) > 0;
	}
	if( not erased )
	{
		std::unique_lock lock(m_impl->m_subscribers_lock);
		for(auto it = m_impl->m_subscribers.begin(); it != m_impl->m_subscribers.end(); ++it)
		{
			if( it->second.erase(sid) == 0 )
				continue;

			erased = true;
			if( it->second.empty() )
				m_impl->m_subscribers.erase(it);
			break;
		}
	}
	if( not erased )
		return ;

	std::unique_lock objs_lock(m_objs_lock);
	std::shared_lock global_lock(m_impl->m_global_subscribers_lock);
	std::shared_lock topic_lock(m_impl->m_subscribers_lock);

	if( m_impl->m_global_subscribers.empty() and m_impl->m_subscribers.empty() )
		g_obj_map.erase(this);
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
