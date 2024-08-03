
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

#ifndef LIBGS_DBI_DETAIL_RESULT_ITERATOR_H
#define LIBGS_DBI_DETAIL_RESULT_ITERATOR_H

namespace libgs::dbi
{

template <concept_char_type CharT>
inline basic_result_iterator<CharT>::~basic_result_iterator()
{

}

template <concept_char_type CharT>
std::optional<std::basic_string<CharT>> basic_result_iterator<CharT>::get_opt_string(size_t column) const
{
	return get_opt_string(column, 1024);
}

#define GET_XX_BY_NAME(_return, _func_name) \
	template <concept_char_type CharT> \
	_return basic_result_iterator<CharT>::_func_name(string_view_t column_name) const { \
		for(size_t column=0; column<column_count(); column++) { \
			if( this->column_name(column) == column_name ) \
				return _func_name(column); \
		} \
		throw runtime_error("libgs::dbi::basic_result_iterator::" #_func_name ": Invalid column name."); \
	}

GET_XX_BY_NAME(std::optional<bool>    , get_opt_bool  )
GET_XX_BY_NAME(std::optional<int8_t>  , get_opt_char  )
GET_XX_BY_NAME(std::optional<uint8_t> , get_opt_uchar )
GET_XX_BY_NAME(std::optional<int16_t> , get_opt_short )
GET_XX_BY_NAME(std::optional<uint16_t>, get_opt_ushort)
GET_XX_BY_NAME(std::optional<int32_t> , get_opt_int   )
GET_XX_BY_NAME(std::optional<uint32_t>, get_opt_uint  )
GET_XX_BY_NAME(std::optional<int64_t> , get_opt_long  )
GET_XX_BY_NAME(std::optional<uint64_t>, get_opt_ulong )
GET_XX_BY_NAME(std::optional<float>   , get_opt_float )
GET_XX_BY_NAME(std::optional<double>  , get_opt_double)

template <concept_char_type CharT>
std::optional<std::basic_string<CharT>> basic_result_iterator<CharT>::get_opt_string
(string_view_t column_name, size_t maxlen) const
{
	for(size_t column=0; column<column_count(); column++)
	{
		if( this->column_name(column) == column_name )
			return get_opt_string(column, maxlen);
	}
	throw runtime_error("dbi::result_set::get_opt_string: Invalid column name.");
	// return {};
}

template <concept_char_type CharT>
std::optional<std::basic_string<CharT>> basic_result_iterator<CharT>::get_opt_string(string_view_t column_name) const
{
	return get_opt_string(column_name, 1024);
}

#define GET_XX_BY_INDEX(_return, _basic_func_name) \
	template <concept_char_type CharT> \
	_return basic_result_iterator<CharT>::get_##_basic_func_name(size_t column) const { \
		auto opt = get_opt_##_basic_func_name(column); \
		if( opt.has_value() ) \
			return opt.value(); \
		else \
			throw runtime_error("dbi::result_set::get_" #_return ": Optional is empty."); \
	}

GET_XX_BY_INDEX(bool    , bool  )
GET_XX_BY_INDEX(int8_t  , char  )
GET_XX_BY_INDEX(uint8_t , uchar )
GET_XX_BY_INDEX(int16_t , short )
GET_XX_BY_INDEX(uint16_t, ushort)
GET_XX_BY_INDEX(int32_t , int   )
GET_XX_BY_INDEX(uint32_t, uint  )
GET_XX_BY_INDEX(int64_t , long  )
GET_XX_BY_INDEX(uint64_t, ulong )
GET_XX_BY_INDEX(float   , float )
GET_XX_BY_INDEX(double  , double)

template <concept_char_type CharT>
std::basic_string<CharT> basic_result_iterator<CharT>::get_string(size_t column, size_t maxlen) const
{
	auto opt = get_opt_string(column, maxlen);
	if( opt.has_value() )
		return opt.value();
	else
		throw runtime_error("dbi::result_set::get_string: Optional is empty.");
	// return {};
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_result_iterator<CharT>::get_string(size_t column) const
{
	return get_string(column, 1024);
}

GET_XX_BY_NAME(bool    , get_bool  )
GET_XX_BY_NAME(int8_t  , get_char  )
GET_XX_BY_NAME(uint8_t , get_uchar )
GET_XX_BY_NAME(int16_t , get_short )
GET_XX_BY_NAME(uint16_t, get_ushort)
GET_XX_BY_NAME(int32_t , get_int   )
GET_XX_BY_NAME(uint32_t, get_uint  )
GET_XX_BY_NAME(int64_t , get_long  )
GET_XX_BY_NAME(uint64_t, get_ulong )
GET_XX_BY_NAME(float   , get_float )
GET_XX_BY_NAME(double  , get_double)

template <concept_char_type CharT>
std::basic_string<CharT> basic_result_iterator<CharT>::get_string(string_view_t column_name, size_t maxlen) const
{
	for(std::size_t column=0; column<column_count(); column++)
	{
		if( this->column_name(column) == column_name )
			return get_string(column, maxlen);
	}
	throw runtime_error("dbi::result_set::get_string: Invalid column name.");
	// return {};
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_result_iterator<CharT>::get_string(string_view_t column_name) const
{
	return get_string(column_name, 1024);
}

template <concept_char_type CharT>
int basic_result_iterator<CharT>::row_id() const
{
	auto res = get_opt_int(0);
	return res? *res : -1;
}

#undef GET_XX_BY_NAME
#undef GET_XX_BY_INDEX

} //namespace libgs::dbi


#endif //LIBGS_DBI_DETAIL_RESULT_ITERATOR_H
