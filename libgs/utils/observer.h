
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

#ifndef LIBGS_UTILS_OBSERVER_H
#define LIBGS_UTILS_OBSERVER_H

#include <libgs/core/execution.h>
#include <libgs/utils/global.h>

namespace libgs::utils
{

template <typename Derived, concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
class LIBGS_UTILS_TAPI basic_observer_base : public std::enable_shared_from_this<
	crtp_derived_t<Derived, basic_observer_base<Derived,Exec,Funcs...>>>
{
	LIBGS_DISABLE_COPY_MOVE(basic_observer_base)
	using derived_t = crtp_derived_t<Derived,basic_observer_base>;

public:
	using ptr_t = std::shared_ptr<derived_t>;
	using callbacks_t = std::tuple<std::vector<std::function<Funcs>>...>;
	using callbacks_tuple_t = std::tuple<std::function<Funcs>...>;
	using executor_t = Exec;

	template <size_t Idx>
	using callback_t = std::tuple_element_t<Idx,callbacks_tuple_t>;

	template <size_t Idx>
	static constexpr bool idx_valid_v = Idx < sizeof...(Funcs);

public:
	template <concepts::match_sched<Exec> Exec0 = io_context_t&>
	explicit basic_observer_base(uint64_t id, Exec0 &&exec = io_context());
	virtual ~basic_observer_base();

public:
	template <typename...Args0>
	[[nodiscard]] static ptr_t make(Args0&&...args) requires
		concepts::constructible<derived_t,Args0...>;

	template <size_t Idx>
	ptr_t on_triggered(callback_t<Idx> func)
		requires idx_valid_v<Idx>;

	template <size_t Idx, typename...Args0>
	static void trigger(uint64_t id, Args0&&...args) requires
		idx_valid_v<Idx> and concepts::callable<callback_t<Idx>,Args0...>;

	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	impl *m_impl = nullptr;
};

template <concepts::exec Exec, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
using basic_observer = basic_observer_base<void, Exec, Funcs...>;

template <typename Derived, concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
using observer_base = basic_observer_base<Derived, asio::any_io_executor, Funcs...>;

template <concepts::std_func_temp...Funcs> requires (sizeof...(Funcs) > 0)
using observer = basic_observer<asio::any_io_executor, Funcs...>;

} //namespace libgs::utils
#include <libgs/utils/detail/observer.h>


#endif //LIBGS_UTILS_OBSERVER_H
