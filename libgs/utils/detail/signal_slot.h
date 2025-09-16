
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

#ifndef LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H
#define LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H

#include <libgs/core/spin_mutex.h>
#include <map>

namespace libgs::utils
{

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
class LIBGS_UTILS_TAPI basic_signal_base<Derived,Func,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <concepts::match_sched<Exec> Exec0>
	explicit impl(Exec0 &&exec) :
		m_exec(get_executor_helper(std::forward<Exec0>(exec))) {}

	template <concepts::function Func1>
	class adapter_impl;

public:
	template <typename Ret, typename...Args>
	class LIBGS_UTILS_TAPI adapter_impl<Ret(Args...)>
	{
		LIBGS_DISABLE_COPY_MOVE(adapter_impl)

	public:
		template <concepts::sched Exec0, concepts::function Func0>
		adapter_impl(Exec0 &&exec, Func0 &&slot) :
			m_func([exec = get_executor_helper(std::forward<Exec0>(exec)),
					slot = std::forward<Func0>(slot)](Args...args)
			{
				auto args_tuple = std::make_tuple(std::move(args)...);
				using indices = std::make_index_sequence<function_traits<Func0>::arg_count>;

				[exec, slot, args = std::move(args_tuple)]<std::size_t...Is>
				(std::index_sequence<Is...>)
				{
					using return_t = function_traits<Func0>::return_type;
					if constexpr( is_awaitable_v<return_t> )
						libgs::dispatch(exec, slot(std::move(std::get<Is>(args))...));
					else
					{
						libgs::dispatch(exec, [slot = std::move(slot), args = std::move(args)] {
							std::invoke(std::move(slot), std::move(std::get<Is>(args))...);
						});
					}
				}
				(indices{});
			})
		{}

		template <typename...Args0>
		void operator()(Args0&&...args) {
			m_func(std::forward<Args0>(args)...);
		}

	private:
		std::function<function_t> m_func {};
	};

public:
	using adapter = adapter_impl<function_t>;
	using adapter_ptr = std::shared_ptr<adapter>;

	std::map<const void*, adapter_ptr> m_slots;
	spin_mutex m_mutex;
	executor_t m_exec;
};

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <concepts::match_sched<Exec> Exec0>
basic_signal_base<Derived,Func,Exec>::basic_signal_base(Exec0 &&exec) :
	m_impl(new impl(std::forward<Exec0>(exec)))
{

}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
basic_signal_base<Derived,Func,Exec>::~basic_signal_base()
{
	delete m_impl;
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <typename...Funcs>
basic_signal_base<Derived,Func,Exec>::derived_t&
basic_signal_base<Derived,Func,Exec>::connect(Funcs&&...funcs)
	noexcept requires is_slots_v<Funcs...>
{
	return connect(m_impl->m_exec, std::forward<Funcs>(funcs)...);
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <concepts::sched Exec0, typename...Funcs>
basic_signal_base<Derived,Func,Exec>::derived_t&
basic_signal_base<Derived,Func,Exec>::connect(Exec0 &&exec, Funcs&&...funcs)
	noexcept requires is_slots_v<Funcs...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->m_slots[reinterpret_cast<const void*>(&funcs)] = std::make_shared<typename impl::adapter>(
			std::forward<Exec0>(exec), std::forward<Funcs>(funcs)
		),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <typename...Func0>
basic_signal_base<Derived,Func,Exec>::derived_t&
basic_signal_base<Derived,Func,Exec>::disconnect(Func0&&...func) noexcept
	requires is_slots_v<Func0...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->m_slots.erase(reinterpret_cast<const void*>(&func)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
basic_signal_base<Derived,Func,Exec>::derived_t&
basic_signal_base<Derived,Func,Exec>::disconnect() noexcept
{
	m_impl->m_mutex.lock();
	m_impl->m_slots.clear();
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <typename...Args>
void basic_signal_base<Derived,Func,Exec>::emit(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	std::vector<typename impl::adapter_ptr> slots;
	m_impl->m_mutex.lock();
	for(auto &slot : std::views::values(m_impl->m_slots))
		slots.emplace_back(slot);
	m_impl->m_mutex.unlock();

	size_t i = 0;
	for(; i<slots.size()-1; i++)
		(*slots[i])(args...);
	(*slots[i])(std::forward<Args>(args)...);
}

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec>
template <typename...Args>
void basic_signal_base<Derived,Func,Exec>::operator()(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	emit(std::forward<Args>(args)...);
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H