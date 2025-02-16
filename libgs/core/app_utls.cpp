
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

namespace fs = std::filesystem;

namespace libgs::app
{

fs::path file_path()
{
	error_code error;
	auto path = file_path(error);
	if( error )
		throw system_error(error, "libgs::app::file_path");
	return path;
}

fs::path dir_path()
{
	error_code error;
	auto path = dir_path(error);
	if( error )
		throw system_error(error, "libgs::app::dir_path");
	return path;
}

fs::path dir_path(error_code &error) noexcept
{
	auto file_name = file_path(error).wstring();
	if( error )
		return {};

	auto index = file_name.find_last_of(L'/');
	if( index == std::wstring::npos or index == file_name.size() - 1 )
		return L"./";
	return file_name.erase(index + 1);
}

bool set_current_directory(const fs::path &path)
{
	error_code error;
	bool res = set_current_directory(error, path);
	if( error )
		throw system_error(error, "libgs::app::set_current_directory");
	return res;
}

fs::path current_directory()
{
	error_code error;
	auto path = current_directory(error);
	if( error )
		throw system_error(error, "libgs::app::current_directory");
	return path;
}

fs::path absolute_path(const fs::path &path)
{
	error_code error;
	auto apath = absolute_path(error, path);
	if( error )
		throw system_error(error, "libgs::app::absolute_path");
	return apath;
}

using optional_string = std::optional<std::string>;

using envs_t = std::map<std::string, std::string>;

optional_string getenv(std::string_view key)
{
	error_code error;
	auto res = getenv(error, key);
	if( error )
		throw system_error(error, "libgs::app::getenv");
	return res;
}

envs_t getenvs()
{
	error_code error;
	auto res = getenvs(error);
	if( error )
		throw system_error(error, "libgs::app::getenvs");
	return res;
}

bool setenv(std::string_view key, std::string_view value, bool overwrite)
{
	error_code error;
	auto res = setenv(error, key, value, overwrite);
	if( error )
		throw system_error(error, "libgs::app::setenv");
	return res;
}

bool unsetenv(std::string_view key)
{
	error_code error;
	auto res = unsetenv(error, key);
	if( error )
		throw system_error(error, "libgs::app::unsetenv");
	return res;
}

} //namespace libgs::app
