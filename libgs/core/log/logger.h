
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

#ifndef LIBGS_CORE_LOG_LOGGER_H
#define LIBGS_CORE_LOG_LOGGER_H

#include <libgs/core/log/buffer.h>

namespace libgs::log
{

class logger_impl;

template <concept_char_type CharT>
class LIBGS_CORE_TAPI basic_logger
{
	LIBGS_DISABLE_COPY_MOVE(basic_logger)

	using duration = std::chrono::milliseconds;
	static constexpr bool is_char_v = libgs::is_char_v<CharT>;

	using buffer = basic_buffer<CharT>;
	using string_t = std::basic_string<CharT>;

	template <typename...Args>
	using format_string = libgs::format_string<CharT,Args...>;

public:
	basic_logger(const char *file, const char *func, size_t line);

public:
	template <typename...Args>
	buffer debug(format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer debug(T &&msg);
	buffer debug();

	template <typename...Args>
	buffer info(format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer info(T &&msg);
	buffer info();

	template <typename...Args>
	buffer warning(format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer warning(T &&msg);
	buffer warning();

	template <typename...Args>
	buffer error(format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer error(T &&msg);
	buffer error();

	template <typename...Args>
	void fatal(format_string<Args...> fmt, Args&&...args);

	template <typename T>
	void fatal(T &&msg);

public:
	template <typename...Args>
	buffer cdebug(string_t category, format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer cdebug(string_t category, T &&msg);
	buffer cdebug(string_t category);

	template <typename...Args>
	buffer cinfo(string_t category, format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer cinfo(string_t category, T &&msg);
	buffer cinfo(string_t category);

	template <typename...Args>
	buffer cwarning(string_t category, format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer cwarning(string_t category, T &&msg);
	buffer cwarning(string_t category);

	template <typename...Args>
	buffer cerror(string_t category, format_string<Args...> fmt, Args&&...args);

	template <typename T>
	buffer cerror(string_t category, T &&msg);
	buffer cerror(string_t category);

	template <typename...Args>
	void cfatal(string_t category, format_string<Args...> fmt, Args&&...args);

	template <typename T>
	void cfatal(string_t category, T &&msg);

private:
	buffer _output(output_type type, string_t category) const;
	basic_output_context<CharT> m_runtime_context;
};

using logger = basic_logger<char>;

using wlogger = basic_logger<wchar_t>;

} //namespace libgs::log
#include <libgs/core/log/detail/logger.h>


#endif //LIBGS_CORE_LOG_LOGGER_H
