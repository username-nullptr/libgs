
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
concept modules_init_func = []() consteval -> bool
{
	if constexpr( is_function_v<Func> )
	{
		using return_t = function_traits<Func>::return_type;
		if constexpr( std::is_same_v<return_t,void> or
					  std::is_same_v<return_t,std::future<void>> or
					  std::is_same_v<return_t,asio::awaitable<void>> )
		{
			constexpr auto arg_count = function_traits<Func>::arg_count;
			if constexpr( arg_count == 0 )
				return true;
			else if constexpr( arg_count == 1 )
			{
				using arg_t = std::tuple_element_t<0, typename function_traits<Func>::arg_types>;
				return std::is_same_v<std::remove_cvref_t<arg_t>, string_vector>;
			}
			else
				return false;
		}
	}
	return false;
}();

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
	using func0_t        = std::function<void()>;
	using future_func0_t = std::function<std::future<void>()>;
	using await_func0_t  = std::function<awaitable<void>()>;

	using func1_t        = std::function<void(string_vector&)>;
	using future_func1_t = std::function<std::future<void>(string_vector&)>;
	using await_func1_t  = std::function<awaitable<void>(string_vector&)>;

	using func_obj_t = std::variant <
		func0_t, future_func0_t, await_func0_t,
		func1_t, future_func1_t, await_func1_t
	>;

public:
	template <typename T>
	static constexpr bool is_level_v =
		concepts::integral_p<T> or std::is_enum_v<std::remove_cvref_t<T>>;

	template <typename T = level_t>
	static void reg_init(concepts::modules_init_func auto &&func, T level = level_6)
		requires is_level_v<T>;

	static string_vector do_init(const string_vector &args = {});
	static string_vector do_init(int argc, const char *argv[]);

private:
	class impl;
};

#define LIBGS_MODULE_INIT(_level, _func) \
	LIBGS_REGISTRATION { \
		libgs::modules::reg_init(_func, _level); \
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