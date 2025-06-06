
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

#ifndef LIBGS_CORE_DETAIL_OBSERVER_H
#define LIBGS_CORE_DETAIL_OBSERVER_H

#include <libgs/core/spin_mutex.h>
#include <cassert>
#include <set>
#include <map>

namespace libgs { namespace detail
{

class LIBGS_CORE_API observer
{
	LIBGS_DISABLE_COPY_MOVE(observer)

public:
	using set_t = std::set<void*>;
	using map_t = std::map<std_typeid_t, set_t>;

	[[nodiscard]] static map_t &map() noexcept;
	[[nodiscard]] static spin_mutex &mutex() noexcept;
};

} //namespace detail

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
class observer_base<Derived,Funcs...>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <concepts::sched Exec>
	explicit impl(Exec &&exec) :
		m_exec(get_executor_helper(std::forward<Exec>(exec))) {}

public:
	asio::any_io_executor m_exec {};
	callbacks_t m_callbacks {};
};

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <concepts::sched Exec>
observer_base<Derived,Funcs...>::observer_base(Exec &&exec) :
	m_impl(new impl(std::forward<Exec>(exec)))
{
	detail::observer::mutex().lock();
	auto [it, inserted] = detail::observer::map()[typeid(derived_t).hash_code()]
		.emplace(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	assert(inserted);
}

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
observer_base<Derived,Funcs...>::~observer_base()
{
	detail::observer::mutex().lock();
	detail::observer::map()[typeid(derived_t).hash_code()]
		.erase(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	delete m_impl;
}

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <typename...Args0>
typename observer_base<Derived,Funcs...>::ptr_t
observer_base<Derived,Funcs...>::make(Args0&&...args) requires
	concepts::constructible<derived_t,Args0...>
{
	return std::make_shared<derived_t>(std::forward<Args0>(args)...);
}

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <size_t Idx>
typename observer_base<Derived,Funcs...>::ptr_t
observer_base<Derived,Funcs...>::on_triggered(callback_t<Idx> func) requires idx_valid_v<Idx>
{
	std::get<Idx>(m_impl->m_callbacks) = std::move(func);
	return this->shared_from_this();
}

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <size_t Idx, typename...Args0>
void observer_base<Derived,Funcs...>::trigger(Args0&&...args)
	requires idx_valid_v<Idx> and concepts::callable<callback_t<Idx>,Args0...>
{
	std::vector<std::function<void()>> functions;
	detail::observer::mutex().lock();

	for(auto &ptr : detail::observer::map()[typeid(derived_t).hash_code()])
	{
		auto obj = static_cast<impl*>(ptr);
		auto func = std::get<Idx>(obj->m_callbacks);

		if( not func )
			continue;

		functions.emplace_back (
		[exec = obj->m_exec, func = std::move(func), ...args = std::forward<Args0>(args)]() mutable
		{
			dispatch(exec, [func = std::move(func), ...args = std::move(args)]() mutable {
				func(std::move(args)...);
			});
		});
	}
	detail::observer::mutex().unlock();
	for(auto &func : functions)
		func();
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_OBSERVER_H