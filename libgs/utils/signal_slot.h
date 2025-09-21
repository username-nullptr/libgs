
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

#ifndef LIBGS_UTILS_SIGNAL_SLOT_H
#define LIBGS_UTILS_SIGNAL_SLOT_H

#include <libgs/utils/global.h>

namespace libgs::utils
{

enum class slot_mode
{
	sync,         // Direct blocking call. (thread unsafe)
	async,        // Non-blocking call. (depends on the executor)
	backpressure  // Thread-safe blocking call. (depends on the executor; otherwise, it will cause a deadlock)
};

template <typename Derived, concepts::std_func_temp Func>
class LIBGS_UTILS_TAPI signal_base
{
	LIBGS_DISABLE_COPY_MOVE(signal_base)

public:
	using derived_t = crtp_derived_t<Derived,signal_base>;
	using function_t = Func;

	signal_base();
	~signal_base();

public:
	template <slot_mode Mode, concepts::function Func0>
	static constexpr bool is_slot_v = []() consteval
	{
		using func0_tr = function_traits<Func0     >;
		using func_tr  = function_traits<function_t>;

		if constexpr( Mode == slot_mode::sync and is_awaitable_v<typename func0_tr::return_type> )
			return false;
		else
		{
			return func0_tr::arg_count <= func_tr::arg_count and
				[]<size_t...Is>(std::index_sequence<Is...>)
				{
					return (std::is_convertible_v <
						typename func_tr ::template arg_type_t<Is>,
						typename func0_tr::template arg_type_t<Is>
					> && ...);
				}
				(std::make_index_sequence<func0_tr::arg_count>{});
		}
	}();

	template <slot_mode Mode, concepts::function Func0>
	static constexpr bool is_global_slot_v =
		not function_traits<Func0>::is_member_func and
		is_slot_v<Mode, Func0>;

	template <typename Obj>
	static constexpr bool is_observer_v =
		is_shared_ptr_v<std::remove_cvref_t<Obj>>;

	template <slot_mode Mode, typename Obj, concepts::function Func0>
	requires is_observer_v<Obj>
	static constexpr bool is_obj_slot_v = []() consteval
	{
		using func0_tr = function_traits<Func0>;
		if constexpr( func0_tr::is_member_func )
		{
			using obj_t = std::remove_cvref_t<Obj>::element_type;
			if constexpr( std::is_same_v<typename func0_tr::class_t, obj_t> )
				return is_slot_v<Mode,Func0>;
			else
				return false;
		}
		else
			return is_global_slot_v<Mode,Func0>;
	}();

	template <slot_mode Mode, concepts::function...Funcs>
	requires (sizeof...(Funcs) > 0)
	static constexpr bool is_global_slots_v = (is_global_slot_v<Mode,Funcs> && ...);

	template <slot_mode Mode, typename Obj, concepts::function...Funcs>
	requires (is_observer_v<Obj> and sizeof...(Funcs) > 0)
	static constexpr bool is_obj_slots_v = (is_obj_slot_v<Mode,Obj,Funcs> && ...);

	template <typename...Args>
	static constexpr bool is_callable_v =
		requires(std::function<function_t> sig, Args&&...args) {
			sig(std::forward<Args>(args)...);
		};

public:
	template <slot_mode Mode, typename...Funcs>
	derived_t &connect(Funcs&&...funcs) noexcept
		requires is_global_slots_v<Mode,Funcs...>;

	template <slot_mode Mode, typename Obj, typename...Funcs>
	derived_t &connect(Obj &&observer, Funcs&&...funcs)
		requires is_obj_slots_v<Mode,Obj,Funcs...>;

	template <slot_mode Mode, concepts::sched Exec0, typename...Funcs>
	derived_t &connect(Exec0 &&exec, Funcs&&...funcs) noexcept
		requires is_global_slots_v<Mode,Funcs...>;

	template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename...Funcs>
	derived_t &connect(Obj &&observer, Exec0 &&exec, Funcs&&...funcs)
		requires is_obj_slots_v<Mode,Obj,Funcs...>;

public:
	template <typename...Funcs>
	derived_t &connect(Funcs&&...funcs) noexcept
		requires is_global_slots_v<slot_mode::sync,Funcs...>;

	template <typename Obj, typename...Funcs>
	derived_t &connect(Obj &&observer, Funcs&&...funcs)
		requires is_obj_slots_v<slot_mode::sync,Obj,Funcs...>;

	template <concepts::sched Exec0, typename...Funcs>
	derived_t &connect(Exec0 &&exec, Funcs&&...funcs) noexcept
		requires is_global_slots_v<slot_mode::sync,Funcs...>;

	template <typename Obj, concepts::sched Exec0, typename...Funcs>
	derived_t &connect(Obj &&observer, Exec0 &&exec, Funcs&&...funcs)
		requires is_obj_slots_v<slot_mode::sync,Obj,Funcs...>;

public:
	template <typename...Funcs>
	derived_t &disconnect(Funcs&&...funcs) noexcept
		requires is_global_slots_v<slot_mode::async,Funcs...>;

	template <typename Obj, typename...Funcs>
	derived_t &disconnect(const Obj &observer, Funcs&&...funcs)
		requires is_obj_slots_v<slot_mode::async,Obj,Funcs...>;

	derived_t &disconnect() noexcept;

	template <typename Obj>
	derived_t &disconnect(const Obj &observer)
		requires is_observer_v<Obj>;

public:
	template <typename...Args>
	void emit(Args&&...args) const noexcept
		requires is_callable_v<Args...>;

	template <typename...Args>
	[[nodiscard]] awaitable<void> co_emit(Args&&...args) const noexcept
		requires is_callable_v<Args...>;

	template <typename...Args>
	void operator()(Args&&...args) const noexcept
		requires is_callable_v<Args...>;

private:
	class impl;
	impl *m_impl;
};

template <concepts::std_func_temp Func>
using signal = signal_base<void,Func>;

} //namespace libgs::utils
#include <libgs/utils/detail/signal_slot.h>


#endif //LIBGS_UTILS_SIGNAL_SLOT_H