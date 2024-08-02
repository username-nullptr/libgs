
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

#ifndef LIBGS_DBI_TYPES_H
#define LIBGS_DBI_TYPES_H

#include <libgs/dbi/cell.h>
#include <vector>
#include <map>

namespace libgs::dbi
{

template <concept_char_type CharT>
struct basic_connect_info
{
	using string_t = std::basic_string<CharT>;
	string_t driver; //odbc using
	string_t ip;
	uint16_t port;
	string_t user;
	string_t passwd;
};

using connect_info = basic_connect_info<char>;
using wconnect_info = basic_connect_info<wchar_t>;

using binary = std::vector<char>;

template <concept_char_type CharT>
class basic_row_vector : public std::vector<basic_cell<CharT>>
{
public:
	using cell_t = basic_cell<CharT>;
	using std::vector<cell_t>::vector;
	using std::vector<cell_t>::operator[];

public:
	cell_t &operator[](std::basic_string_view<CharT> column_name)
	{
		for(auto &cell : *this)
		{
			if( cell.column_name() == column_name )
				return cell;
		}
		throw runtime_error("libgs::dbi::basic_row_vector::operator[](std::basic_string_view): Invalid column name: '{}'.", column_name);
		// return xxx;
	}
};

template <concept_char_type CharT>
using basic_table_data = std::vector<basic_row_vector<CharT>>;

using table_data = basic_table_data<char>;
using wtable_data = basic_table_data<wchar_t>;

} //namespace libgs::dbi


#endif //LIBGS_DBI_TYPES_H
