
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

#ifndef LIBGS_CORE_LOG_BUFFER_H
#define LIBGS_CORE_LOG_BUFFER_H

#include <libgs/core/log/scheduler.h>
#include <chrono>
#include <cstdio>

namespace libgs::log
{

template <concept_char_type CharT>
class LIBGS_CORE_TAPI basic_buffer
{
	using ocontext = basic_output_context<CharT>;

public:
	explicit basic_buffer(output_type t);
	basic_buffer(const basic_buffer &other);
	basic_buffer(basic_buffer &&other) noexcept;
	~basic_buffer();

public:
	basic_buffer &operator=(const basic_buffer &other);
	basic_buffer &operator=(basic_buffer &&other) noexcept;

public:
	template <typename T>
	basic_buffer &write(T &&msg);

	template <typename T>
	basic_buffer &operator<<(T &&msg);

public:
	const ocontext &context() const;
	ocontext &context();

private:
	struct data
	{
		output_type type;
		ocontext context;
		std::basic_string<CharT> buffer;
	}
	*m_data;
};

using buffer = basic_buffer<char>;

using wbuffer = basic_buffer<wchar_t>;

} //namespace libgs::log
#include <libgs/core/log/detail/buffer.h>


#endif //LIBGS_CORE_LOG_BUFFER_H
