
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
	impl() = default;
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
	struct node
	{
		T data;
		std::atomic<node*> next {nullptr};

		template <typename...Args>
		explicit node(Args&&...args) : data(std::forward<Args>(args)...) {}
	};
	std::atomic<node*> m_head {nullptr};
	std::atomic<node*> m_tail {nullptr};
};

template <concepts::copy_or_move_constructible T>
lock_free_queue<T>::lock_free_queue() :
	m_impl(new impl())
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
void lock_free_queue<T>::enqueue(const T &data) requires concepts::copy_constructible<T>
{
	emplace(data);
}

template <concepts::copy_or_move_constructible T>
void lock_free_queue<T>::enqueue(T &&data)
{
	emplace(std::move(data));
}

template <concepts::copy_or_move_constructible T>
template <typename...Args>
void lock_free_queue<T>::emplace(Args&&...args)
{
	using node_t = typename impl::node;
	auto n = new node_t(std::forward<Args>(args)...);
	node_t *tail = nullptr;
	for(;;)
	{
		tail = m_impl->m_tail.load(std::memory_order_acquire);
        if( not tail )
        {
            node_t *tmp = nullptr;
            if( m_impl->m_head.compare_exchange_weak
            	(tmp, n, std::memory_order_acq_rel, std::memory_order_relaxed) )
            {
                m_impl->m_tail.store(n, std::memory_order_release);
                return ;
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
				return ;
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
}

template <concepts::copy_or_move_constructible T>
std::optional<T> lock_free_queue<T>::dequeue()
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
			delete head;
			return std::move(head->data);
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

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
