
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

#include "execution.h"

namespace libgs::execution
{

static std::atomic_int g_exit_code {0};

static std::atomic_bool g_run_flag {false};

context_t &context() noexcept
{
	// Don't destruct it.
	// An exception occurs when the main function exits in Win10.
	static auto *g_ioc = new context_t();
	return *g_ioc;
}

executor_t get_executor() noexcept
{
	return context().get_executor();
}

int exec()
{
	if( g_run_flag )
		throw runtime_error("libgs::execution::exec: not reentrant.");

	g_run_flag = true;
	auto &ioc = context();

	asio::io_context::work io_work(ioc); LIBGS_UNUSED(io_work);
	for(;;)
	{
		ioc.run();
		if( not g_run_flag )
			break;
		ioc.restart();
	}
	return g_exit_code;
}

void exit(int code)
{
	if( not g_run_flag )
		return ;

	g_exit_code = code;
	g_run_flag = false;
	context().stop();
}

bool is_run()
{
	return g_run_flag;
}

} //namespace libgs::execution