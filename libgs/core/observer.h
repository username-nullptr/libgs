
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

#ifndef LIBGS_CORE_OBSERVER_H
#define LIBGS_CORE_OBSERVER_H

#include <libgs/core/execution.h>

namespace libgs
{

template <typename Derived, typename...Args>
class LIBGS_CORE_TAPI observer : public std::enable_shared_from_this<
	crtp_derived_t<Derived, observer<Derived,Args...>>>
{
	LIBGS_DISABLE_COPY_MOVE(observer)
	using derived_t = crtp_derived_t<Derived,observer>;

public:
	using callback_t = std::function<void(Args...)>;
	using ptr_t = std::shared_ptr<derived_t>;

	template <concepts::sched Exec>
	explicit observer(Exec &&exec = io_context());
	virtual ~observer() = 0;

public:
	template <concepts::sched Exec, typename...Args0>
	[[nodiscard]] static ptr_t make(Exec &&exec, Args0&&...args) requires
		concepts::constructible<derived_t,Exec,Args0...>;

	template <typename...Args0>
	[[nodiscard]] static ptr_t make(Args0&&...args) requires
		concepts::constructible<derived_t,io_context_t&,Args0...> or
		concepts::constructible<derived_t,Args0...>;

	template <typename...Args0>
	static void trigger(Args0&&...args) requires
		concepts::callable<callback_t,Args0...>;

protected:
	ptr_t set_callback(callback_t func);

private:
	class impl;
	impl *m_impl = nullptr;
};

} //namespace libgs
#include <libgs/core/detail/observer.h>


#endif //LIBGS_CORE_OBSERVER_H