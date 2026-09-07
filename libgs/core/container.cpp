
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#include "container.h"

namespace libgs
{

parameter_map::iterator parameter_map::find(std::string_view key)
{
	return std::ranges::find(*this, key, [](const auto &pair) {
		return return_reference(pair.first);
	});
}

parameter_map::const_iterator parameter_map::find(std::string_view key) const
{
	return std::ranges::find(*this, key, [](const auto &pair) {
		return return_reference(pair.first);
	});
}

value &parameter_map::operator[](std::string_view key)
{
	auto it = find(key);
	if( it == end() )
	{
		emplace_back(key, value{});
		it = std::prev(end());
	}
	return it->second;
}

value &parameter_map::operator[](std::string &&key)
{
	auto it = find(key);
	if( it == end() )
	{
		emplace_back(std::move(key), value{});
		it = std::prev(end());
	}
	return it->second;
}

const value &parameter_map::operator[](std::string_view key) const
{
	auto it = find(key);
	if( it == end() )
	{
		out_of_range::loc_throw (
			"libgs::parameter_map: key not found"
		);
	}
	return it->second;
}

} //namespace libgs
