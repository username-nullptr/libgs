// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
#define LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4324)
#endif //_MSC_VER

namespace libgs
{

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(element_t &&data)
{
	force_emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	force_emplace(data);
}

template <concepts::copy_or_move_constructible T, typename Derived>
template <typename...Args>
size_t lock_free_queue_base<T,Derived>::force_emplace(Args&&...args) requires
	concepts::constructible<element_t,Args...>
{
	size_t sum = 0;
	auto self = static_cast<Derived*>(this);
	for(;;)
	{
		if( self->full() )
		{
			self->dequeue();
			sum++;
		}
		if( self->emplace(std::forward<Args>(args)...) )
			break;
	}
	return sum;
}

namespace detail
{

template <concepts::copy_or_move_constructible, queue_type, size_t>
class lock_free_queue_impl;

template <concepts::copy_or_move_constructible T, size_t N> requires (N > 0)
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::linked, N>
{
LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)
public: lock_free_queue_impl() = default;
};

template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::linked, 0>
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)

public:
	explicit lock_free_queue_impl
	(size_t capacity = std::numeric_limits<size_t>::max()) {
		m_capacity = capacity > 0 ? capacity : std::numeric_limits<size_t>::max();
	}
	std::atomic_size_t m_capacity {};
};

} //namespace detail

template <concepts::copy_or_move_constructible T, size_t N>
class LIBGS_CORE_TAPI lock_free_queue<T,queue_type::linked,N>::impl :
	public detail::lock_free_queue_impl<T,queue_type::linked,N>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	struct node
	{
		node() = default;

		template <typename...Args>
		explicit node(std::in_place_t, Args&&...args) {
			data.emplace(std::forward<Args>(args)...);
		}
		optional<element_t> data {};
		std::atomic<node*> next {nullptr};
		node *retired_next = nullptr;
	};

	struct hazard_record
	{
		std::atomic_bool active {false};
		std::atomic<node*> pointers[2] {};
		hazard_record *next = nullptr;

		node *retired = nullptr;
		node *available = nullptr;
		size_t retired_count = 0;
	};

	class hazard_guard
	{
		LIBGS_DISABLE_COPY_MOVE(hazard_guard)

	public:
		explicit hazard_guard(impl *owner, std::atomic<hazard_record*> &hint) :
			m_owner(owner), m_record(owner->acquire_hazard_record(hint)) {}

		~hazard_guard()
		{
			clear(0);
			clear(1);
			m_record->active.store(false, std::memory_order_release);
		}

		[[nodiscard]] node *protect(size_t index, const std::atomic<node*> &source)
		{
			node *protected_node = nullptr;
			do {
				protected_node = source.load(std::memory_order_acquire);
				m_record->pointers[index].store(protected_node, std::memory_order_release);
			}
			while( protected_node != source.load(std::memory_order_acquire) );
			return protected_node;
		}

		void clear(size_t index) noexcept {
			m_record->pointers[index].store(nullptr, std::memory_order_release);
		}

		void retire(node *retired_node) {
			m_owner->retire(m_record, retired_node);
		}

		template <typename...Args>
		[[nodiscard]] node *make_node(Args&&...args) {
			return m_owner->make_node(m_record, std::forward<Args>(args)...);
		}

	private:
		impl *m_owner;
		hazard_record *m_record;
	};

public:
	explicit impl(size_t capacity = std::numeric_limits<size_t>::max())
		requires (capacity_v == 0) :
		detail::lock_free_queue_impl<T,queue_type::linked,N>(capacity)
	{
		auto dummy = new node();
		m_head.store(dummy, std::memory_order_release);
		m_tail.store(dummy, std::memory_order_release);
	}

	impl() requires (capacity_v > 0)
	{
		auto dummy = new node();
		m_head.store(dummy, std::memory_order_release);
		m_tail.store(dummy, std::memory_order_release);
	}

	~impl()
	{
		auto current = m_head.load(std::memory_order_relaxed);
		while( current )
		{
			auto next = current->next.load(std::memory_order_relaxed);
			delete current;
			current = next;
		}
		auto record = m_hazard_records.load(std::memory_order_relaxed);
		while( record )
		{
			current = record->retired;
			while( current )
			{
				auto next = current->retired_next;
				delete current;
				current = next;
			}
			current = record->available;
			while( current )
			{
				auto next = current->retired_next;
				delete current;
				current = next;
			}
			auto next = record->next;
			delete record;
			record = next;
		}
		current = m_recycled.exchange(nullptr, std::memory_order_relaxed);
		while( current )
		{
			auto next = current->retired_next;
			delete current;
			current = next;
		}
	}

private:
	[[nodiscard]] hazard_record *acquire_hazard_record(std::atomic<hazard_record*> &hint)
	{
		if( auto record = hint.load(std::memory_order_acquire) )
		{
			if( bool expected = false;
				record->active.compare_exchange_strong(expected, true,
					std::memory_order_acq_rel, std::memory_order_relaxed) )
				return record;
		}
		for(auto record=m_hazard_records.load(std::memory_order_acquire);
			record; record=record->next)
		{
			if( bool expected = false;
				record->active.compare_exchange_strong(expected, true,
					std::memory_order_acq_rel, std::memory_order_relaxed) )
			{
				hint.store(record, std::memory_order_release);
				return record;
			}
		}
		auto new_record = std::make_unique<hazard_record>();
		new_record->active.store(true, std::memory_order_relaxed);

		auto head = m_hazard_records.load(std::memory_order_relaxed);
		do {
			new_record->next = head;
		}
		while( not m_hazard_records.compare_exchange_weak(head, new_record.get(),
			   std::memory_order_release, std::memory_order_relaxed) );

		auto result = new_record.release();
		hint.store(result, std::memory_order_release);
		return result;
	}

	[[nodiscard]] bool is_hazard(const node *target_node) const noexcept
	{
		for(auto record=m_hazard_records.load(std::memory_order_acquire);
			record; record=record->next)
		{
			for(auto &pointer : record->pointers)
			{
				if( pointer.load(std::memory_order_acquire) == target_node )
					return true;
			}
		}
		return false;
	}

	void recycle(node *first, node *last, size_t count) noexcept
	{
		if( count > recycled_node_capacity )
		{
			while( first )
			{
				auto next = first->retired_next;
				delete first;
				first = next;
			}
			return ;
		}
		auto pooled = m_recycled_count.fetch_add(count, std::memory_order_relaxed);
		if( pooled > recycled_node_capacity - count )
		{
			m_recycled_count.fetch_sub(count, std::memory_order_relaxed);
			while( first )
			{
				auto next = first->retired_next;
				delete first;
				first = next;
			}
			return ;
		}
		auto head = m_recycled.load(std::memory_order_relaxed);
		do {
			last->retired_next = head;
		}
		while( not m_recycled.compare_exchange_weak(head, first,
			std::memory_order_release, std::memory_order_relaxed) );
	}

	template <typename...Args>
	[[nodiscard]] node *make_node(hazard_record *record, Args&&...args)
	{
		auto result = record->available;
		if( result )
			record->available = result->retired_next;
		else
		{
			result = m_recycled.exchange(nullptr, std::memory_order_acq_rel);
			if( not result )
				return new node(std::in_place, std::forward<Args>(args)...);

			size_t recycled_count = 0;
			for(auto current=result; current; current=current->retired_next)
				recycled_count++;

			m_recycled_count.fetch_sub(recycled_count, std::memory_order_relaxed);
			record->available = result->retired_next;
		}
		result->retired_next = nullptr;
		result->next.store(nullptr, std::memory_order_relaxed);
		try {
			result->data.emplace(std::forward<Args>(args)...);
		}
		catch(...)
		{
			result->retired_next = record->available;
			record->available = result;
			throw;
		}
		return result;
	}

	void retire(hazard_record *record, node *retired_node)
	{
		retired_node->retired_next = record->retired;
		record->retired = retired_node;

		if( ++record->retired_count < retire_scan_threshold )
			return ;

		node *hazards[hazard_snapshot_capacity] {};
		size_t hazard_count = 0;
		bool hazard_overflow = false;

		for(auto current_record=m_hazard_records.load(std::memory_order_acquire);
			current_record; current_record=current_record->next)
		{
			for(auto &pointer : current_record->pointers)
			{
				if( auto pointer_value = pointer.load(std::memory_order_acquire) )
				{
					if( hazard_count == hazard_snapshot_capacity )
						hazard_overflow = true;
					else
						hazards[hazard_count++] = pointer_value;
				}
			}
		}
		node *reclaimed = nullptr;
		node *reclaimed_tail = nullptr;

		size_t reclaimed_count = 0;
		auto link = &record->retired;

		while( *link )
		{
			auto current = *link;
			const auto hazardous = hazard_overflow ? is_hazard(current) :
				std::find(hazards, hazards + hazard_count, current) != hazards + hazard_count;

			if( hazardous )
				link = &current->retired_next;
			else
			{
				*link = current->retired_next;
				record->retired_count--;

				current->next.store(nullptr, std::memory_order_relaxed);
				current->retired_next = reclaimed;

				reclaimed = current;
				reclaimed_count++;

				if( not reclaimed_tail )
					reclaimed_tail = current;
			}
		}
		if( reclaimed )
			recycle(reclaimed, reclaimed_tail, reclaimed_count);
	}

public:
	static constexpr size_t retire_scan_threshold = 64;
	static constexpr size_t hazard_snapshot_capacity = 32;
	static constexpr size_t recycled_node_capacity = 1024;

	alignas(64) std::atomic<node*> m_head {nullptr};
	alignas(64) std::atomic<node*> m_tail {nullptr};
	alignas(64) std::atomic<size_t> m_size {0};
	alignas(64) std::atomic<hazard_record*> m_hazard_records {nullptr};

	std::atomic<hazard_record*> m_enqueue_hazard_hint {nullptr};
	std::atomic<hazard_record*> m_dequeue_hazard_hint {nullptr};

	alignas(64) std::atomic<node*> m_recycled {nullptr};
	std::atomic_size_t m_recycled_count {0};
};

template <concepts::copy_or_move_constructible T, size_t N>
consteval size_t lock_free_queue<T,queue_type::linked,N>::capacity()
	noexcept requires (capacity_v > 0)
{
	return capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::linked,N>::lock_free_queue(size_t capacity)
	requires (capacity_v == 0) :
	m_impl(new impl(capacity))
{

}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::linked,N>::lock_free_queue() :
	m_impl(new impl())
{

}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::linked,N>::~lock_free_queue()
{
	delete m_impl;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::linked,N>::lock_free_queue(lock_free_queue &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::linked,N>&
lock_free_queue<T,queue_type::linked,N>::operator=(lock_free_queue &&other) noexcept
{
	if( &other == this )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::linked,N>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::linked,N>::enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T, size_t N>
template <typename...Args>
bool lock_free_queue<T,queue_type::linked,N>::emplace(Args&&...args) requires
	concepts::constructible<element_t,Args...>
{
	typename impl::hazard_guard hazard(m_impl, m_impl->m_enqueue_hazard_hint);

	// Reserve capacity before publishing the node. This keeps concurrent
	// producers from exceeding a bounded queue's configured capacity.
	auto size = m_impl->m_size.load(std::memory_order_relaxed);
	for(;;)
	{
		if( size >= capacity() )
			return false; // full

		if( m_impl->m_size.compare_exchange_weak(size, size + 1,
			std::memory_order_acq_rel, std::memory_order_relaxed) )
			break;
	}
	typename impl::node *new_node = nullptr;
	try {
		new_node = hazard.make_node(std::forward<Args>(args)...);
	}
	catch(...)
	{
		m_impl->m_size.fetch_sub(1, std::memory_order_release);
		throw;
	}
	for(;;)
	{
		auto old_tail = hazard.protect(0, m_impl->m_tail);
		auto next = old_tail->next.load(std::memory_order_acquire);

		if( old_tail != m_impl->m_tail.load(std::memory_order_acquire) )
			continue;
		if( next )
		{
			m_impl->m_tail.compare_exchange_weak(old_tail, next,
				std::memory_order_release, std::memory_order_relaxed);
			continue;
		}
		if( auto expected = static_cast<impl::node*>(nullptr);
			old_tail->next.compare_exchange_weak(expected, new_node,
				std::memory_order_release, std::memory_order_relaxed) )
		{
			m_impl->m_tail.compare_exchange_strong(old_tail, new_node,
				std::memory_order_release, std::memory_order_relaxed);
			return true;
		}
	}
}

template <concepts::copy_or_move_constructible T, size_t N>
optional<T> lock_free_queue<T,queue_type::linked,N>::dequeue()
{
	if( m_impl->m_size.load(std::memory_order_acquire) == 0 )
		return nullopt;

	typename impl::hazard_guard hazard(m_impl, m_impl->m_dequeue_hazard_hint);
	for(;;)
	{
		auto old_head = hazard.protect(0, m_impl->m_head);
		auto next = hazard.protect(1, old_head->next);

		if( old_head != m_impl->m_head.load(std::memory_order_acquire) )
			continue;

		if( not next )
			return nullopt;

		auto old_tail = m_impl->m_tail.load(std::memory_order_acquire);
		if( old_head == old_tail )
		{
			m_impl->m_tail.compare_exchange_weak(old_tail, next,
				std::memory_order_release, std::memory_order_relaxed);
			continue;
		}
		if( not m_impl->m_head.compare_exchange_weak(old_head, next,
			std::memory_order_acq_rel, std::memory_order_relaxed) )
			continue;

		m_impl->m_size.fetch_sub(1, std::memory_order_release);
		hazard.clear(0);
		hazard.retire(old_head);

		optional<element_t> elem;
		try {
			elem.emplace(std::move(*next->data));
		}
		catch(...)
		{
			next->data.reset();
			throw;
		}
		next->data.reset();
		return elem;
	}
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::linked,N>::dequeue(T &data)
{
	auto _data = dequeue();
	if( _data )
	{
		data = std::move(*_data);
		return true;
	}
	return false;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::linked,N>::empty() const noexcept
{
	return size() == 0;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::linked,N>::full() const noexcept
{
	return size() >= capacity();
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::linked,N>::size() const noexcept
{
	return m_impl->m_size.load(std::memory_order_acquire);
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::linked,N>::capacity()
	const noexcept requires (capacity_v == 0)
{
	return m_impl->m_capacity.load(std::memory_order_acquire);
}

template <concepts::copy_or_move_constructible T, size_t N>
void lock_free_queue<T,queue_type::linked,N>::set_capacity(size_t size)
	requires (capacity_v == 0)
{
	m_impl->m_capacity.store (
		size > 0 ? size : std::numeric_limits<size_t>::max(),
		std::memory_order_release
	);
}

namespace detail
{

template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI circular_lock_free_queue_block
{
	LIBGS_DISABLE_COPY_MOVE(circular_lock_free_queue_block)

	struct cell
	{
		std::atomic_size_t sequence {0};
		alignas(T) char storage[sizeof(T)] {};

		[[nodiscard]] T *value() noexcept {
			return std::launder(reinterpret_cast<T*>(storage));
		}
	};

public:
	explicit circular_lock_free_queue_block(size_t capacity) :
		m_capacity(capacity), m_cells(new cell[capacity])
	{
		for(size_t i=0; i<m_capacity; i++)
			m_cells[i].sequence.store(i * 2, std::memory_order_relaxed);
	}

	~circular_lock_free_queue_block()
	{
		// Queue destruction is documented as unsafe. With no concurrent users,
		// destroy every value that is still published in the ring.
		auto begin = m_dequeue_pos.load(std::memory_order_relaxed);
		auto end = m_enqueue_pos.load(std::memory_order_relaxed);

		for(auto pos=begin; pos<end; pos++)
		{
			if( auto &entry = m_cells[pos % m_capacity];
				entry.sequence.load(std::memory_order_relaxed) == pos * 2 + 1 )
				entry.value()->~T();
		}
	}

public:
	template <typename...Args>
	[[nodiscard]] bool emplace(Args&&...args) requires
		concepts::constructible<T,Args...>
	{
		size_t pos = m_enqueue_pos.load(std::memory_order_relaxed);
		cell *entry = nullptr;
		for(;;)
		{
			entry = &m_cells[pos % m_capacity];
			auto sequence = entry->sequence.load(std::memory_order_acquire);

			auto diff = static_cast<std::intptr_t>(sequence) -
				static_cast<std::intptr_t>(pos * 2);

			if( diff == 0 )
			{
				if( m_enqueue_pos.compare_exchange_weak(pos, pos + 1,
					std::memory_order_relaxed, std::memory_order_relaxed) )
					break;
			}
			else if( diff < 0 )
				return false;
			else
				pos = m_enqueue_pos.load(std::memory_order_relaxed);
		}
		new (entry->storage) T(std::forward<Args>(args)...);
		m_size.fetch_add(1, std::memory_order_release);

		entry->sequence.store(pos * 2 + 1, std::memory_order_release);
		return true;
	}

	[[nodiscard]] optional<T> dequeue()
	{
		size_t pos = m_dequeue_pos.load(std::memory_order_relaxed);
		cell *entry = nullptr;
		for(;;)
		{
			entry = &m_cells[pos % m_capacity];
			auto sequence = entry->sequence.load(std::memory_order_acquire);

			auto diff = static_cast<std::intptr_t>(sequence) -
				static_cast<std::intptr_t>(pos * 2 + 1);

			if( diff == 0 )
			{
				if( m_dequeue_pos.compare_exchange_weak(pos, pos + 1,
					std::memory_order_relaxed, std::memory_order_relaxed) )
					break;
			}
			else if( diff < 0 )
				return nullopt;
			else
				pos = m_dequeue_pos.load(std::memory_order_relaxed);
		}
		T result = std::move(*entry->value());
		entry->value()->~T();

		m_size.fetch_sub(1, std::memory_order_release);
		entry->sequence.store((pos + m_capacity) * 2, std::memory_order_release);
		return result;
	}

	[[nodiscard]] size_t size() const noexcept {
		return m_size.load(std::memory_order_acquire);
	}

public:
	const size_t m_capacity;
	std::atomic_bool m_closed {false};
	std::atomic_size_t m_active_enqueues {0};
	std::atomic<circular_lock_free_queue_block*> m_next {nullptr};

private:
	std::unique_ptr<cell[]> m_cells;
	alignas(64) std::atomic_size_t m_enqueue_pos {0};
	alignas(64) std::atomic_size_t m_dequeue_pos {0};
	alignas(64) std::atomic_size_t m_size {0};
};

template <concepts::copy_or_move_constructible T, size_t N> requires (N > 0)
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::circular, N>
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)

public:
	lock_free_queue_impl() : m_block(N) {}

	static_assert(std::atomic_size_t::is_always_lock_free,
		"Atomic size must be lock-free"
	);

public:
	template <typename...Args>
	[[nodiscard]] bool emplace(Args&&...args) requires
		concepts::constructible<T,Args...>
	{
		return m_block.emplace(std::forward<Args>(args)...);
	}

	[[nodiscard]] optional<T> dequeue()
	{
		return m_block.dequeue();
	}

	[[nodiscard]] size_t size() const noexcept {
		return m_block.size();
	}

private:
	circular_lock_free_queue_block<T> m_block;
};

template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::circular, 0>
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)

public:
	static_assert(std::atomic_size_t::is_always_lock_free,
		"Atomic size must be lock-free"
	);

	using block = circular_lock_free_queue_block<T>;

public:
	explicit lock_free_queue_impl(size_t capacity = 64)
	{
		m_capacity = capacity > 0 ? capacity : 64;
		m_first = new block(m_capacity);
		m_enqueue_block.store(m_first, std::memory_order_relaxed);
		m_dequeue_block.store(m_first, std::memory_order_relaxed);
	}

	~lock_free_queue_impl()
	{
		auto current = m_first;
		while( current )
		{
			auto next = current->m_next.load(std::memory_order_relaxed);
			delete current;
			current = next;
		}
	}

public:
	template <typename...Args>
	[[nodiscard]] bool emplace(Args&&...args) requires
		concepts::constructible<T,Args...>
	{
		for(;;)
		{
			auto current = m_enqueue_block.load(std::memory_order_acquire);
			current->m_active_enqueues.fetch_add(1, std::memory_order_acq_rel);

			if( current->m_closed.load(std::memory_order_acquire) )
			{
				current->m_active_enqueues.fetch_sub(1, std::memory_order_release);
				continue;
			}
			auto result = current->emplace(std::forward<Args>(args)...);
			current->m_active_enqueues.fetch_sub(1, std::memory_order_release);
			return result;
		}
	}

	[[nodiscard]] optional<T> dequeue()
	{
		for(;;)
		{
			auto current = m_dequeue_block.load(std::memory_order_acquire);
			if( auto result = current->dequeue() )
				return result;

			if( current == m_enqueue_block.load(std::memory_order_acquire) or
				not current->m_closed.load(std::memory_order_acquire) or
				current->m_active_enqueues.load(std::memory_order_acquire) != 0 )
				return nullopt;

			auto next = current->m_next.load(std::memory_order_acquire);
			if( not next )
				return nullopt;

			m_dequeue_block.compare_exchange_weak(current, next,
				std::memory_order_release, std::memory_order_relaxed);
		}
	}

	void set_capacity(size_t capacity)
	{
		capacity = capacity > 0 ? capacity : 64;
		auto new_block = std::make_unique<block>(capacity);
		for(;;)
		{
			auto current = m_enqueue_block.load(std::memory_order_acquire);
			if( bool expected = false;
				not current->m_closed.compare_exchange_weak(expected, true,
					std::memory_order_acq_rel, std::memory_order_relaxed) )
				continue;

			current->m_next.store(new_block.get(), std::memory_order_release);
			m_capacity.store(capacity, std::memory_order_release);
			m_enqueue_block.store(new_block.release(), std::memory_order_release);
			return;
		}
	}

	[[nodiscard]] size_t size() const noexcept
	{
		size_t result = 0;
		auto current = m_dequeue_block.load(std::memory_order_acquire);
		auto end = m_enqueue_block.load(std::memory_order_acquire);

		while( current )
		{
			result += current->size();
			if( current == end )
				break;
			current = current->m_next.load(std::memory_order_acquire);
		}
		return result;
	}

	[[nodiscard]] size_t capacity() const noexcept {
		return m_capacity.load(std::memory_order_acquire);
	}

private:
	block *m_first = nullptr;
	alignas(64) std::atomic_size_t m_capacity {};
	alignas(64) std::atomic<block*> m_enqueue_block {nullptr};
	alignas(64) std::atomic<block*> m_dequeue_block {nullptr};
};

} //namespace detail

template <concepts::copy_or_move_constructible T, size_t N>
class LIBGS_CORE_TAPI lock_free_queue<T,queue_type::circular,N>::impl :
	public detail::lock_free_queue_impl<T,queue_type::circular,N>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using detail::lock_free_queue_impl
		<T,queue_type::circular,N>::lock_free_queue_impl;
};

template <concepts::copy_or_move_constructible T, size_t N>
consteval size_t lock_free_queue<T,queue_type::circular,N>::capacity()
	noexcept requires (capacity_v > 0)
{
	return capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::circular,N>::lock_free_queue(size_t capacity)
	requires (capacity_v == 0) :
	m_impl(new impl(capacity))
{

}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::circular,N>::lock_free_queue() :
	m_impl(new impl())
{

}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::circular,N>::~lock_free_queue()
{
	delete m_impl;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::circular,N>::lock_free_queue(lock_free_queue &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,queue_type::circular,N>&
lock_free_queue<T,queue_type::circular,N>::operator=(lock_free_queue &&other) noexcept
{
	if( &other == this )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::circular,N>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::circular,N>::enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T, size_t N>
template <typename...Args>
bool lock_free_queue<T,queue_type::circular,N>::emplace(Args&&...args) requires
	concepts::constructible<element_t,Args...>
{
	return m_impl->emplace(std::forward<Args>(args)...);
}

template <concepts::copy_or_move_constructible T, size_t N>
optional<T> lock_free_queue<T,queue_type::circular,N>::dequeue()
{
	return m_impl->dequeue();
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::circular,N>::dequeue(element_t &data)
{
	auto _data = dequeue();
	if( _data )
	{
		data = std::move(*_data);
		return true;
	}
	return false;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::circular,N>::empty() const noexcept
{
	return size() == 0;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,queue_type::circular,N>::full() const noexcept
{
	return size() >= capacity();
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::circular,N>::size() const noexcept
{
	return m_impl->size();
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::circular,N>::capacity()
	const noexcept requires (capacity_v == 0)
{
	return m_impl->capacity();
}

template <concepts::copy_or_move_constructible T, size_t N>
void lock_free_queue<T,queue_type::circular,N>::set_capacity(size_t size)
	requires (capacity_v == 0)
{
	m_impl->set_capacity(size);
}

} //namespace libgs

#ifdef _MSC_VER
# pragma warning(pop)
#endif //_MSC_VER

#endif //LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
