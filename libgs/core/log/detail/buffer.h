
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

#ifndef LIBGS_CORE_LOG_DETAIL_LOG_BUFFER_H
#define LIBGS_CORE_LOG_DETAIL_LOG_BUFFER_H

namespace libgs::log
{

template <concept_char_type CharT>
basic_buffer<CharT>::basic_buffer(output_type t) :
	m_data(new data)
{
	m_data->type = t;
}

template <concept_char_type CharT>
basic_buffer<CharT>::basic_buffer(const basic_buffer &other) :
	m_data(new data)
{
	operator=(other);
}

template <concept_char_type CharT>
basic_buffer<CharT>::basic_buffer(basic_buffer &&other) noexcept
{
	m_data = other.m_data;
	other.m_data = nullptr;
}

template <concept_char_type CharT>
basic_buffer<CharT>::~basic_buffer()
{
	if( m_data == nullptr )
		return ;

#ifndef __NO_DEBUG__
	else if( not m_data->buffer.empty() )
		scheduler::instance().write(m_data->type, std::move(m_data->context), std::move(m_data->buffer));
#endif //__NO_DEBUG__
	delete m_data;
}

template <concept_char_type CharT>
basic_buffer<CharT> &basic_buffer<CharT>::operator=(const basic_buffer &other)
{
	*m_data = *other.m_data;
	return *this;
}

template <concept_char_type CharT>
basic_buffer<CharT> &basic_buffer<CharT>::operator=(basic_buffer &&other) noexcept
{
	if( m_data )
		delete m_data;
	m_data = other.m_data;
	other.m_data = nullptr;
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> &basic_buffer<CharT>::write(T &&msg)
{
#ifndef __NO_DEBUG__
	if constexpr( is_char_v<CharT> )
		m_data->buffer += std::format("{} ", std::forward<T>(msg));
	else
		m_data->buffer += std::format(L"{} ", std::forward<T>(msg));
#endif //__NO_DEBUG__
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_buffer<CharT> &basic_buffer<CharT>::operator<<(T &&msg)
{
	return write(std::forward<T>(msg));
}

template <concept_char_type CharT>
const basic_output_context<CharT> &basic_buffer<CharT>::context() const 
{
	return m_data->context;
}

template <concept_char_type CharT>
basic_output_context<CharT> &basic_buffer<CharT>::context()
{
	return m_data->context;
}

} //namespace libgs::log


#endif //LIBGS_CORE_LOG_DETAIL_LOG_BUFFER_H
