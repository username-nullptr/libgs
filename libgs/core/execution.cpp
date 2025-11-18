
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

namespace libgs
{

static std::atomic_int g_exit_code {0};

static std::atomic_bool g_run_flag {false};

using io_worker_t = asio::executor_work_guard<io_context_t::executor_type>;
static std::unique_ptr<io_worker_t> g_io_worker;

io_context_t &io_context() noexcept
{
	// Don't destruct it.
	// An exception occurs when the main function exits in Win10.
	static auto *g_ioc = new io_context_t();
	return *g_ioc;
}

io_executor_t get_executor() noexcept
{
	return io_context().get_executor();
}

int exec()
{
	if( g_run_flag )
		throw runtime_error("libgs::execution::exec: not reentrant.");

	g_run_flag = true;
	auto &ioc = io_context();
	ioc.restart();

	g_io_worker = std::make_unique<io_worker_t>(ioc.get_executor());
	for(;;)
	{
		ioc.run();
		ioc.restart();
		if( not g_run_flag )
			break;
	}
	return g_exit_code;
}

void exec(io_context_t &ioc)
{
	io_worker_t work(ioc.get_executor()); (void)work;
	ioc.run();
}

void exec_detach(io_context_t &ioc)
{
    std::thread([&ioc]{exec(ioc);}).detach();
}

void exit(int code)
{
	if( g_run_flag )
	{
		g_exit_code = code;
		g_run_flag = false;
	}
	io_context().stop();
	g_io_worker.reset();

	while( g_run_flag )
	{
		g_run_flag = false;
		io_context().stop();
	}
}

bool is_run()
{
	return g_run_flag;
}



} //namespace libgs