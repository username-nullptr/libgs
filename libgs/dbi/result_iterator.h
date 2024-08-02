
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

#ifndef LIBGS_DBI_RESULT_ITERATOR_H
#define LIBGS_DBI_RESULT_ITERATOR_H

#include <libgs/dbi/types.h>
#include <optional>

namespace libgs::dbi
{

template <concept_char_type CharT>
class LIBGS_DBI_TAPI basic_result_iterator
{
	LIBGS_DISABLE_COPY_MOVE(basic_result_iterator)

public:
	template <typename T>
	using opt = std::optional<T>;

	using string_t = std::basic_string<CharT>;
	using string_view_t = std::basic_string_view<CharT>;

public:
	basic_result_iterator() = default;
	virtual ~basic_result_iterator() = 0;

public:
	[[nodiscard]] virtual bool next() = 0;
	[[nodiscard]] virtual size_t column_count() const = 0;
	[[nodiscard]] virtual string_view_t column_name(size_t column) const = 0;

public:
	[[nodiscard]] virtual opt<bool>     get_opt_bool  (size_t column) const = 0;
	[[nodiscard]] virtual opt<int8_t>   get_opt_char  (size_t column) const = 0;
	[[nodiscard]] virtual opt<uint8_t>  get_opt_uchar (size_t column) const = 0;
	[[nodiscard]] virtual opt<int16_t>  get_opt_short (size_t column) const = 0;
	[[nodiscard]] virtual opt<uint16_t> get_opt_ushort(size_t column) const = 0;
	[[nodiscard]] virtual opt<int32_t>  get_opt_int   (size_t column) const = 0;
	[[nodiscard]] virtual opt<uint32_t> get_opt_uint  (size_t column) const = 0;
	[[nodiscard]] virtual opt<int64_t>  get_opt_long  (size_t column) const = 0;
	[[nodiscard]] virtual opt<uint64_t> get_opt_ulong (size_t column) const = 0;
	[[nodiscard]] virtual opt<float>    get_opt_float (size_t column) const = 0;
	[[nodiscard]] virtual opt<double>   get_opt_double(size_t column) const = 0;
	[[nodiscard]] virtual opt<string_t> get_opt_string(size_t column, size_t maxlen) const = 0;
	[[nodiscard]] opt<string_t>         get_opt_string(size_t column) const; //1024
	// ... ...

public:
	[[nodiscard]] virtual opt<bool>     get_opt_bool  (string_view_t column_name) const;
	[[nodiscard]] virtual opt<int8_t>   get_opt_char  (string_view_t column_name) const;
	[[nodiscard]] virtual opt<uint8_t>  get_opt_uchar (string_view_t column_name) const;
	[[nodiscard]] virtual opt<int16_t>  get_opt_short (string_view_t column_name) const;
	[[nodiscard]] virtual opt<uint16_t> get_opt_ushort(string_view_t column_name) const;
	[[nodiscard]] virtual opt<int32_t>  get_opt_int   (string_view_t column_name) const;
	[[nodiscard]] virtual opt<uint32_t> get_opt_uint  (string_view_t column_name) const;
	[[nodiscard]] virtual opt<int64_t>  get_opt_long  (string_view_t column_name) const;
	[[nodiscard]] virtual opt<uint64_t> get_opt_ulong (string_view_t column_name) const;
	[[nodiscard]] virtual opt<float>    get_opt_float (string_view_t column_name) const;
	[[nodiscard]] virtual opt<double>   get_opt_double(string_view_t column_name) const;
	[[nodiscard]] virtual opt<string_t> get_opt_string(string_view_t column_name, size_t maxlen) const;
	[[nodiscard]] opt<string_t>         get_opt_string(string_view_t column_name) const; //1024
	// ... ...

public:
	[[nodiscard]] virtual bool     get_bool  (size_t column) const;
	[[nodiscard]] virtual int8_t   get_char  (size_t column) const;
	[[nodiscard]] virtual uint8_t  get_uchar (size_t column) const;
	[[nodiscard]] virtual int16_t  get_short (size_t column) const;
	[[nodiscard]] virtual uint16_t get_ushort(size_t column) const;
	[[nodiscard]] virtual int32_t  get_int   (size_t column) const;
	[[nodiscard]] virtual uint32_t get_uint  (size_t column) const;
	[[nodiscard]] virtual int64_t  get_long  (size_t column) const;
	[[nodiscard]] virtual uint64_t get_ulong (size_t column) const;
	[[nodiscard]] virtual float    get_float (size_t column) const;
	[[nodiscard]] virtual double   get_double(size_t column) const;
	[[nodiscard]] virtual string_t get_string(size_t column, size_t maxlen) const;
	[[nodiscard]] string_t         get_string(size_t column) const; //1024
	// ... ...

public:
	[[nodiscard]] virtual bool     get_bool  (string_view_t column_name) const;
	[[nodiscard]] virtual int8_t   get_char  (string_view_t column_name) const;
	[[nodiscard]] virtual uint8_t  get_uchar (string_view_t column_name) const;
	[[nodiscard]] virtual int16_t  get_short (string_view_t column_name) const;
	[[nodiscard]] virtual uint16_t get_ushort(string_view_t column_name) const;
	[[nodiscard]] virtual int32_t  get_int   (string_view_t column_name) const;
	[[nodiscard]] virtual uint32_t get_uint  (string_view_t column_name) const;
	[[nodiscard]] virtual int64_t  get_long  (string_view_t column_name) const;
	[[nodiscard]] virtual uint64_t get_ulong (string_view_t column_name) const;
	[[nodiscard]] virtual float    get_float (string_view_t column_name) const;
	[[nodiscard]] virtual double   get_double(string_view_t column_name) const;
	[[nodiscard]] virtual string_t get_string(string_view_t column_name, size_t maxlen) const;
	[[nodiscard]] string_t         get_string(string_view_t column_name) const; //1024
	// ... ...

public:
	[[nodiscard]] int row_id() const;
};

using result_iterator = basic_result_iterator<char>;
using wresult_iterator = basic_result_iterator<wchar_t>;

template <concept_char_type CharT>
using basic_result_iterator_ptr = std::shared_ptr<basic_result_iterator<CharT>>;

using result_iterator_ptr = basic_result_iterator_ptr<char>;
using wresult_iterator_ptr = basic_result_iterator_ptr<wchar_t>;

} //namespace libgs::dbi
#include <libgs/dbi/detail/result_iterator.h>


#endif //LIBGS_DBI_RESULT_ITERATOR_H
