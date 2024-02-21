
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

#include "app_utls.h"
#include "log.h"

namespace libgs::app
{

std::string dir_path()
{
	auto file_name = file_path();
	auto index = file_name.find_last_of('/');
	if( index == std::string::npos or index == file_name.size() - 1 )
		return "./";
	return file_name.erase(index+1);
}

bool is_absolute_path(std::string_view path)
{
	return path.starts_with("/")
#ifdef __unix__
			or path.starts_with("~/")
#endif //__unix__
			;
}

using optional_string = std::optional<std::string>;

optional_string getenv(std::string_view key)
{
	auto value = ::getenv(key.data());
	return value? optional_string(value) : optional_string();
}

bool setenv(std::string_view key, std::string_view value, bool overwrite)
{
	if( ::setenv(key.data(), value.data(), overwrite) != 0 )
	{
		libgs_log_error("libgs::appinfo::setenv: setenv: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

bool unsetenv(std::string_view key)
{
	if( ::unsetenv(key.data()) != 0 )
	{
		libgs_log_error("libgs::appinfo::unsetenv: unsetenv: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

} //namespace libgs::app
