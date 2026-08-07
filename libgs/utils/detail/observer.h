
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_UTILS_DETAIL_OBSERVER_H
#define LIBGS_UTILS_DETAIL_OBSERVER_H

#include <libgs/core/spin_mutex.h>
#include <cassert>
#include <set>
#include <map>

namespace libgs::utils { namespace detail
{

class LIBGS_UTILS_API observer
{
	LIBGS_DISABLE_COPY_MOVE(observer)

public:
	using set_t = std::set<void*>;
	using map_t = std::map<std_typeid_t, set_t>;

	[[nodiscard]] static map_t &map() noexcept;
	[[nodiscard]] static spin_mutex &mutex() noexcept;
};

} //namespace detail

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
class LIBGS_UTILS_TAPI basic_observer_base<Derived,Exec,Funcs...>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Exec0>
	impl(size_t id, Exec0 &&exec) :
		m_exec(get_executor_helper(std::forward<Exec0>(exec))), m_id(id) {}

public:
	asio::any_io_executor m_exec {};
	callbacks_t m_callbacks {};
	uint64_t m_id = 0;
};

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <concepts::match_sched<Exec> Exec0>
basic_observer_base<Derived,Exec,Funcs...>::basic_observer_base(uint64_t id, Exec0 &&exec) :
	m_impl(new impl(id, std::forward<Exec0>(exec)))
{
	detail::observer::mutex().lock();
	auto [it, inserted] = detail::observer::map()[typeid(derived_t).hash_code()]
		.emplace(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	assert(inserted);
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
basic_observer_base<Derived,Exec,Funcs...>::~basic_observer_base()
{
	detail::observer::mutex().lock();
	detail::observer::map()[typeid(derived_t).hash_code()]
		.erase(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	delete m_impl;
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <typename...Args0>
basic_observer_base<Derived,Exec,Funcs...>::ptr_t
basic_observer_base<Derived,Exec,Funcs...>::make(Args0&&...args) requires
	concepts::constructible<derived_t,Args0...>
{
	return std::make_shared<derived_t>(std::forward<Args0>(args)...);
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <size_t Idx>
basic_observer_base<Derived,Exec,Funcs...>::ptr_t
basic_observer_base<Derived,Exec,Funcs...>::on_triggered(callback_t<Idx> func) requires idx_valid_v<Idx>
{
	std::get<Idx>(m_impl->m_callbacks).emplace_back(std::move(func));
	return this->shared_from_this();
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
template <size_t Idx, typename...Args0>
void basic_observer_base<Derived,Exec,Funcs...>::trigger(uint64_t id, Args0&&...args)
	requires idx_valid_v<Idx> and concepts::callable<callback_t<Idx>,Args0...>
{
	std::vector<std::function<void()>> functions;
	detail::observer::mutex().lock();

	for(auto &ptr : detail::observer::map()[typeid(derived_t).hash_code()])
	{
		auto obj = static_cast<impl*>(ptr);
		auto &funcs = std::get<Idx>(obj->m_callbacks);

		if( id != obj->m_id or funcs.empty() )
			continue;

		functions.emplace_back([exec = obj->m_exec, funcs, args...]() mutable
		{
			auto call = [exec = std::move(exec)]<typename...Args>(auto func, Args&&...args)
			{
				using return_t = decltype(func(std::forward<Args>(args)...));
				if constexpr( is_awaitable_v<return_t> )
				{
					libgs::dispatch(exec, [func = std::move(func), ...args = std::forward<Args>(args)]
					() mutable -> awaitable<void> {
						co_await func(std::move(args)...);
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(exec, [func = std::move(func), ...args = std::forward<Args>(args)]() mutable {
						func(std::move(args)...);
					});
				}
			};
			for(size_t i=0; i<funcs.size()-1; i++)
				call(std::move(funcs[i]), args...);
			call(std::move(funcs.back()), std::move(args)...);
		});
	}
	detail::observer::mutex().unlock();
	for(auto &func : functions)
		func();
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
basic_observer_base<Derived,Exec,Funcs...>::executor_t
basic_observer_base<Derived,Exec,Funcs...>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_OBSERVER_H
