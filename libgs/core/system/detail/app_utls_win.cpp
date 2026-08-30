
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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
#include <libgs/core/system/app_utls.h>
#include <Windows.h>
#include <knownfolders.h>
#include <shlobj.h>

#ifdef _MSC_VER
# pragma comment(lib, "shell32.lib")
#endif

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

[[nodiscard]] static error_code sys_error()
{
	return { static_cast<int>(GetLastError()), std::system_category() };
}

sys_expected<path_t> file_path() noexcept
{
	WCHAR buf[MAX_PATH] {0};
	auto len = GetModuleFileNameW(nullptr, buf, MAX_PATH);

	sys_expected<path_t> result {L""};
	if( len == 0 )
		result.despair(sys_error());
	else
		result = strtls::replace(std::wstring(buf,len), L"\\", L"/");
	return result;
}

sys_expected<> set_current_directory(const path_t &path) noexcept
{
	sys_expected<> result;
	auto wpath = strtls::replace(path.wstring(), L"/", L"\\");

	if( not SetCurrentDirectoryW(wpath.c_str()) )
		result.despair(sys_error());
	return result;
}

sys_expected<path_t> current_directory() noexcept
{
	wchar_t buf[1024] {0};
	auto len = GetCurrentDirectoryW(1023, buf);

	sys_expected<path_t> result {L""};
	if( len == 0 )
		result.despair(sys_error());
	else
	{
		auto path = strtls::replace(std::wstring(buf,len), L"\\", L"/");
		if( not path.ends_with(L"/") )
			path += L"/";
		result = path;
	}
	return result;
}

constexpr size_t g_max_buf_size = 4096;

sys_expected<path_t> absolute_path(const path_t &path) noexcept
{
	auto wpath = path.wstring();
	sys_expected<path_t> result {L""};

	if( not is_absolute_path(path) )
	{
		result = dir_path().transform([&](const path_t &dir) -> path_t {
			return dir.wstring() + wpath;
		});
	}
	else if( wpath.starts_with(L"~") )
	{
		wchar_t tmp[g_max_buf_size] {0};
		auto len = GetEnvironmentVariableW(L"USERPROFILE", tmp, g_max_buf_size);

		if( len == 0 or len > g_max_buf_size )
			result.despair(sys_error());
		else
		{
			auto home = strtls::replace(std::wstring(tmp,len), L"\\", L"/");
			if( home.ends_with(L"/") )
				home.pop_back();
			result = home + wpath.erase(0,1);
		}
	}
	return result.transform([](const path_t &resolved_path) -> path_t
	{
		auto normalized_path = strtls::replace(resolved_path.wstring(), L"/./", L"/", false);
		return strtls::replace(std::move(normalized_path), L"//", L"/", false);
	});
}

bool is_absolute_path(const path_t &path) noexcept
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

sys_expected<std::string> getenv(std::string_view key) noexcept
{
	char buf[g_max_buf_size] = "";
	auto len = GetEnvironmentVariable(key.data(), buf, g_max_buf_size);

	sys_expected<std::string> result {""};
	if( len == 0 )
		result.despair(sys_error());
	else
		result = std::string(buf,len);
	return result;
}

sys_expected<std::map<std::string,std::string>> getenvs() noexcept
{
	using envs_t = std::map<std::string,std::string>;
	sys_expected<envs_t> result {envs_t{}};

	auto buf = GetEnvironmentStrings();
	if( buf == nullptr )
		return result.despair(sys_error());

	size_t start = 0;
	for(size_t i=0; ;i++)
	{
		if( buf[i] == '=' )
		{
			auto m = i;
			while( buf[++i] != '\0' ) {}

			result.value().emplace (
				std::string(buf + start, m - start),
				std::string(buf + m + 1, i - m - 1)
			);
			start = i + 1;
		}
		else if( buf[i] == '\0' )
			break;
	}
	return result;
}

sys_expected<> setenv(std::string_view key, std::string_view value, bool overwrite) noexcept
{
	sys_expected<> result;
	if( (not overwrite and app::getenv(key).has_value()) or
		SetEnvironmentVariable(key.data(), value.data()) )
		return result;
	return result.despair(sys_error());
}

sys_expected<> unsetenv(std::string_view key) noexcept
{
	sys_expected<> result;
	if( SetEnvironmentVariable(key.data(), nullptr) )
		return result;
	return result.despair(sys_error());
}

sys_expected<path_t> home_directory() noexcept
{
	sys_expected<path_t> result;
	PWSTR path = nullptr;

	auto hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);
	if( SUCCEEDED(hr) )
	{
		result.emplace(path);
		CoTaskMemFree(path);
	}
	else
		result.despair(sys_error());
	return result;
}

} //namespace libgs::app

#endif //__WINNT__ || _WINDOWS
