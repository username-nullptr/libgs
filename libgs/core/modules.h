
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

#ifndef LIBGS_CORE_MODULES_H
#define LIBGS_CORE_MODULES_H

#include <libgs/core/string_vector.h>

namespace libgs { namespace concepts
{

template <typename Func>
concept modules_init_func0 = requires(Func func) {
	requires not is_awaitable_v<decltype(func())>;
};

template <typename Func>
concept modules_init_func1 = requires(Func func, string_vector args) {
	requires not is_awaitable_v<decltype(func(args))>;
};

template <typename Func>
concept modules_init_func =
	modules_init_func0<Func> or
	modules_init_func1<Func>;

template <typename Token>
concept modules_init_token =
	std::is_same_v<std::remove_cvref_t<Token>, use_sync_t> or
	std::is_same_v<std::remove_cvref_t<Token>, use_future_t> or
	std::is_same_v<std::remove_cvref_t<Token>, detached_t> or
	callable<Token>;

} //namespace concepts

class LIBGS_CORE_API modules
{
	LIBGS_DISABLE_COPY_MOVE(modules)

public:
	struct dependency
	{
		string_vector before;
		string_vector after;
	};
	static void reg_init(std::string name, dependency depy,
		concepts::modules_init_func auto &&func
	);
	static void reg_init(std::string name,
		concepts::modules_init_func auto &&func
	);

public:
	template <concepts::modules_init_token Token = use_sync_t>
	[[nodiscard]] static auto do_init(int argc, const char **argv, Token &&token = {});

	template <concepts::modules_init_token Token = use_sync_t>
	[[nodiscard]] static auto do_init(const string_vector &args, Token &&token = {});

	template <concepts::modules_init_token Token = use_sync_t>
	[[nodiscard]] static auto do_init(Token &&token = {});

	[[nodiscard]] static auto do_init(int argc, const char **argv,
		concepts::sched auto &&exec, concepts::callable auto &&callback
	);
	[[nodiscard]] static auto do_init(const string_vector &args,
		concepts::sched auto &&exec, concepts::callable auto &&callback
	);
	[[nodiscard]] static auto do_init (
		concepts::sched auto &&exec, concepts::callable auto &&callback
	);

public:
	[[nodiscard]] static std::string sprint() noexcept;
};

#define LIBGS_MODULE_INIT(_name, ...) \
	LIBGS_REGISTRATION { \
		libgs::modules::reg_init(_name, __VA_ARGS__); \
	}

} //namespace libgs
#include <libgs/core/detail/modules.h>


#endif //LIBGS_CORE_MODULES_H