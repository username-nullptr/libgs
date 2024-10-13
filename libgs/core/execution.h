
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

namespace libgs { namespace execution
{

using context_t = asio::io_context;
using executor_t = typename context_t::executor_type;

[[nodiscard]] LIBGS_CORE_API context_t &context();
[[nodiscard]] LIBGS_CORE_API executor_t get_executor() noexcept;

LIBGS_CORE_API int exec();
LIBGS_CORE_API void exit(int code = 0);

[[nodiscard]] LIBGS_CORE_API bool is_run();

template<concepts::execution Exec = executor_t>
LIBGS_CORE_TAPI void delete_later(auto *obj, const Exec &exec = get_executor());

template<concepts::execution_context Exec = context_t>
LIBGS_CORE_TAPI void delete_later(auto *obj, Exec &exec = context());

} //namespace execution

namespace concepts
{

template <typename NativeExec>
concept match_default_execution =
	is_execution_v<NativeExec> and requires { NativeExec(execution::get_executor()); };

} //namespace concepts

template <typename NativeExec>
struct is_match_default_execution {
	static constexpr bool value = concepts::match_default_execution<NativeExec>;
};

template <typename NativeExec>
constexpr bool is_match_default_execution_v = is_match_default_execution<NativeExec>::value;

} //namespace libgs
#include <libgs/core/detail/execution.h>


#endif //LIBGS_CORE_EXECUTION_H
