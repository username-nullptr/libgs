
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

#ifndef LIBGS_CORE_LOG_CONTEXT_H
#define LIBGS_CORE_LOG_CONTEXT_H

#include <libgs/core/global.h>

namespace libgs::log
{

template <typename T>
struct basic_log_context;

template <>
struct basic_log_context<void>
{
	virtual ~basic_log_context() = default;
	std::string directory/* = "" */;

	size_t max_size_one_file = 10240;
	size_t max_size_one_day = 10485760;
	size_t max_size = 1073741824;

	uint8_t mask = 4;
	bool source_visible = false;
	bool file_path_visible = true;

	bool time_category = true;
	bool async = false;
	bool no_stdout = false;
	bool header_breaks_aline = false;
};

template <>
struct basic_log_context<char> : basic_log_context<void> {
	std::string category = "default";
};

template <>
struct basic_log_context<wchar_t> : basic_log_context<void> {
	std::wstring category = L"default";
};

using log_context = basic_log_context<char>;

using log_wcontext = basic_log_context<wchar_t>;

LIBGS_CORE_API void set_context(log_context context);
LIBGS_CORE_API void set_context(log_wcontext context);

template <concept_char_type CharT = char>
[[nodiscard]] basic_log_context<CharT> get_context();

enum class output_type
{
	debug,   //stdout
	info,    //stdout
	warning, //stderr
	error,   //stderr
	fetal    //stderr-sync
};

using output_time = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

template <concept_char_type CharT>
struct basic_output_context
{
	output_time time;
	std::basic_string<CharT> category;
	std::basic_string<CharT> file;
	std::basic_string<CharT> func;
	size_t line = 0;

	basic_output_context() = default;
	basic_output_context(const basic_output_context &other) = default;
	basic_output_context &operator=(const basic_output_context &other) = default;
	basic_output_context(basic_output_context &&other) noexcept;
	basic_output_context &operator=(basic_output_context &&other) noexcept;
};

using output_context = basic_output_context<char>;

using output_wcontext = basic_output_context<wchar_t>;

} //namespace libgs::log
#include <libgs/core/log/detail/context.h>


#endif //LIBGS_CORE_LOG_CONTEXT_H
