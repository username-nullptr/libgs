
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
#define LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H

// #include <libgs/core/system/cpu.h>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4324)
#endif //_MSC_VER

namespace libgs
{

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(element_t &&data)
{
	emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	emplace(data);
}

template <concepts::copy_or_move_constructible T, typename Derived>
template <typename...Args>
void lock_free_queue_base<T,Derived>::force_emplace(Args&&...args) requires
	concepts::constructible<element_t,Args...>
{
	auto self = static_cast<Derived*>(this);
	for(;;)
	{
		if( self->full() )
			self->dequeue();
		if( self->emplace(std::forward<Args>(args)...) )
			break;
	}
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
		std::atomic_bool constructing {true};
		std::unique_ptr<element_t> data {};
		std::atomic<node*> next {nullptr};
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
	}

public:
	std::atomic<node*> m_head {nullptr};
	std::atomic<node*> m_tail {nullptr};
	std::atomic<size_t> m_size {0};
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
	for(;;)
	{
		if( m_impl->m_size.load(std::memory_order_acquire) >= capacity() )
			return false; // full

		auto node = new impl::node();
		auto old_tail = m_impl->m_tail.load(std::memory_order_relaxed);

		if( not m_impl->m_tail.compare_exchange_weak(old_tail,
			node, std::memory_order_release, std::memory_order_relaxed) )
		{
			// There are other threads that have executed ahead of this one.
			delete node;
			continue;
		}
		old_tail->next.store(node, std::memory_order_release);
		node->data = std::make_unique<element_t>(std::forward<Args>(args)...);

		// Construct time slice protection.
		node->constructing.store(false, std::memory_order_release);
		break;
	}
	m_impl->m_size.fetch_add(1, std::memory_order_release);
	return false;
}

template <concepts::copy_or_move_constructible T, size_t N>
optional<T> lock_free_queue<T,queue_type::linked,N>::dequeue()
{
	std::unique_ptr<element_t> elem;
	for(;;)
	{
		if( m_impl->m_size.load(std::memory_order_acquire) == 0 )
			return {}; // empty

		auto old_head = m_impl->m_head.load(std::memory_order_relaxed);
		auto next = old_head->next.load(std::memory_order_acquire);

		if( not next )
			return {}; // empty

		// Wait for the node to be constructed.
		while( next->constructing.load(std::memory_order_relaxed) ) {}

		if( not m_impl->m_head.compare_exchange_strong(old_head,
			next, std::memory_order_release, std::memory_order_relaxed) )
			continue;  // There are other threads that have executed ahead of this one.

		elem = std::move(next->data);
		delete old_head;
		break;
	}
	m_impl->m_size.fetch_sub(1, std::memory_order_release);
	return std::move(*elem);
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
	return size() == capacity();
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

enum class circular_lock_free_queue_node_state {
	writable, writing, readable, reading
};

template <concepts::copy_or_move_constructible T, size_t N> requires (N > 0)
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::circular, N>
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)

public:
	using node_state = circular_lock_free_queue_node_state;
	lock_free_queue_impl() = default;

	static_assert(std::atomic<node_state>::is_always_lock_free,
		"Atomic state must be lock-free"
	);
	static_assert(std::atomic_size_t::is_always_lock_free,
		"Atomic size must be lock-free"
	);

public:
	template <typename...Args>
	[[nodiscard]] bool emplace(Args&&...args) requires
		concepts::constructible<T,Args...>
	{
		size_t curr_tail = 0;
		for(;;)
		{
			curr_tail = m_tail.load(std::memory_order_relaxed);
			size_t curr_head = m_head.load(std::memory_order_acquire);

			if( curr_tail - curr_head >= N )
				return false;

			else if( m_tail.compare_exchange_weak(curr_tail, curr_tail + 1,
				std::memory_order_acq_rel, std::memory_order_relaxed) )
				break;
		}
		size_t idx = curr_tail % N;
		new (&m_buffer[idx]) T(std::forward<Args>(args)...);

		m_size.fetch_add(1, std::memory_order_release);
		m_occupied[idx].store(true, std::memory_order_release);
		return true;
	}

	[[nodiscard]] optional<T> dequeue()
	{
		size_t curr_head = 0;
		for(;;)
		{
			curr_head = m_head.load(std::memory_order_relaxed);
			size_t curr_tail = m_tail.load(std::memory_order_acquire);

			if( curr_head >= curr_tail )
				return nullopt;

			else if( m_head.compare_exchange_weak(curr_head, curr_head + 1,
				std::memory_order_acq_rel, std::memory_order_relaxed) )
				break;
		}
		size_t idx = curr_head % N;
		while( not m_occupied[idx].load(std::memory_order_acquire) ) {
			// none_instruction();
		}
		auto ptr = reinterpret_cast<T*>(&m_buffer[idx]);
		T result = std::move(*ptr);
		ptr->~T();

		m_size.fetch_sub(1, std::memory_order_acq_rel);
		m_occupied[idx].store(false, std::memory_order_release);
		return std::move(result);
	}

public:
	alignas(64) std::atomic_size_t m_head {0};
	alignas(64) std::atomic_size_t m_tail {0};
	alignas(64) std::atomic_size_t m_size {0};

	std::atomic_bool m_occupied[N] {};
	alignas(T) char m_buffer[N][sizeof(T)];
};

template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI lock_free_queue_impl<T, queue_type::circular, 0>
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue_impl)

public:
	using node_state = circular_lock_free_queue_node_state;

	static_assert(std::atomic<node_state>::is_always_lock_free,
		"Atomic state must be lock-free"
	);
	static_assert(std::atomic_size_t::is_always_lock_free,
		"Atomic size must be lock-free"
	);

	class LIBGS_CORE_TAPI block
	{
		LIBGS_DISABLE_COPY_MOVE(block)

	public:
		explicit block(size_t capacity) :
			m_capacity(capacity),
			m_occupied(new std::atomic_bool[capacity]()),
			m_buffer(new char[capacity * sizeof(T)]()) {}

		alignas(64) std::atomic<size_t> m_head {0};
		alignas(64) std::atomic<size_t> m_tail {0};
		const size_t m_capacity = 0;

		std::unique_ptr<std::atomic_bool[]> m_occupied {};
		std::unique_ptr<char[]> m_buffer {};
	};

public:
	explicit lock_free_queue_impl(size_t capacity = 64)
	{
		m_capacity = capacity > 0 ? capacity : 64;
		m_blocks[0].store(new block(m_capacity));
	}

	~lock_free_queue_impl()
	{
		for(auto &block : m_blocks)
		{
			auto ptr = block.load(std::memory_order_relaxed);
			if( ptr )
				delete ptr;
		}
	}

public:
	template <typename...Args>
	[[nodiscard]] bool emplace(Args&&...args) requires
		concepts::constructible<T,Args...>
	{
		auto enq_idx = m_enq_idx.load(std::memory_order_acquire);
		auto block = m_blocks[enq_idx].load(std::memory_order_acquire);
		size_t curr_tail = 0;
		for(;;)
		{
			curr_tail = block->m_tail.load(std::memory_order_relaxed);
			size_t curr_head = block->m_head.load(std::memory_order_acquire);

			if( curr_tail - curr_head >= block->m_capacity )
				return false;

			else if( block->m_tail.compare_exchange_weak(curr_tail, curr_tail + 1,
				std::memory_order_acq_rel, std::memory_order_relaxed) )
				break;
		}
		size_t idx = curr_tail % block->m_capacity;
		new (element_ptr(block->m_buffer.get(), idx))
			T(std::forward<Args>(args)...);

		m_size.fetch_add(1, std::memory_order_release);
		block->m_occupied[idx].store(true, std::memory_order_release);
		return true;
	}

	[[nodiscard]] optional<T> dequeue()
	{
		auto deq_idx = m_deq_idx.load(std::memory_order_acquire);
		auto block = m_blocks[deq_idx].load(std::memory_order_acquire);
		size_t curr_head = 0;
		for(;;)
		{
			curr_head = block->m_head.load(std::memory_order_relaxed);
			size_t curr_tail = block->m_tail.load(std::memory_order_acquire);

			if( curr_head >= curr_tail )
			{
				m_deq_idx.store (
					m_enq_idx.load(std::memory_order_relaxed),
					std::memory_order_release
				);
				block = m_blocks[deq_idx].load(std::memory_order_acquire);
				curr_head = block->m_head.load(std::memory_order_relaxed);
				curr_tail = block->m_tail.load(std::memory_order_acquire);

				if( curr_head >= curr_tail )
					return nullopt;
			}
			else if( block->m_head.compare_exchange_weak(curr_head, curr_head + 1,
				std::memory_order_acq_rel, std::memory_order_relaxed) )
				break;
		}
		size_t idx = curr_head % block->m_capacity;
		while( not block->m_occupied[idx].load(std::memory_order_acquire) ) {
			// none_instruction();
		}
		auto ptr = element_ptr(block->m_buffer.get(), idx);
		T result = std::move(*ptr);
		ptr->~T();

		m_size.fetch_sub(1, std::memory_order_acq_rel);
		block->m_occupied[idx].store(false, std::memory_order_release);
		return std::move(result);
	}

	void set_capacity(size_t capacity)
	{
		if( capacity == 0 )
			capacity = std::numeric_limits<size_t>::max();
		auto new_block = new block(capacity);

		if( auto expected = m_capacity.load(std::memory_order_relaxed);
			not m_capacity.compare_exchange_strong(expected, capacity,
			std::memory_order_release, std::memory_order_relaxed) )
		{
			delete new_block;
			return ;
		}
		auto deq_idx = m_deq_idx.load(std::memory_order_acquire);
		auto enq_idx = (deq_idx + 1) % m_blocks.size();

		auto old_block = m_blocks[enq_idx].exchange(new_block);
		m_enq_idx.store(enq_idx, std::memory_order_release);

		if( old_block )
			delete old_block;
	}

private:
	[[nodiscard]] static constexpr T *element_ptr(char *buffer, size_t idx) noexcept {
		return reinterpret_cast<T*>(buffer + idx * sizeof(T));
	}

public:
	alignas(64) std::atomic_size_t m_capacity {};
	alignas(64) std::atomic<size_t> m_size {0};

	alignas(64) std::atomic_size_t m_enq_idx {0};
	alignas(64) std::atomic_size_t m_deq_idx {0};

	std::array<std::atomic<block*>,2> m_blocks {};
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
	return size() == capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::circular,N>::size() const noexcept
{
	return m_impl->m_size.load(std::memory_order_acquire);
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,queue_type::circular,N>::capacity()
	const noexcept requires (capacity_v == 0)
{
	return m_impl->m_capacity.load(std::memory_order_acquire);
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
