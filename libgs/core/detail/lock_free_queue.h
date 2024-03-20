
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

template <concept_copymovable T>
lock_free_queue<T>::lock_free_queue() :
		m_head(new node(T())),
		m_tail(m_head.load())
{

}

template <concept_copymovable T>
lock_free_queue<T>::~lock_free_queue()
{
	do {
		auto n = m_head.load();
		m_head.store(n->next.load());
		delete n;
	}
	while( m_head.load() );
}

template <concept_copymovable T>
void lock_free_queue<T>::enqueue(const T &data)
{
	emplace(data);
}

template <concept_copymovable T>
void lock_free_queue<T>::enqueue(T &&data)
{
	emplace(std::move(data));
}

template <concept_copymovable T>
std::optional<T> lock_free_queue<T>::dequeue()
{
	node *head = nullptr;
	std::optional<T> data;
	for(;;)
	{
		head = m_head.load();
		auto tail = m_tail.load();
		auto next = head->next.load();

		if( head == m_head.load() )
		{
			if( head == tail ) // Queue may be empty.
			{
				if( next == nullptr ) // Queue is empty.
					return data;

				// Another thread is inserting.
				m_tail.compare_exchange_weak(tail, next);
			}
			else // pop front
			{
				data = std::move(next->data);
				if( m_head.compare_exchange_weak(head, next) )
					break;
			}
		}
	}
	delete head;
	return data;
}

template <concept_copymovable T>
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

template <concept_copymovable T>
template <typename...Args>
void lock_free_queue<T>::emplace(Args&&...args)
{
	auto n = new node(std::forward<Args>(args)...);
	auto tail = m_tail.load(std::memory_order_relaxed);
	for(;;)
	{
		auto next = tail->next.load();
		if( not next )
		{
			if( tail->next.compare_exchange_weak(next, n) )
			{
				m_tail.compare_exchange_strong(tail, n);
				return ;
			}
		}
			// The 'next' is not empty,
			// which means that another thread is also being inserted
			// and the temporary tail node needs to be updated.
		else m_tail.compare_exchange_strong(tail, next);
	}
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LOCK_FREE_QUEUE_H
