
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

#ifndef LIBGS_CORE_LOG_DETAIL_LOGGER_H
#define LIBGS_CORE_LOG_DETAIL_LOGGER_H

#include <cstring>
#include <chrono>

namespace libgs::log
{

template <concept_char_type CharT>
basic_logger<CharT>::basic_logger(const char *file, const char *func, size_t line)
{
	namespace dt = std::chrono;
	m_runtime_context.time = dt::time_point_cast<dt::milliseconds>(dt::system_clock::now());

	if constexpr( is_char_v )
	{
		m_runtime_context.file = file;
		m_runtime_context.func = func;
	}
	else
	{
		auto size = strlen(file);
		auto buf = new wchar_t[size + 1] {0};

		std::mbstowcs(buf, file, size);
		m_runtime_context.file = buf;

		size = strlen(func);
		memset(buf, 0, size);

		std::mbstowcs(buf, func, size);
		m_runtime_context.func = buf;

		delete[] buf;
	}
	m_runtime_context.line = line;
}

template <concept_char_type CharT>
void basic_logger<CharT>::set_context(const context &con)
{
	writer::instance().set_context(con);
}

template <concept_char_type CharT>
void basic_logger<CharT>::set_header_breaks_aline(bool enable)
{
	writer::instance().set_header_breaks_aline(enable);
}

template <concept_char_type CharT>
basic_log_context<CharT> basic_logger<CharT>::get_context()
{
	if constexpr( is_char_v )
		return writer::instance().get_context();
	else
		return writer::instance().get_wcontext();
}

template <concept_char_type CharT>
bool basic_logger<CharT>::get_header_breaks_aline()
{
	return writer::instance().get_header_breaks_aline();
}

#ifdef __NO_DEBUG__
# define __LIBGS_CORE_LOG_OUTPUT(_type, _category)  return buffer(output_type::_type)
#else //DEBUG
#define __LIBGS_CORE_LOG_OUTPUT(_type, _category) \
({ \
	buffer o(output_type::_type); \
	o.context() = m_runtime_context; \
	o.context().category = std::move(_category); \
	return o; \
})
#endif //__NO_DEBUG

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::debug(format_string<Args...> fmt_value, Args&&...args) {
	return std::move(debug() << std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::debug(T &&msg) {
	return std::move(debug() << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::debug()
{
	if constexpr( is_char_v )
		__LIBGS_CORE_LOG_OUTPUT(debug, "");
	else
		__LIBGS_CORE_LOG_OUTPUT(debug, L"");
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::info(format_string<Args...> fmt_value, Args&&...args) {
	return std::move(info() << std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::info(T &&msg) {
	return std::move(info() << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::info()
{
	if constexpr( is_char_v )
		__LIBGS_CORE_LOG_OUTPUT(info, "");
	else
		__LIBGS_CORE_LOG_OUTPUT(info, L"");
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::warning(format_string<Args...> fmt_value, Args&&...args) {
	return std::move(warning() << std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::warning(T &&msg) {
	return std::move(warning() << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::warning()
{
	if constexpr( is_char_v )
		__LIBGS_CORE_LOG_OUTPUT(warning, "");
	else
		__LIBGS_CORE_LOG_OUTPUT(warning, L"");
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::error(format_string<Args...> fmt_value, Args&&...args) {
	return std::move(error() << std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::error(T &&msg) {
	return std::move(error() << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::error()
{
	if constexpr( is_char_v )
		__LIBGS_CORE_LOG_OUTPUT(error, "");
	else
		__LIBGS_CORE_LOG_OUTPUT(error, L"");
}

template <concept_char_type CharT>
template <typename...Args>
void basic_logger<CharT>::fatal(format_string<Args...> fmt_value, Args&&...args) 
{
	if constexpr( is_char_v )
		m_runtime_context.category = "";
	else
		m_runtime_context.category = L"";
	writer::instance().fatal(std::move(m_runtime_context), std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
void basic_logger<CharT>::fatal(T &&msg) 
{
	if constexpr( is_char_v )
		m_runtime_context.category = "";
	else
		m_runtime_context.category = L"";
	writer::instance().fatal(std::move(m_runtime_context), std::format(default_format_v<CharT>, std::forward<T>(msg)));
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::cdebug(str_type category, format_string<Args...> fmt, Args&&...args) {
	return std::move(cdebug(std::move(category)) << std::format(fmt, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::cdebug(str_type category, T &&msg) {
	return std::move(cdebug(std::move(category)) << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::cdebug(str_type category)
{
	__LIBGS_CORE_LOG_OUTPUT(debug, category);
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::cinfo(str_type category, format_string<Args...> fmt, Args&&...args) {
	return std::move(cinfo(std::move(category)) << std::format(fmt, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::cinfo(str_type category, T &&msg) {
	return std::move(cinfo(std::move(category)) << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::cinfo(str_type category)
{
	__LIBGS_CORE_LOG_OUTPUT(info, category);
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::cwarning(str_type category, format_string<Args...> fmt, Args&&...args) {
	return std::move(cwarning(std::move(category)) << std::format(fmt, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::cwarning(str_type category, T &&msg) {
	return std::move(cwarning(std::move(category)) << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::cwarning(str_type category)
{
	__LIBGS_CORE_LOG_OUTPUT(warning, category);
}

template <concept_char_type CharT>
template <typename...Args>
basic_buffer<CharT> basic_logger<CharT>::cerror(str_type category, format_string<Args...> fmt, Args&&...args) {
	return std::move(cerror(std::move(category)) << std::format(fmt, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> basic_logger<CharT>::cerror(str_type category, T &&msg) {
	return std::move(cerror(std::move(category)) << std::forward<T>(msg));
}

template <concept_char_type CharT>
basic_buffer<CharT> basic_logger<CharT>::cerror(str_type category)
{
	__LIBGS_CORE_LOG_OUTPUT(error, category);
}

template <concept_char_type CharT>
template <typename...Args>
void basic_logger<CharT>::cfatal(str_type category, format_string<Args...> fmt_value, Args&&...args) 
{
	m_runtime_context.category = std::move(category);
	writer::instance().fatal(std::move(m_runtime_context), std::format(fmt_value, std::forward<Args>(args)...));
}

template <concept_char_type CharT>
template <typename T>
void basic_logger<CharT>::cfatal(str_type category, T &&msg)
{
	m_runtime_context.category = std::move(category);
	writer::instance().fatal(std::move(m_runtime_context), std::format(default_format_v<CharT>, std::forward<T>(msg)));
}

template <concept_char_type CharT>
void basic_logger<CharT>::wait(const duration &ms)
{
	writer::instance().wait(ms);
}

template <concept_char_type CharT>
void basic_logger<CharT>::wait()
{
	writer::instance().wait();
}

} //namespace libgs::log


#endif //LIBGS_CORE_LOG_DETAIL_LOGGER_H
