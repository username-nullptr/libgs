
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
#include <libgs/core/execution.h>

namespace libgs::utils
{

template <typename Derived, concepts::std_func_temp Func, concepts::exec Exec = asio::any_io_executor>
class LIBGS_UTILS_TAPI basic_signal_base
{
	LIBGS_DISABLE_COPY_MOVE(basic_signal_base)

public:
	using derived_t = crtp_derived_t<Derived,basic_signal_base>;
	using function_t = Func;
	using executor_t = Exec;

	template <concepts::match_sched<Exec> Exec0 = io_context_t&>
	explicit basic_signal_base(Exec0 &&exec = io_context());
	~basic_signal_base();

public:
	template <concepts::function Func0>
	static constexpr bool is_slot_v = []() consteval
	{
		using func_tr0 = function_traits<Func0     >;
		using func_tr  = function_traits<function_t>;

		return func_tr0::arg_count <= func_tr::arg_count and
			[]<size_t...Is>(std::index_sequence<Is...>)
			{
				return (std::is_convertible_v <
					typename func_tr ::template arg_type_t<Is>,
					typename func_tr0::template arg_type_t<Is>
				> && ...);
			}
			(std::make_index_sequence<func_tr0::arg_count>{});
	}();

	template <concepts::function...Funcs> requires (sizeof...(Funcs) > 0)
	static constexpr bool is_slots_v = (is_slot_v<Funcs> && ...);

	template <typename...Args>
	static constexpr bool is_callable_v =
		requires(std::function<function_t> sig, Args&&...args) {
			sig(std::forward<Args>(args)...);
		};

public:
	template <typename...Funcs>
	derived_t &connect(Funcs&&...funcs) noexcept
		requires is_slots_v<Funcs...>;

	template <concepts::sched Exec0, typename...Funcs>
	derived_t &connect(Exec0 &&exec, Funcs&&...funcs) noexcept
		requires is_slots_v<Funcs...>;

	template <typename...Func0>
	derived_t &disconnect(Func0&&...funcs) noexcept
		requires is_slots_v<Func0...>;

	derived_t &disconnect() noexcept;

public:
	template <typename...Args>
	void emit(Args&&...args) const noexcept
		requires is_callable_v<Args...>;

	template <typename...Args>
	void operator()(Args&&...args) const noexcept
		requires is_callable_v<Args...>;

private:
	class impl;
	impl *m_impl;
};

template <typename Derived, concepts::std_func_temp Func>
using signal_base = basic_signal_base<Derived,Func>;

template <concepts::std_func_temp Func, concepts::exec Exec = asio::any_io_executor>
using basic_signal = basic_signal_base<void,Func,Exec>;

template <concepts::std_func_temp Func>
using signal = basic_signal<Func>;

} //namespace libgs::utils
#include <libgs/utils/detail/signal_slot.h>


#endif //LIBGS_UTILS_SIGNAL_SLOT_H