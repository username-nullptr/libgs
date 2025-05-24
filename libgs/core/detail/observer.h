
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

#include <cassert>
#include <set>
#include <map>

namespace libgs { namespace detail
{

using obs_set_t = std::set<void*>;
using obs_map_t = std::map<std_typeid_t, obs_set_t>;
[[nodiscard]] LIBGS_CORE_API obs_map_t &observer_map() noexcept;

} //namespace detail

template <typename Derived, typename...Args>
class observer<Derived,Args...>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <concepts::sched Exec>
	explicit impl(Exec &&exec) :
		m_exec(get_executor_helper(std::forward<Exec>(exec))) {}

public:
	asio::any_io_executor m_exec {};
	callback_t m_callback {};
};

template <typename Derived, typename...Args>
template <concepts::sched Exec>
observer<Derived,Args...>::observer(Exec &&exec) :
	m_impl(new impl(std::forward<Exec>(exec)))
{
	auto [it, inserted] = detail::observer_map()[typeid(derived_t).hash_code()]
		.emplace(static_cast<void*>(m_impl));
	assert(inserted);
}

template <typename Derived, typename...Args>
observer<Derived,Args...>::~observer()
{
	detail::observer_map()[typeid(derived_t).hash_code()]
		.erase(static_cast<void*>(m_impl));
	delete m_impl;
}

template <typename Derived, typename...Args>
template <concepts::sched Exec, typename...Args0>
typename observer<Derived,Args...>::ptr_t
observer<Derived,Args...>::make(Exec &&exec, Args0&&...args) requires
	concepts::constructible<derived_t,Exec,Args0...>
{
	return std::make_shared<derived_t>(
		std::forward<Exec>(exec), std::forward<Args0>(args)...
	);
}

template <typename Derived, typename...Args>
template <typename...Args0>
typename observer<Derived,Args...>::ptr_t
observer<Derived,Args...>::make(Args0&&...args) requires
	concepts::constructible<derived_t,io_context_t&,Args0...> or
	concepts::constructible<derived_t,Args0...>
{
	if constexpr( concepts::constructible<derived_t,io_context_t&,Args0...> )
		return make(io_context(), std::forward<Args0>(args)...);
	else
		return std::make_shared<derived_t>(std::forward<Args0>(args)...);
}

template <typename Derived, typename...Args>
template <typename...Args0>
void observer<Derived,Args...>::trigger(Args0&&...args)
	requires concepts::callable<callback_t,Args0...>
{
	for(auto &ptr : detail::observer_map()[typeid(derived_t).hash_code()])
	{
		auto obj = static_cast<impl*>(ptr);
		if( not obj->m_callback )
			continue;

		dispatch(obj->m_exec,
		[func = obj->m_callback, ...args = std::forward<Args0>(args)]() mutable {
			func(std::forward<Args0>(args)...);
		});
	}
}

template <typename Derived, typename...Args>
typename observer<Derived,Args...>::ptr_t
observer<Derived,Args...>::set_callback(callback_t func)
{
	m_impl->m_callback = std::move(func);
	return this->shared_from_this();
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_OBSERVER_H