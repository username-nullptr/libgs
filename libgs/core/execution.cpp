// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
	{
		runtime_error::loc_throw (
			"libgs::execution::exec: not reentrant."
		);
	}
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
