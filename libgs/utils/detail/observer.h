// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
class LIBGS_UTILS_TAPI basic_observer_base<Derived,Exec,Funcs...>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename Exec0>
	impl(uint64_t id, Exec0 &&exec) :
		m_exec(get_executor_helper(std::forward<Exec0>(exec))), m_id(id) {}

public:
	asio::any_io_executor m_exec {};
	callbacks_t m_callbacks {};
	uint64_t m_id = 0;
};

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
template <concepts::match_sched<Exec> Exec0>
basic_observer_base<Derived,Exec,Funcs...>::basic_observer_base(uint64_t id, Exec0 &&exec) :
	m_impl(new impl(id, std::forward<Exec0>(exec)))
{
	detail::observer::mutex().lock();
	[[maybe_unused]] auto [it, inserted] =
		detail::observer::map()[typeid(derived_t).hash_code()]
		.emplace(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	assert(inserted);
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
basic_observer_base<Derived,Exec,Funcs...>::~basic_observer_base()
{
	detail::observer::mutex().lock();
	detail::observer::map()[typeid(derived_t).hash_code()]
		.erase(static_cast<void*>(m_impl));

	detail::observer::mutex().unlock();
	delete m_impl;
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
template <typename...Args0>
auto basic_observer_base<Derived,Exec,Funcs...>::make(Args0&&...args) -> ptr_t
	requires concepts::constructible<derived_t,Args0...>
{
	return std::make_shared<derived_t>(std::forward<Args0>(args)...);
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
template <size_t Idx>
auto basic_observer_base<Derived,Exec,Funcs...>::on_triggered(callback_t<Idx> func)
	-> ptr_t requires idx_valid_v<Idx>
{
	std::get<Idx>(m_impl->m_callbacks).emplace_back(std::move(func));
	return this->shared_from_this();
}

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
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
			auto call = [call_exec = std::move(exec)]<typename...Args>(auto callback, Args&&...call_args)
			{
				using return_t = decltype(callback(std::forward<Args>(call_args)...));
				if constexpr( is_awaitable_v<return_t> )
				{
					libgs::dispatch(call_exec, [slot = std::move(callback),
						...slot_args = std::forward<Args>(call_args)]
					() mutable -> awaitable<void> {
						co_await slot(std::move(slot_args)...);
						co_return ;
					});
				}
				else
				{
					libgs::dispatch(call_exec, [slot = std::move(callback),
						...slot_args = std::forward<Args>(call_args)]() mutable {
						slot(std::move(slot_args)...);
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

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs>
requires (sizeof...(Funcs) > 0)
auto basic_observer_base<Derived,Exec,Funcs...>::get_executor() noexcept -> executor_t
{
	return m_impl->m_exec;
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_OBSERVER_H
