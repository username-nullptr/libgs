
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

#define libgs_exe  libgs::execution::instance()

namespace libgs::execution
{

using executor_type = typename asio::io_context::executor_type;

[[nodiscard]] LIBGS_CORE_VAPI asio::io_context &io_context();
[[nodiscard]] LIBGS_CORE_VAPI executor_type get_executor() noexcept;

LIBGS_CORE_VAPI int exec();
LIBGS_CORE_VAPI void exit(int code = 0);

[[nodiscard]] LIBGS_CORE_VAPI bool is_run();

template<typename T, concept_schedulable Exec = asio::io_context>
void delete_later(T *obj, Exec &exec = io_context());

} //namespace libgs::execution
#include <libgs/core/detail/execution.h>


#endif //LIBGS_CORE_EXECUTION_H
