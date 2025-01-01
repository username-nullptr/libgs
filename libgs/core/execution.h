
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

#ifndef LIBGS_CORE_EXECUTION_H
#define LIBGS_CORE_EXECUTION_H

#include <libgs/core/global.h>

namespace libgs
{

using io_context_t = asio::io_context;
using io_executor_t = io_context_t::executor_type;

[[nodiscard]] LIBGS_CORE_API io_context_t &io_context() noexcept;
[[nodiscard]] LIBGS_CORE_API io_executor_t get_executor() noexcept;

LIBGS_CORE_API int exec();
LIBGS_CORE_API void exit(int code = 0);

[[nodiscard]] LIBGS_CORE_API bool is_run();

namespace concepts
{

template <typename Work>
concept dispatch_work = []() consteval -> bool
{
	if constexpr( callable<Work> )
		return true;
	else
	{
		using work_t = std::remove_cvref_t<Work>;
		return awaitable_type<work_t> and std::is_rvalue_reference_v<Work&&>;
	}
}();

template <typename Token, typename Work>
concept dispatch_token = []() consteval -> bool
{
	if constexpr( not dispatch_work<Work> )
		return false;
	else
	{
		using work_t = std::remove_cvref_t<Work>;
		if constexpr( awaitable_type<work_t> )
		{
			using return_t = typename work_t::value_type;
			if constexpr( std::is_void_v<return_t> )
				return opt_token<Token, std::exception_ptr>;
			else
				return opt_token<Token, std::exception_ptr, return_t>;
		}
		else
		{
			if constexpr( not callable<Work> )
				return false;
			else
			{
				using return_t = std::invoke_result_t<Work>;
				if constexpr( is_awaitable_v<return_t> )
				{
					using nreturn_t = typename return_t::value_type;
					if constexpr( std::is_void_v<nreturn_t> )
						return opt_token<Token, std::exception_ptr>;
					else
						return opt_token<Token, std::exception_ptr, nreturn_t>;
				}
				else if constexpr( std::is_void_v<return_t> )
					return opt_token<Token, std::exception_ptr>;
				else
					return opt_token<Token, std::exception_ptr, return_t>;
			}
		}
	}
}();

} //namespace concepts

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token = const detached_t&>
LIBGS_CORE_TAPI auto dispatch (
	concepts::schedulable auto &&exec, Work &&work, Token &&token = detached
);

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token = const detached_t&>
LIBGS_CORE_TAPI auto dispatch (
	Work &&work, Token &&token = detached
);

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token = const detached_t&>
LIBGS_CORE_TAPI auto post (
	concepts::schedulable auto &&exec, Work &&work, Token &&token = detached
);

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token = const detached_t&>
LIBGS_CORE_TAPI auto post (
	Work &&work, Token &&token = detached
);

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
LIBGS_CORE_TAPI auto local_dispatch (
	concepts::execution_context auto &exec, Work &&work, Token &&token
);

template <typename Work>
LIBGS_CORE_TAPI auto local_dispatch(concepts::execution_context auto &exec, Work &&work)
	requires concepts::dispatch_token<detached_t, Work>;

template <concepts::dispatch_work Work, concepts::dispatch_token<Work> Token>
LIBGS_CORE_TAPI auto local_dispatch (
	Work &&work, Token &&token
);

template <typename Work>
LIBGS_CORE_TAPI auto local_dispatch(Work &&work)
	requires concepts::dispatch_token<detached_t, Work>;

namespace concepts
{

template <typename Func, typename Handler>
concept async_wake_up = std::is_rvalue_reference_v<Handler> and (
	callable<Func,Handler&&> or callable<Func,Handler&&,asio::any_io_executor>
);

} //namespace concepts

template <concepts::execution Exec, typename...Args>
class LIBGS_CORE_TAPI basic_async_work
{
	LIBGS_DISABLE_COPY_MOVE(basic_async_work)

public:
	basic_async_work() = default;
	using handler_t = asio::detail::awaitable_handler<Exec,Args...>;

	template <concepts::async_opt_token<Args...> Token = const use_awaitable_t&>
	[[nodiscard]] static auto handle (
		concepts::schedulable auto &&exec, concepts::async_wake_up<handler_t&&> auto &&wake_up,
		Token &&token = use_awaitable
	);

	template <concepts::async_opt_token<Args...> Token = const use_awaitable_t&>
	[[nodiscard]] static auto handle (
		concepts::async_wake_up<handler_t&&> auto &&wake_up,
		Token &&token = use_awaitable
	);
};

template <typename...Args>
using async_work = basic_async_work<asio::any_io_executor, Args...>;

LIBGS_CORE_TAPI void delete_later(const concepts::execution auto &exec, auto *obj);
LIBGS_CORE_TAPI void delete_later(concepts::execution_context auto &exec, auto *obj);
LIBGS_CORE_TAPI void delete_later(auto *obj);

template <typename NativeExec>
struct is_match_default_execution
{
	static constexpr bool value =
		is_execution_v<NativeExec> and
		requires {
			NativeExec(get_executor());
		};
};

template <typename NativeExec>
constexpr bool is_match_default_execution_v = is_match_default_execution<NativeExec>::value;

namespace concepts
{

template <typename NativeExec>
concept match_default_execution = is_match_default_execution_v<NativeExec>;

}} //namespace libgs::concepts
#include <libgs/core/detail/execution.h>


#endif //LIBGS_CORE_EXECUTION_H
