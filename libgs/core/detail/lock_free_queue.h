
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

template <concepts::copy_or_move_constructible T>
class lock_free_queue<T>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	constexpr explicit impl(size_t capacity) :
		m_capacity(check_capacity(capacity)) {}

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

	[[nodiscard]] bool is_full(std::memory_order order) const noexcept {
		return m_size.load(order) >= m_capacity;
	}

private:
	constexpr static size_t check_capacity(size_t capacity) {
		return capacity > 0 ? capacity : std::numeric_limits<size_t>::max();
	}

public:
	struct node
	{
		element_t data;
		std::atomic<node*> next {nullptr};

		template <typename...Args>
		explicit node(Args&&...args) : data(std::forward<Args>(args)...) {}
	};
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
bool lock_free_queue<T>::enqueue(const element_t &data) requires concepts::copy_constructible<T>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T>
bool lock_free_queue<T>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T>
template <typename...Args>
bool lock_free_queue<T>::emplace(Args&&...args)
{
	if( m_impl->is_full(std::memory_order_relaxed) )
		return false;

	using node_t = impl::node;
	auto n = new node_t(std::forward<Args>(args)...);

	node_t *tail = nullptr;
	for(;;)
	{
		if( m_impl->is_full(std::memory_order_acquire) )
		{
			delete n;
			break;
		}
		tail = m_impl->m_tail.load(std::memory_order_acquire);
        if( not tail )
        {
            node_t *tmp = nullptr;
            if( m_impl->m_head.compare_exchange_weak
            	(tmp, n, std::memory_order_acq_rel, std::memory_order_relaxed) )
            {
                m_impl->m_tail.store(n, std::memory_order_release);
            	m_impl->m_size.fetch_add(1, std::memory_order_release);
                return true;
            }
            continue;
        }
		auto next = tail->next.load(std::memory_order_acquire);
		if( not next )
		{
			if( tail->next.compare_exchange_weak
				(next, n, std::memory_order_acq_rel, std::memory_order_relaxed) )
			{
				m_impl->m_tail.compare_exchange_strong(tail, n);
				m_impl->m_size.fetch_add(1, std::memory_order_release);
				return true;
			}
		}
		// The 'next' is not empty,
		// which means that another thread is also being inserted
		// and the temporary tail node needs to be updated.
		else
		{
			m_impl->m_tail.compare_exchange_strong (
				tail, next, std::memory_order_release, std::memory_order_relaxed
			);
		}
	}
	return false;
}

template <concepts::copy_or_move_constructible T>
optional<T> lock_free_queue<T>::dequeue()
{
	typename impl::node *head = nullptr;
	for(;;)
	{
		head = m_impl->m_head.load(std::memory_order_acquire);
		if( not head )
			return {};

		auto tail = m_impl->m_tail.load(std::memory_order_acquire);
		auto next = head->next.load(std::memory_order_acquire);

		if( head != m_impl->m_head.load(std::memory_order_relaxed) )
			continue;

		else if( head == tail ) // Queue may be empty.
		{
			if( not next ) // Queue is empty.
				return {};

			// Another thread is inserting.
			m_impl->m_tail.compare_exchange_weak (
				tail, next, std::memory_order_release, std::memory_order_relaxed
			);
		}
		else if( m_impl->m_head.compare_exchange_weak
				 (head, next, std::memory_order_acq_rel, std::memory_order_relaxed) )
		{
			auto data = std::move(head->data);
			delete head;

			m_impl->m_size.fetch_sub(1, std::memory_order_release);
			return std::move(data);
		}
	}
#ifndef _MSC_VER
	return {};
#endif //_MSC_VER
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
	return m_impl->is_full(std::memory_order_acquire);
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

    [[nodiscard]] static constexpr size_t index_for(size_t idx) noexcept
	{
        if constexpr( is_power_of_two(capacity_v) )
            return idx & (capacity_v - 1);  // 位运算，更高效
        else
            return idx % capacity_v;  // 模运算，兼容性更好
    }

private:
	[[nodiscard]] static consteval bool is_power_of_two(size_t n) noexcept {
		return n > 0 and (n & (n - 1)) == 0;
	}

public:
	alignas(64) std::unique_ptr<element_t> m_buffer[N] {};
	alignas(64) std::atomic<size_t> m_read_idx {0};
	alignas(64) std::atomic<size_t> m_write_idx {0};
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
bool lock_free_queue<T,N>::enqueue(const element_t &data) requires concepts::copy_constructible<T>
{
	return emplace(data);
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::enqueue(element_t &&data)
{
	return emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T, size_t N>
template <typename...Args>
bool lock_free_queue<T,N>::emplace(Args&&...args)
{
	auto w_idx = m_impl->m_write_idx.load(std::memory_order_relaxed);
	for(;;)
	{
		auto r_idx = m_impl->m_read_idx.load(std::memory_order_acquire);

		// is full ?
		if( w_idx - r_idx >= capacity_v )
			return false;

		// try to update write index
		if( m_impl->m_write_idx.compare_exchange_weak
			(w_idx, w_idx + 1, std::memory_order_release, std::memory_order_relaxed) )
		{
			m_impl->m_buffer[impl::index_for(w_idx)] =
				std::make_unique<element_t>(std::forward<Args>(args)...);
			break;
		}
		// failed, retry
	}
	return true;
}

template <concepts::copy_or_move_constructible T, size_t N>
optional<T> lock_free_queue<T,N>::dequeue()
{
	auto r_idx = m_impl->m_read_idx.load(std::memory_order_relaxed);
	for(;;)
	{
		auto w_idx = m_impl->m_write_idx.load(std::memory_order_acquire);

		// is empty ?
		if( r_idx == w_idx )
			break;

		// try to update read index
		if( m_impl->m_read_idx.compare_exchange_weak
			(r_idx, r_idx + 1, std::memory_order_release, std::memory_order_relaxed) )
			return std::move(*m_impl->m_buffer[impl::index_for(r_idx)]);

		// failed, retry
	}
	return {};
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
	return m_impl->m_read_idx.load(std::memory_order_acquire) ==
		   m_impl->m_write_idx.load(std::memory_order_acquire);
}

template <concepts::copy_or_move_constructible T, size_t N>
bool lock_free_queue<T,N>::full() const noexcept
{
	return size() >= capacity_v;
}

template <concepts::copy_or_move_constructible T, size_t N>
size_t lock_free_queue<T,N>::size() const noexcept
{
	auto w_idx = m_impl->m_write_idx.load(std::memory_order_acquire);
	auto r_idx = m_impl->m_read_idx.load(std::memory_order_acquire);
	return w_idx - r_idx;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
