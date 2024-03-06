
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

#if defined(__WINNT__) || defined(_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "libgs/core/algorithm/base.h"
#include "libgs/core/app_utls.h"
#include "libgs/core/log.h"

namespace libgs::app
{

static LPSTR convert_error_code_to_string(DWORD errc)
{
	HLOCAL local_address = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, errc, 0, reinterpret_cast<PTSTR>(&local_address), 0, nullptr);
	return reinterpret_cast<LPSTR>(local_address);
}

std::string file_path()
{
	TCHAR buf[MAX_PATH] = {0};
	auto len = GetModuleFileName(nullptr, buf, _countof(buf));

	if( len == 0 )
	{
		auto errc = GetLastError();
		libgs_log_error("libgs::app::file_path: GetModuleFileName: {}. ({})", convert_error_code_to_string(errc), errc);
	}
	std::string path(buf, len);
	str_replace(path, "\\", "/");
	return path;
}

bool set_current_directory(std::string_view path)
{
	std::string _path(path.data(), path.size());
	str_replace(_path, "/", "\\");

	if( SetCurrentDirectory(_path.c_str()) )
		return true;

	auto errc = GetLastError();
	libgs_log_error("libgs::app::set_current_directory: SetCurrentDirectory: {}. ({})", convert_error_code_to_string(errc), errc);
	return false;
}

std::string current_directory()
{
	char buf[1024] = "";
	auto len = GetCurrentDirectory(1023, buf);

	if( len == 0 )
	{
		auto errc = GetLastError();
		libgs_log_error("libgs::app::current_directory: GetCurrentDirectory: {}. ({})", convert_error_code_to_string(errc), errc);
	}
	std::string path(buf, len);
	str_replace(path, "\\", "/");

	if( not path.ends_with("/") )
		path += "/";
	return buf;
}

bool is_absolute_path(std::string_view path)
{
	if( path.starts_with("/") )
		return true;
	else if( path.starts_with("~") )
		return path.size() == 1 or path[1] == '/' or path[1] == '\\';

	auto pos = path.find(':');
	if( pos == 0 or pos == std::string::npos )
		return false;

	else if( pos == path.size() - 1 )
		return true;

	if( path[pos + 1] == '/' or path[pos + 1] == '\\' )
		return true;
	return false;
}

constexpr size_t g_max_buf_size = 4096;

std::string absolute_path(std::string_view path)
{
	std::string result(path.data(), path.size());
	if( not is_absolute_path(path) )
		return dir_path() + result;

	else if( path.starts_with("~") )
	{
		char tmp[g_max_buf_size] = {0};
		auto len = GetEnvironmentVariable("USERPROFILE", tmp, g_max_buf_size);

		if( len == 0 or len > g_max_buf_size )
		{
			auto errc = GetLastError();
			libgs_log_error("libgs::app::absolute_path: GetEnvironmentVariable: {}. ({})", convert_error_code_to_string(errc), errc);

			result.clear();
			return result;
		}
		std::string home(tmp, len);
		str_replace(home, "\\", "/");

		if( home.ends_with("/") )
			home.pop_back();

		result = home + result.erase(0,1);
		if( not result.ends_with("/") )
			result += "/";
	}
	return result;
}

using optional_string = std::optional<std::string>;

using envs_t = std::map<std::string, std::string>;

optional_string getenv(std::string_view key)
{
	char buf[g_max_buf_size] = "";
	auto len = GetEnvironmentVariable(key.data(), buf, g_max_buf_size);

	if( len == 0 )
	{
		auto errc = GetLastError();
		libgs_log_error("libgs::app::getenv: GetEnvironmentVariable: {}. ({})", convert_error_code_to_string(errc), errc);
		return {};
	}
	return std::string(buf,len);
}

envs_t getenvs()
{
	envs_t envs;
	auto buf = GetEnvironmentStrings();

	if( buf == nullptr )
	{
		auto errc = GetLastError();
		libgs_log_error("libgs::app::getenvs: GetEnvironmentStrings: {}. ({})", convert_error_code_to_string(errc), errc);
		return envs;
	}
	size_t start = 0;
	for(size_t i=0; ;i++)
	{
		if( buf[i] == '=' )
		{
			auto m = i;
			while( buf[++i] != '\0' );

			envs.emplace(std::string(buf+start, m-start), std::string(buf+m+1, i-m-1));
			start = i + 1;
		}
		else if( buf[i] == '\0' )
			break;
	}
	return envs;
}

bool setenv(std::string_view key, std::string_view value, bool overwrite)
{
	if( not overwrite and libgs::app::getenv(key).has_value() )
		return true;
	return SetEnvironmentVariable(key.data(), value.data());
}

bool unsetenv(std::string_view key)
{
	return SetEnvironmentVariable(key.data(), nullptr);
}

} //namespace libgs::app

#endif //__WINNT__ || _WINDOWS
