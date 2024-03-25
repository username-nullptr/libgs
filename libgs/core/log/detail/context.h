
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

#ifndef LIBGS_CORE_LOG_DETAIL_CONTEXT_H
#define LIBGS_CORE_LOG_DETAIL_CONTEXT_H

#include <libgs/core/algorithm/base.h>
#include <libgs/core/shared_mutex.h>
#include <libgs/core/app_utls.h>

namespace libgs::log
{

namespace detail 
{

inline log_context g_context;

inline shared_mutex g_context_rwlock;

} //namespace detail 

inline void set_context(log_context context)
{
	using namespace detail;

	if( context.category.empty() )
		context.category = "default";
	else
		str_replace(context.category, "\\", "/");

	str_replace(context.directory, "\\", "/");

	if( not context.directory.empty() )
		context.directory = app::absolute_path(context.directory);

	g_context_rwlock.lock();
	g_context = std::move(context);
	g_context_rwlock.unlock();
}

inline void set_context(log_wcontext context)
{
	log_context tmp;
	tmp.directory           = context.directory;
	tmp.max_size_one_file   = context.max_size_one_file;
	tmp.max_size_one_day    = context.max_size_one_day ;
	tmp.max_size            = context.max_size;
	tmp.mask                = context.mask;
	tmp.source_visible      = context.source_visible;
	tmp.time_category       = context.time_category;
	tmp.async               = context.async;
	tmp.no_stdout           = context.no_stdout;
	tmp.header_breaks_aline = context.header_breaks_aline;
	tmp.category            = wcstombs(context.category);
	set_context(std::move(tmp));
}

template <concept_char_type CharT>
basic_log_context<CharT> get_context()
{
	using namespace detail;
	if constexpr( is_char_v<CharT> )
	{
		unique_lock locker(g_context_rwlock); LIBGS_UNUSED(locker);
		return g_context;
	}
	else
	{
		g_context_rwlock.lock_shared();
		log_wcontext tmp;
		tmp.directory           = g_context.directory;
		tmp.max_size_one_file   = g_context.max_size_one_file;
		tmp.max_size_one_day    = g_context.max_size_one_day;
		tmp.max_size            = g_context.max_size;
		tmp.mask                = g_context.mask;
		tmp.source_visible      = g_context.source_visible;
		tmp.time_category       = g_context.time_category;
		tmp.async               = g_context.async;
		tmp.no_stdout           = g_context.no_stdout;
		tmp.header_breaks_aline = g_context.header_breaks_aline;
		tmp.category            = mbstowcs(g_context.category);
		g_context_rwlock.unlock_shared();
		return tmp;
	}
}

template <concept_char_type CharT>
basic_output_context<CharT>::basic_output_context(basic_output_context &&other) noexcept :
	time(std::move(other.time)),
	category(std::move(other.category)),
	file(other.file),
	func(other.func),
	line(other.line)
{

}

template <concept_char_type CharT>
basic_output_context<CharT> &basic_output_context<CharT>::operator=(basic_output_context &&other) noexcept
{
	time = std::move(other.time);
	category = std::move(other.category);
	file = other.file;
	func = other.func;
	line = other.line;
	return *this;
}

} //namespace libgs::log


#endif //LIBGS_CORE_LOG_DETAIL_CONTEXT_H

