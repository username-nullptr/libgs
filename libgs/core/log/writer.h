
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

#ifndef LIBGS_CORE_LOG_WRITER_H
#define LIBGS_CORE_LOG_WRITER_H

#include <libgs/core/log/context.h>

namespace libgs::log
{

class LIBGS_CORE_API writer
{
	LIBGS_DISABLE_COPY_MOVE(writer)

	using type = output_type;
	using duration = std::chrono::milliseconds;

public:
	writer();
	~writer();

public:
	static writer &instance();
	void write(type type, output_context &&runtime_context, std::string &&msg);
	void write(type type, output_wcontext &&runtime_context, std::wstring &&msg);

public:
	void fatal(output_context &&runtime_context, const std::string &msg);
	void fatal(output_wcontext &&runtime_context, const std::wstring &msg);

#if defined(__WINNT__) || defined(_WINDOWS)
	void exit();
#endif //Windows
};

} //namespace libgs::log


#endif //LIBGS_CORE_LOG_WRITER_H
