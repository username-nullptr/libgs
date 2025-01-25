
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

#ifndef LIBGS_CORE_LOCK_FREE_QUEUE_H
#define LIBGS_CORE_LOCK_FREE_QUEUE_H

#include <libgs/core/global.h>

namespace libgs
{

template <concepts::copy_or_move_constructible T>
class LIBGS_CORE_TAPI lock_free_queue
{
	LIBGS_DISABLE_COPY_MOVE(lock_free_queue)

public:
	lock_free_queue();
	~lock_free_queue();

public:
	void enqueue(const T &data) requires concepts::copy_constructible<T>;
	void enqueue(T &&data);

	template <typename...Args>
	void emplace(Args&&...args);

	std::optional<T> dequeue();
	bool dequeue(T &data);

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs
#include <libgs/core/detail/lock_free_queue.h>


#endif //LIBGS_CORE_LOCK_FREE_QUEUE_H
