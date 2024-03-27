
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

#ifndef LIBGS_CORE_DETAIL_EXECUTION_H
#define LIBGS_CORE_DETAIL_EXECUTION_H

#include <cstdio>

namespace libgs
{

namespace _execution::detail
{

inline std::atomic_int g_exit_code {0};

inline std::atomic_bool g_run_flag {false};

} //namespace _execution::detail

inline execution &execution::instance()
{
	static execution g_obj;
	return g_obj;
}

inline asio::io_context &execution::io_context()
{
	static asio::io_context ioc;
	return ioc;
}

inline typename execution::executor_type execution::get_executor() noexcept
{
	return io_context().get_executor();
}

inline int execution::exec()
{
	using namespace _execution::detail;
	if( g_run_flag )
		throw runtime_error("libgs::execution::exec: not reentrant.");

	g_run_flag = true;
	auto &ioc = io_context();

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

inline void execution::exit(int code)
{
	using namespace _execution::detail;
	if( not g_run_flag )
		return ;

	g_exit_code = code;
	g_run_flag = false;
	io_context().stop();
}

inline bool execution::is_run() const
{
	return _execution::detail::g_run_flag;
}

inline asio::io_context &io_context()
{
	return execution::instance().io_context();
}

template<typename T, concept_execution Exec>
void delete_later(T *obj, Exec &exec)
{
	post(exec, [obj]{
		delete obj;
	});
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_EXECUTION_H
