
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

namespace libgs
{

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(element_t &&data)
{
	emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, typename Derived>
void lock_free_queue_base<T,Derived>::force_enqueue(const element_t &data)
	requires concepts::copy_constructible<T>
{
	emplace(data);
}

template <concepts::copy_or_move_constructible T, typename Derived>
template <typename...Args>
void lock_free_queue_base<T,Derived>::force_emplace(Args&&...args) requires
	concepts::constructible<T,Args...>
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

template <concepts::copy_or_move_constructible T>
class lock_free_queue<T>::impl
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
	constexpr explicit impl(size_t capacity) :
		m_capacity(check_capacity(capacity))
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

private:
	constexpr static size_t check_capacity(size_t capacity) {
		return capacity > 0 ? capacity : std::numeric_limits<size_t>::max();
	}

public:
	std::atomic<node*> m_head {nullptr};
	std::atomic<node*> m_tail {nullptr};
	std::atomic<size_t> m_size {0};
	const size_t m_capacity = 0;
};

template <concepts::copy_or_move_constructible T>
constexpr lock_free_queue<T>::lock_free_queue(size_t capacity) :
	m_impl(new impl(capacity))
{

}

template <concepts::copy_or_move_constructible T>
lock_free_queue<T>::~lock_free_queue()
{
	delete m_impl;
}

template <concepts::copy_or_move_constructible T>
lock_free_queue<T>::lock_free_queue(lock_free_queue &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::copy_or_move_constructible T>
lock_free_queue<T> &lock_free_queue<T>::operator=(lock_free_queue &&other) noexcept
{
	if( &other == this )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T>
template <typename...Args>
bool lock_free_queue<T>::emplace(Args&&...args) requires
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

template <concepts::copy_or_move_constructible T>
optional<T> lock_free_queue<T>::dequeue()
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

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::dequeue(T &data)
{
	auto _data = dequeue();
	if( _data )
	{
		data = std::move(*_data);
		return true;
	}
	return false;
}

template <concepts::copy_or_move_constructible T>
constexpr size_t lock_free_queue<T>::capacity() const noexcept
{
	return m_impl->m_capacity;
}

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::empty() const noexcept
{
	return size() == 0;
}

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::full() const noexcept
{
	return size() == capacity();
}

template <concepts::copy_or_move_constructible T>
size_t lock_free_queue<T>::size() const noexcept
{
	return m_impl->m_size.load(std::memory_order_acquire);
}

template <concepts::copy_or_move_constructible T, size_t N>
class lock_free_queue<T,N>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;
	~impl() = default;

    [[nodiscard]] static constexpr size_t next_index(size_t idx) noexcept
	{
        if constexpr( is_power_of_two(capacity_v) )
    		return idx & (capacity_v - 1);
        else
            return idx % capacity_v;
    }

private:
	[[nodiscard]] static consteval bool is_power_of_two(size_t n) noexcept {
		return n > 0 and (n & (n - 1)) == 0;
	}

public:
	enum class node_state {
		writable, writing, readable, reading
	};
	std::atomic<node_state> m_states[N] {};
	std::unique_ptr<element_t> m_buffer[N] {};

	alignas(64) std::atomic<size_t> m_head {0};
	alignas(64) std::atomic<size_t> m_tail {0};
	alignas(64) std::atomic<size_t> m_size {0};
};

template <concepts::copy_or_move_constructible T, size_t N>
consteval size_t lock_free_queue<T,N>::capacity() noexcept
{
	return capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,N>::lock_free_queue() :
	m_impl(new impl())
{

}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,N>::~lock_free_queue()
{
	delete m_impl;
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,N>::lock_free_queue(lock_free_queue &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::copy_or_move_constructible T, size_t N>
lock_free_queue<T,N> &lock_free_queue<T,N>::operator=(lock_free_queue &&other) noexcept
{
	if( &other == this )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::enqueue(const element_t &data)
	requires concepts::copy_constructible<element_t>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T, size_t N>
template <typename...Args>
bool lock_free_queue<T,N>::emplace(Args&&...args) requires
	concepts::constructible<element_t,Args...>
{
	for(;;)
	{
		auto curr_size = m_impl->m_size.load(std::memory_order_acquire);
		if( curr_size >= capacity_v )
			return false; // full

		else if( m_impl->m_size.compare_exchange_strong(curr_size,
			curr_size + 1, std::memory_order_acquire, std::memory_order_relaxed) )
			break;
	}
	constexpr size_t max_retries = 3;
	size_t retries = 0;

	auto curr_tail = m_impl->m_tail.load(std::memory_order_relaxed);
	auto next_tail = impl::next_index(curr_tail);
	for(;;)
	{
		// Update the index first.
		auto expected = impl::node_state::writable;
		if( not m_impl->m_states[next_tail].compare_exchange_strong(expected,
			impl::node_state::writing, std::memory_order_acquire, std::memory_order_relaxed) )
		{
			// There are other threads that have executed ahead of this one.
			if( ++retries >= max_retries )
			{
				m_impl->m_tail.compare_exchange_strong(curr_tail,
					impl::next_index(curr_tail), std::memory_order_relaxed, std::memory_order_relaxed
				);
				curr_tail = m_impl->m_tail.load(std::memory_order_relaxed);
				next_tail = impl::next_index(curr_tail);
				retries = 0;
			}
			continue;
		}
		m_impl->m_buffer[next_tail] =
			std::make_unique<element_t>(std::forward<Args>(args)...);

		m_impl->m_states[next_tail].store (
			impl::node_state::readable, std::memory_order_release
		);
		m_impl->m_tail.store(next_tail + 1, std::memory_order_release);
		break;
	}
	return true;
}

template <concepts::copy_or_move_constructible T, size_t N>
optional<T> lock_free_queue<T,N>::dequeue()
{
	std::unique_ptr<element_t> elem;
	for(;;)
	{
		auto curr_size = m_impl->m_size.load(std::memory_order_acquire);
		if( curr_size == 0 )
			return {}; // empty

		else if( m_impl->m_size.compare_exchange_strong(curr_size,
			curr_size - 1, std::memory_order_acquire, std::memory_order_relaxed) )
			break;
	}
	constexpr size_t max_retries = 3;
	size_t retries = 0;

	auto curr_head = m_impl->m_head.load(std::memory_order_relaxed);
	auto next_head = impl::next_index(curr_head);
	for(;;)
	{
		auto expected = impl::node_state::readable;
		if( not m_impl->m_states[next_head].compare_exchange_strong(expected,
			impl::node_state::reading, std::memory_order_acquire, std::memory_order_relaxed) )
		{
			// There are other threads that have executed ahead of this one.
			if( ++retries >= max_retries )
			{
				m_impl->m_head.compare_exchange_strong(curr_head,
					impl::next_index(curr_head), std::memory_order_relaxed, std::memory_order_relaxed
				);
				curr_head = m_impl->m_head.load(std::memory_order_relaxed);
				next_head = impl::next_index(curr_head);
				retries = 0;
			}
			continue;
		}
		elem = std::move(m_impl->m_buffer[next_head]);

		m_impl->m_states[next_head].store (
			impl::node_state::writable, std::memory_order_release
		);
		m_impl->m_head.store(next_head + 1, std::memory_order_release);
		break;
	}
	return std::move(*elem);
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::dequeue(element_t &data)
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
bool lock_free_queue<T,N>::empty() const noexcept
{
	return size() == 0;
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::full() const noexcept
{
	return size() == capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,N>::size() const noexcept
{
	return m_impl->m_size.load(std::memory_order_acquire);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
