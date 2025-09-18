
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
#include <libgs/core/execution.h>

namespace libgs::utils
{

template <typename Derived, concepts::std_func_temp Func>
class LIBGS_UTILS_TAPI signal_base<Derived,Func>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;

	template <concepts::function Func1>
	class adapter_impl; // Type erasure

public:
	template <typename Ret, typename...Args>
	class LIBGS_UTILS_TAPI adapter_impl<Ret(Args...)>
	{
		LIBGS_DISABLE_COPY_MOVE(adapter_impl)

	public:
		using ptr_t = std::shared_ptr<adapter_impl>;
		adapter_impl() = default;

		template <slot_mode Mode, typename...Args0>
		[[nodiscard]] static ptr_t make(Args0&&...args) noexcept
		{
			auto obj = std::make_shared<adapter_impl>();
			obj->template emplace<Mode>(std::forward<Args0>(args)...);
			return obj;
		}

	private:
		template <slot_mode Mode, concepts::function Func0>
		void emplace(Func0 &&slot) noexcept {
			emplace<Mode>(io_context(), std::forward<Func0>(slot));
		}

		template <slot_mode Mode, concepts::sched Exec0, concepts::function Func0>
		void emplace(Exec0 &&exec, Func0 &&slot) noexcept
		{
			if constexpr( Mode != slot_mode::async )
				m_block = true;

			m_func = [exec = get_executor_helper(std::forward<Exec0>(exec)),
					  slot = std::forward<Func0>(slot)](Args...args)
			{
				using func0_tr = function_traits<Func0>;
				using indices = std::make_index_sequence<func0_tr::arg_count>;

				[&exec, &slot, args = std::make_tuple(std::move(args)...)]
				<std::size_t...Is>(std::index_sequence<Is...>)
				{
					using return_t = function_traits<Func0>::return_type;
					if constexpr( is_awaitable_v<return_t> )
					{
						if constexpr( Mode == slot_mode::backpressure )
						{
							libgs::dispatch(exec,
								slot(std::move(std::get<Is>(args))...),
								use_future
							).wait();
						}
						else
						{
							libgs::dispatch(exec,
								slot(std::move(std::get<Is>(args))...)
							);
						}
					}
					else if constexpr( Mode == slot_mode::async )
					{
						libgs::dispatch(exec, [slot, args = std::move(args)]{
							slot(std::move(std::get<Is>(args))...);
						});
					}
					else
						slot(std::move(std::get<Is>(args))...);
				}
				(indices{});
			};
		}

	private:
		template <slot_mode Mode, typename Obj, concepts::function Func0>
		void emplace(Obj &&obj, Func0 &&slot) noexcept {
			emplace<Mode>(std::forward<Obj>(obj), io_context(), std::forward<Func0>(slot));
		}

		template <slot_mode Mode, typename Obj, concepts::sched Exec0, concepts::function Func0>
		void emplace(Obj &&obj, Exec0 &&exec, Func0 &&slot) noexcept
		{
			m_obj = obj.get();
			if constexpr( Mode != slot_mode::async )
				m_block = true;

			using obj_t = std::remove_cvref_t<Obj>::element_type;
			m_is_valid = [obj = std::weak_ptr<obj_t>(obj)] {
				return not obj.expired();
			};

			m_func = [obj = obj.get(), is_valid = m_is_valid,
					  exec = get_executor_helper(std::forward<Exec0>(exec)),
					  slot = std::forward<Func0>(slot)](Args...args)
			{
				using func0_tr = function_traits<Func0>;
				using indices = std::make_index_sequence<func0_tr::arg_count>;

				[obj, &exec, &slot, &is_valid, args = std::make_tuple(std::move(args)...)]
				<std::size_t...Is>(std::index_sequence<Is...>)
				{
					using return_t = function_traits<Func0>::return_type;
					// There are too many nested structures, but it's better than template specialization.
					if constexpr( func0_tr::is_member_func )
					{
						if constexpr( is_awaitable_v<return_t> )
						{
							if constexpr( Mode == slot_mode::backpressure )
							{
								libgs::dispatch(exec, [obj, is_valid = std::move(is_valid),
									slot = std::move(slot), args = std::move(args)]() -> awaitable<void>
								{
									if( is_valid() )
										co_await (obj->*slot)(std::move(std::get<Is>(args))...);
									co_return ;
								},
								use_future).wait();
							}
							else
							{
								libgs::dispatch(exec, [obj, is_valid = std::move(is_valid),
									slot = std::move(slot), args = std::move(args)]() -> awaitable<void>
								{
									if( is_valid() )
										co_await (obj->*slot)(std::move(std::get<Is>(args))...);
									co_return ;
								});
							}
						}
						else if constexpr( Mode == slot_mode::async )
						{
							libgs::dispatch(exec, [obj, is_valid = std::move(is_valid),
								slot = std::move(slot), args = std::move(args)]
							{
								if( is_valid() )
									(obj->*slot)(std::move(std::get<Is>(args))...);
							});
						}
						else
						{
							if( is_valid() )
								(obj->*slot)(std::move(std::get<Is>(args))...);
						}
					}
					else if constexpr( is_awaitable_v<return_t> )
					{
						if constexpr( Mode == slot_mode::backpressure )
						{
							libgs::dispatch(exec, [is_valid = std::move(is_valid),
								slot = std::move(slot), args = std::move(args)]() -> awaitable<void>
							{
								if( is_valid() )
									co_await slot(std::move(std::get<Is>(args))...);
								co_return ;
							},
							use_future).wait();
						}
						else
						{
							libgs::dispatch(exec, [is_valid = std::move(is_valid),
								slot = std::move(slot), args = std::move(args)]() -> awaitable<void>
							{
								if( is_valid() )
									co_await slot(std::move(std::get<Is>(args))...);
								co_return ;
							});
						}
					}
					else if constexpr( Mode == slot_mode::async )
					{
						libgs::dispatch(exec, [is_valid = std::move(is_valid),
							slot = std::move(slot), args = std::move(args)]
						{
							if( is_valid() )
								slot(std::move(std::get<Is>(args))...);
						});
					}
					else
					{
						if( is_valid() )
							slot(std::move(std::get<Is>(args))...);
					}
				}
				(indices{});
			};
		}

	public:
		template <typename...Args0>
		void operator()(Args0&&...args) {
			m_func(std::forward<Args0>(args)...);
		}

		const void *m_obj = nullptr;
		std::function<bool()> m_is_valid = []{ return true; };

		bool m_block = false;
		std::function<function_t> m_func {};
	};

public:
	using adapter = adapter_impl<function_t>;
	using adapter_ptr = adapter::ptr_t;

	struct slot_info
	{
		const void *func = nullptr;
		adapter_ptr slot {};
	};
	using slot_info_ptr = std::shared_ptr<slot_info>;

public:
	template <slot_mode Mode, typename Func0>
	void connect(Func0 &&func) noexcept
		requires is_global_slot_v<Mode,Func0> {
		connect<Mode>(io_context(), std::forward<Func0>(func));
	}

	template <slot_mode Mode, concepts::sched Exec0, typename Func0>
	void connect(Exec0 &&exec, Func0 &&func) noexcept
		requires is_global_slot_v<Mode,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [func](const auto &info) {
			return info->func == &func and info->slot->m_obj == nullptr;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(
			std::forward<Exec0>(exec), std::forward<Func0>(func)
		);
	}

	template <slot_mode Mode, typename Obj, typename Func0>
	void connect(Obj &&observer, Func0 &&func) noexcept
		requires is_obj_slot_v<Mode,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(
			std::forward<Obj>(observer), std::forward<Func0>(func)
		);
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename Func0>
	void connect(Obj &&observer, Exec0 &&exec, Func0 &&func) noexcept
		requires is_obj_slot_v<Mode,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(std::forward<Obj>(observer),
			std::forward<Exec0>(exec), std::forward<Func0>(func)
		);
	}

public:
	template <typename Func0>
	void disconnect(Func0 &&func) noexcept
		requires is_global_slot_v<slot_mode::async,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [func](const auto &info){
			return info->func == &func and info->slot->m_obj == nullptr;
		});
		if( it != m_slots.end() )
			m_slots.erase(it);
	}

	template <typename Obj, typename Func0>
	void disconnect(const Obj &observer, Func0 &&func) noexcept
		requires is_obj_slot_v<slot_mode::async,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it != m_slots.end() )
			m_slots.erase(it);
	}

public:
	std::deque<slot_info_ptr> m_slots;
	spin_mutex m_mutex;
};

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::signal_base() :
	m_impl(new impl())
{

}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::~signal_base()
{
	delete m_impl;
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Funcs&&...funcs)
	noexcept requires is_global_slots_v<Mode,Funcs...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(std::forward<Funcs>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Funcs&&...funcs)
	requires is_obj_slots_v<Mode,Obj,Funcs...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::connect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(
			std::forward<Obj>(observer), std::forward<Funcs>(funcs)
		),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, concepts::sched Exec0, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Exec0 &&exec, Funcs&&...funcs)
	noexcept requires is_global_slots_v<Mode,Funcs...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(
			std::forward<Exec0>(exec), std::forward<Funcs>(funcs)
		),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Exec0 &&exec, Funcs&&...funcs)
	requires is_obj_slots_v<Mode,Obj,Funcs...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::connect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(std::forward<Obj>(observer),
			std::forward<Exec0>(exec), std::forward<Funcs>(funcs)
		),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Funcs&&...funcs)
	noexcept requires is_global_slots_v<slot_mode::sync,Funcs...>
{
	return connect<slot_mode::sync>(std::forward<Funcs>(funcs)...);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Funcs&&...funcs)
	requires is_obj_slots_v<slot_mode::sync,Obj,Funcs...>
{
	return connect<slot_mode::sync>(
		std::forward<Obj>(observer), std::forward<Funcs>(funcs)...
	);
}

template <typename Derived, concepts::std_func_temp Func>
template <concepts::sched Exec0, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Exec0 &&exec, Funcs&&...funcs)
	noexcept requires is_global_slots_v<slot_mode::sync,Funcs...>
{
	return connect<slot_mode::sync>(
		std::forward<Exec0>(exec), std::forward<Funcs>(funcs)...
	);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, concepts::sched Exec0, typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Exec0 &&exec, Funcs&&...funcs)
	requires is_obj_slots_v<slot_mode::sync,Obj,Funcs...>
{
	return connect<slot_mode::sync>(std::forward<Obj>(observer),
		std::forward<Exec0>(exec), std::forward<Funcs>(funcs)...
	);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Funcs>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(Funcs&&...funcs)
	noexcept requires is_global_slots_v<slot_mode::async,Funcs...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->disconnect(std::forward<Funcs>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, typename...Func0>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer, Func0&&...funcs)
	requires is_obj_slots_v<slot_mode::async,Obj,Func0...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::disconnect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->disconnect(observer, std::forward<Func0>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect() noexcept
{
	m_impl->m_mutex.lock();
	m_impl->m_slots.clear();
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer)
	requires is_observer_v<Obj>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::disconnect: observer is nullptr");

	m_impl->m_mutex.lock();
	for(auto it=m_impl->m_slots.begin(); it!=m_impl->m_slots.end(); ++it)
	{
		if( (*it)->slot->m_obj == observer.get() )
			it = m_impl->m_slots.erase(it);
	}
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Args>
void signal_base<Derived,Func>::emit(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	std::vector<typename impl::adapter_ptr> nonblock_slots;
	std::vector<typename impl::adapter_ptr> block_slots;
	m_impl->m_mutex.lock();
	for(auto it=m_impl->m_slots.begin(); it!=m_impl->m_slots.end();)
	{
		if( (*it)->slot->m_is_valid() )
		{
			if( (*it)->slot->m_block )
				block_slots.emplace_back((*it)->slot);
			else
				nonblock_slots.emplace_back((*it)->slot);
			++it;
		}
		else
			it = m_impl->m_slots.erase(it);
	}
	m_impl->m_mutex.unlock();

	for(auto &slot : nonblock_slots)
		(*slot)(args...);

	for(auto &slot : block_slots)
		(*slot)(args...);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Args>
void signal_base<Derived,Func>::operator()(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	emit(std::forward<Args>(args)...);
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H