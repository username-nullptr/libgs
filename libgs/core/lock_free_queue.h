
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

#ifndef LIBGS_CORE_LOCK_FREE_QUEUE_H
#define LIBGS_CORE_LOCK_FREE_QUEUE_H

#include <libgs/core/global.h>

namespace libgs
{

// TODO ... ...
enum class queue_type {
	linked, circular
};

template <concepts::copy_or_move_constructible T, typename Derived>
class LIBGS_CORE_TAPI lock_free_queue_base
{
	using element_t = T;
	using derived_t = crtp_derived_t<Derived,lock_free_queue_base>;

public:
	void force_enqueue(element_t &&data);
	void force_enqueue(const element_t &data) requires
		concepts::copy_constructible<element_t>;

	template <typename...Args>
	void force_emplace(Args&&...args) requires
		concepts::constructible<element_t,Args...>;
};

template <concepts::copy_or_move_constructible T, size_t N = 0>
class LIBGS_CORE_TAPI lock_free_queue;

// linked list queue
template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI lock_free_queue<T,0> :
	public lock_free_queue_base<T,lock_free_queue<T>>
{
	LIBGS_DISABLE_COPY(lock_free_queue)

public:
	using element_t = T;
	constexpr explicit lock_free_queue(size_t capacity = std::numeric_limits<size_t>::max());
	~lock_free_queue();

	lock_free_queue(lock_free_queue &&other) noexcept; // unsafe
	lock_free_queue &operator=(lock_free_queue &&other) noexcept; // unsafe

public: // safe
	bool enqueue(element_t &&data);
	bool enqueue(const element_t &data) requires
		concepts::copy_constructible<element_t>;

	template <typename...Args>
	bool emplace(Args&&...args) requires
		concepts::constructible<element_t,Args...>;

	optional<element_t> dequeue();
	bool dequeue(element_t &data);

public:
	[[nodiscard]] constexpr size_t capacity() const noexcept;
	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] bool full() const noexcept;
	[[nodiscard]] size_t size() const noexcept;

private:
	class impl;
	impl *m_impl;
};

// circular queue
template <concepts::copy_or_move_constructible T, size_t N>
class LIBGS_CORE_TAPI lock_free_queue :
	public lock_free_queue_base<T,lock_free_queue<T,N>>
{
	LIBGS_DISABLE_COPY(lock_free_queue)

public:
	using element_t = T;
	static constexpr size_t capacity_v = N;
	[[nodiscard]] static consteval size_t capacity() noexcept;

	lock_free_queue();
	~lock_free_queue();

	lock_free_queue(lock_free_queue &&other) noexcept; // unsafe
	lock_free_queue &operator=(lock_free_queue &&other) noexcept; // unsafe

public: // safe
	bool enqueue(element_t &&data);
	bool enqueue(const element_t &data) requires
		concepts::copy_constructible<element_t>;

	template <typename...Args>
	bool emplace(Args&&...args) requires
		concepts::constructible<element_t,Args...>;

	optional<element_t> dequeue();
	bool dequeue(element_t &data);

public:
	[[nodiscard]] bool empty() const noexcept;
	[[nodiscard]] bool full() const noexcept;
	[[nodiscard]] size_t size() const noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs
#include <libgs/core/detail/lock_free_queue.h>


#endif //LIBGS_CORE_LOCK_FREE_QUEUE_H
