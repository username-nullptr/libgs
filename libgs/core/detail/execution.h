
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

namespace libgs::execution
{

namespace detail
{

struct LIBGS_CORE_API execution_impl
{
	inline static asio::io_context ioc;
	inline static std::atomic_int exit_code {0};
	inline static std::atomic_bool run_flag {false};
};

} //namespace detail

inline asio::io_context &io_context()
{
	return detail::execution_impl::ioc;
}

inline executor_type get_executor() noexcept
{
	return io_context().get_executor();
}

inline int exec()
{
	using namespace detail;
	if( execution_impl::run_flag )
		throw runtime_error("libgs::execution::exec: not reentrant.");

	execution_impl::run_flag = true;
	auto &ioc = io_context();

	asio::io_context::work io_work(ioc); LIBGS_UNUSED(io_work);
	for(;;)
	{
		ioc.run();
		if( not execution_impl::run_flag )
			break;
		ioc.restart();
	}
	return execution_impl::exit_code;
}

inline void exit(int code)
{
	using namespace detail;
	if( not execution_impl::run_flag )
		return ;

	execution_impl::exit_code = code;
	execution_impl::run_flag = false;
	io_context().stop();
}

inline bool is_run()
{
	return detail::execution_impl::run_flag;
}

template<typename T, concept_execution Exec>
void delete_later(T *obj, Exec &exec)
{
	post(exec, [obj]{
		delete obj;
	});
}

} //namespace libgs::execution


#endif //LIBGS_CORE_DETAIL_EXECUTION_H
