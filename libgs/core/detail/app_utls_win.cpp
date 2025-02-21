
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

namespace fs = std::filesystem;

namespace libgs::app
{

#if 0
static LPSTR convert_error_code_to_string(DWORD errc)
{
	HLOCAL local_address = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				  nullptr, errc, 0, reinterpret_cast<PTSTR>(&local_address), 0, nullptr);
	return reinterpret_cast<LPSTR>(local_address);
}
#endif

static void set_error(error_code &error)
{
	error.assign(static_cast<int>(GetLastError()), std::system_category());
}

fs::path file_path(error_code &error) noexcept
{
	WCHAR buf[MAX_PATH] {0};
	auto len = GetModuleFileNameW(nullptr, buf, MAX_PATH);

	if( len == 0 )
		set_error(error);
	return str_replace(std::wstring(buf,len), {L"\\", L"/"});
}

bool set_current_directory(error_code &error, const fs::path &path) noexcept
{
	error = error_code();
	auto wpath = str_replace(path.wstring(), {L"/", L"\\"});

	if( SetCurrentDirectoryW(wpath.c_str()) )
		return true;

	set_error(error);
	return false;
}

fs::path current_directory(error_code &error) noexcept
{
	error = error_code();

	wchar_t buf[1024] {0};
	auto len = GetCurrentDirectoryW(1023, buf);

	if( len == 0 )
		set_error(error);

	auto path = str_replace(std::wstring(buf,len), {L"\\", L"/"});
	if( not path.ends_with(L"/") )
		path += L"/";
	return buf;
}

bool is_absolute_path(const fs::path &path) noexcept
{
	auto wpath = path.wstring();
	if( wpath.starts_with(L"/") )
		return true;
	else if( wpath.starts_with(L"~") )
		return wpath.size() == 1 or wpath[1] == L'/' or wpath[1] == L'\\';

	auto pos = wpath.find(L':');
	if( pos == 0 or pos == std::wstring::npos )
		return false;

	else if( pos == wpath.size() - 1 )
		return true;

	if( wpath[pos + 1] == L'/' or wpath[pos + 1] == L'\\' )
		return true;
	return false;
}

constexpr size_t g_max_buf_size = 4096;

fs::path absolute_path(error_code &error, const fs::path &path) noexcept
{
	error = error_code();
	auto wpath = path.wstring();
	auto result = wpath;

	if( not is_absolute_path(path) )
		result = dir_path().wstring() + result;

	else if( wpath.starts_with(L"~") )
	{
		wchar_t tmp[g_max_buf_size] {0};
		auto len = GetEnvironmentVariableW(L"USERPROFILE", tmp, g_max_buf_size);

		if( len == 0 or len > g_max_buf_size )
		{
			set_error(error);
			result.clear();
			return result;
		}
		auto home = str_replace(std::wstring(tmp,len), {L"\\", L"/"});
		if( home.ends_with(L"/") )
			home.pop_back();

		result = home + result.erase(0,1);
	}
	result = str_replace(std::move(result), {L"/./", L"/", false});
	result = str_replace(std::move(result), {L"//", L"/", false});
	return result;
}

using optional_string = std::optional<std::string>;

using envs_t = std::map<std::string, std::string>;

optional_string getenv(error_code &error, std::string_view key) noexcept
{
	error = error_code();

	char buf[g_max_buf_size] = "";
	auto len = GetEnvironmentVariable(key.data(), buf, g_max_buf_size);

	if( len == 0 )
	{
		set_error(error);
		return {};
	}
	return std::string(buf,len);
}

envs_t getenvs(error_code &error) noexcept
{
	error = error_code();

	envs_t envs;
	auto buf = GetEnvironmentStrings();

	if( buf == nullptr )
	{
		set_error(error);
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

bool setenv(error_code &error, std::string_view key, std::string_view value, bool overwrite) noexcept
{
	error = error_code();
	if( (not overwrite and libgs::app::getenv(key).has_value()) or
		SetEnvironmentVariable(key.data(), value.data()) )
		return true;

	set_error(error);
	return false;
}

bool unsetenv(error_code &error, std::string_view key) noexcept
{
	error = error_code();
	 if( SetEnvironmentVariable(key.data(), nullptr) )
		 return true;

	set_error(error);
	return false;
}

} //namespace libgs::app

#endif //__WINNT__ || _WINDOWS
