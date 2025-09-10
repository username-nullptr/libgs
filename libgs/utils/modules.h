
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

#ifndef LIBGS_UTILS_MODULES_H
#define LIBGS_UTILS_MODULES_H

#include <libgs/core/string_vector.h>
#include <libgs/utils/global.h>

namespace libgs::utils
{

class LIBGS_UTILS_API modules
{
	LIBGS_DISABLE_COPY_MOVE(modules)

public:
	struct dependency
	{
		string_vector before;
		string_vector after;
	};
	template <typename Func>
	static constexpr bool init_func_v =
		concepts::callable_ret<Func,bool> or concepts::callable_ret<Func,bool,string_vector> or
		concepts::callable_void<Func> or concepts::callable_void<Func,string_vector>;

	template <typename Func>
	static void reg_init(std::string name, dependency depy, Func &&func)
		requires init_func_v<Func>;

	template <typename Func>
	static void reg_init(std::string name, Func &&func)
		requires init_func_v<Func>;

public:
	struct unexpected
	{
		std::vector<std::string> failures;
		std::vector<std::string> unregistered;
		std::vector<std::string> children;
	};

	template <typename Func>
	static constexpr bool init_callable_v =
		concepts::callable<Func,unexpected> or concepts::callable<Func>;

	template <typename Token>
	static constexpr bool init_token_v =
		std::is_same_v<std::remove_cvref_t<Token>, use_sync_t> or
		std::is_same_v<std::remove_cvref_t<Token>, use_future_t> or
		std::is_same_v<std::remove_cvref_t<Token>, detached_t> or
		init_callable_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(int argc, const char **argv, Token &&token = {})
		requires init_token_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(const string_vector &args, Token &&token = {})
		requires init_token_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(Token &&token = {})
		requires init_token_v<Token>;

	template <typename Func>
	static auto do_init(int argc, const char **argv,
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

	template <typename Func>
	static auto do_init(const string_vector &args,
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

	template <typename Func>
	static auto do_init (
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

public:
	[[nodiscard]] static std::string sprint() noexcept;
};

#define LIBGS_UTILS_MODULE_INIT(_name, ...) \
	LIBGS_REGISTRATION { \
		libgs::utils::modules::reg_init(_name, __VA_ARGS__); \
	}

} //namespace libgs::utils
#include <libgs/utils/detail/modules.h>


#endif //LIBGS_UTILS_MODULES_H