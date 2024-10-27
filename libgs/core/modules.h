
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

#include <libgs/core/global.h>
#include <libgs/core/coroutine.h>

namespace libgs { namespace concepts
{

template <typename Func>
concept modules_init_func =
	is_function_v<Func> and (function_traits<Func>::arg_count == 0) and (
		std::is_same_v<typename function_traits<Func>::return_type,void> or
		std::is_same_v<typename function_traits<Func>::return_type,std::future<void>> or
		std::is_same_v<typename function_traits<Func>::return_type,awaitable<void>>
	);

} //namespace concepts

class LIBGS_CORE_API modules
{
	LIBGS_DISABLE_COPY_MOVE(modules)

public:
	using level_t = size_t;
	static constexpr level_t level_0 = 0;
	static constexpr level_t level_1 = 1;
	static constexpr level_t level_2 = 2;
	static constexpr level_t level_3 = 3;
	static constexpr level_t level_4 = 4;
	static constexpr level_t level_5 = 5;
	static constexpr level_t level_6 = 6;

public:
	using init_func_t        = std::function<void()>;
	using future_init_func_t = std::function<std::future<void>()>;
	using await_init_func_t  = std::function<awaitable<void>()>;

	using func_obj_t = std::variant <
		init_func_t, future_init_func_t, await_init_func_t
	>;

public:
	/* awiatable as the return value of the initializer
	 * must be executed by co_run_init,
	 * run_init execution will ignore it.
	 * */
	static void reg_init(concepts::modules_init_func auto &&func, level_t level = level_6);

	static awaitable<void> co_run_init();
	static void run_init();

private:
	static void reg_init_p(func_obj_t func, level_t level);
};

#define LIBGS_MODULE_INIT(_level, _func) \
	LIBGS_REGISTRATION { \
		libgs::modules::reg_init(_func, static_cast<libgs::modules::level_t>(_level)); \
	}

#define LIBGS_MODULE_INIT_DEF(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_6, _func)

#define LIBGS_MODULE_INIT_0(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_0, _func)

#define LIBGS_MODULE_INIT_1(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_1, _func)

#define LIBGS_MODULE_INIT_2(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_2, _func)

#define LIBGS_MODULE_INIT_3(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_3, _func)

#define LIBGS_MODULE_INIT_4(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_4, _func)

#define LIBGS_MODULE_INIT_5(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_5, _func)

#define LIBGS_MODULE_INIT_6(_func) \
	LIBGS_MODULE_INIT(libgs::modules::level_6, _func)

} //namespace libgs
#include <libgs/core/detail/modules.h>


#endif //LIBGS_CORE_MODULES_H