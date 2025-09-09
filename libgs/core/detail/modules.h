
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

#ifndef LIBGS_CORE_DETAIL_MODULES_H
#define LIBGS_CORE_DETAIL_MODULES_H

#include <libgs/core/execution.h>

namespace libgs { namespace detail
{

class LIBGS_CORE_API modules
{
	LIBGS_DISABLE_COPY_MOVE(modules);

public:
	using func0_t = std::function<void()>;
	using func1_t = std::function<void(const string_vector&)>;

	enum class state {
		finished, not_register
	};
	using func_obj_t = std::variant <
		func0_t, func1_t, state
	>;
	static void reg_init(std::string name,
		libgs::modules::dependency depy, func_obj_t func
	);
	static void do_init(const string_vector &args,
		std::function<void()> callback = {}
	);
};

} //namespace detail

void modules::reg_init(std::string name, dependency depy, concepts::modules_init_func auto &&func)
{
	using Func = decltype(func);
	if constexpr( concepts::modules_init_func0<Func> )
	{
		detail::modules::reg_init(std::move(name), std::move(depy),
			detail::modules::func0_t(std::forward<Func>(func))
		);
	}
	else /* if constexpr( concepts::modules_init_func1<Func> ) */
	{
		detail::modules::reg_init(std::move(name), std::move(depy),
			detail::modules::func1_t(std::forward<Func>(func))
		);
	}
}

void modules::reg_init(std::string name, concepts::modules_init_func auto &&func)
{
	using Func = decltype(func);
	reg_init(std::move(name), {}, std::forward<Func>(func));
}

template <concepts::modules_init_token Token>
auto modules::do_init(int argc, const char *argv[], Token &&token)
{
	return do_init({argv, argv + argc}, std::forward<Token>(token));
}

template <concepts::modules_init_token Token>
auto modules::do_init(const string_vector &args, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_sync_opt_token_v<token_t> )
		detail::modules::do_init(args);

	else if constexpr( is_use_future_v<token_t> )
	{
		std::promise<void> promise;
		auto future = promise.get_future();
		detail::modules::do_init(args, [promise = std::move(promise)]() mutable {
			promise.set_value();
		});
		return future;
	}
	else if constexpr( is_detached_v<token_t> )
		detail::modules::do_init(args, []{});
	else
		detail::modules::do_init(args, std::forward<Token>(token));
}

template <concepts::modules_init_token Token>
auto modules::do_init(Token &&token)
{
	return do_init({}, std::forward<Token>(token));
}

auto modules::do_init
(int argc, const char **argv, concepts::sched auto &&exec, concepts::callable auto &&callback)
{
	using Exec = decltype(exec);
	using Func = decltype(callback);
	return do_init(argc, argv,
	[exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(callback)] {
		libgs::dispatch(exec, std::move(func));
	});
}

auto modules::do_init
(const string_vector &args, concepts::sched auto &&exec, concepts::callable auto &&callback)
{
	using Exec = decltype(exec);
	using Func = decltype(callback);
	return do_init(args,
	[exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(callback)] {
		libgs::dispatch(exec, std::move(func));
	});
}

auto modules::do_init
(concepts::sched auto &&exec, concepts::callable auto &&callback)
{
	using Exec = decltype(exec);
	using Func = decltype(callback);
	return do_init(
	[exec = get_executor_helper(std::forward<Exec>(exec)), func = std::forward<Func>(callback)] {
		libgs::dispatch(exec, std::move(func));
	});
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_MODULES_H