
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

#ifdef __unix__

#include "libgs/core/system/app_utls.h"
#include "libgs/core/shared_mutex.h"

#include <unistd.h>
#include <pwd.h>

/* extern char **environ; */

namespace fs = std::filesystem;

namespace libgs::app
{

[[nodiscard]] static error_code sys_error()
{
	return { errno, std::system_category() };
}

sys_expected<path_t> file_path() noexcept
{
	sys_expected<path_t> result {""};
	char exe_name[1024] = "";

	if( readlink("/proc/self/exe", exe_name, sizeof(exe_name)) < 0 )
		result.despair(sys_error());
	else
		result = exe_name;
	return result;
}

sys_expected<> set_current_directory(const path_t &path) noexcept
{
	sys_expected<> result;
	auto str = path.string();

	if( chdir(str.data()) < 0 )
		result.despair(sys_error());
	return result;
}

sys_expected<path_t> current_directory() noexcept
{
	sys_expected<path_t> result {""};
	char buf[1024] = "";

	if( getcwd(buf, sizeof(buf)) == nullptr )
		result.despair(sys_error());
	else
	{
		std::string str(buf);
		if( not str.ends_with("/") )
			str += "/";
		result = str;
	}
	return result;
}

sys_expected<path_t> absolute_path(const path_t &path) noexcept
{
	auto str = path.string();
	sys_expected<path_t> result = path;

	if( not is_absolute_path(path) )
	{
		result = dir_path().transform([&](const path_t &dir) -> path_t {
			return dir.string() + str;
		});
	}
	else if( str.starts_with("~") )
	{
		result = home_directory().transform([&](const path_t &_path) -> path_t {
			return _path.string() + str.erase(0,1);
		});
	}
	return result.transform([](const path_t &resolved_path) -> path_t
	{
		auto normalized_path = strtls::replace(resolved_path.string(), "/./", "/", false);
		return strtls::replace(std::move(normalized_path), "//", "/", false);
	});
}

bool is_absolute_path(const path_t &path) noexcept
{
	auto str = path.string();
	if( str.starts_with("/") )
		return true;
	else if( str.starts_with("~") )
		return str.size() == 1 or str[1] == '/';
	return false;
}

static spin_shared_mutex g_env_mutex;

sys_expected<std::string> getenv(std::string_view key) noexcept
{
	g_env_mutex.lock_shared();
	auto value = ::getenv(key.data());
	g_env_mutex.unlock_shared();

	sys_expected<std::string> result {""};
	if( value )
		result = value;
	else
		result.despair(sys_error());
	return result;
}

sys_expected<std::map<std::string,std::string>> getenvs() noexcept
{
	std::map<std::string,std::string> envs;
	g_env_mutex.lock_shared();

	for(int i=0; environ[i]!=nullptr; i++)
	{
		std::string tmp = environ[i];
		auto pos = tmp.find('=');

		if( pos == std::string::npos )
			envs.emplace(tmp, "");
		else
			envs.emplace(tmp.substr(0,pos), tmp.substr(pos+1));
	}
	g_env_mutex.unlock_shared();
	return envs;
}

sys_expected<> setenv(std::string_view key, const libgs::value &value, bool overwrite) noexcept
{
	sys_expected<> result;
	spin_shared_unique_lock locker(g_env_mutex);

	if( ::setenv(key.data(), value->c_str(), overwrite) != 0 )
		result.despair(sys_error());
	return result;
}

sys_expected<> unsetenv(std::string_view key) noexcept
{
	sys_expected<> result;
	spin_shared_unique_lock locker(g_env_mutex);

	if( ::unsetenv(key.data()) != 0 )
		result.despair(sys_error());
	return result;
}

sys_expected<std::string> current_user() noexcept
{
	auto uid = getuid();
	passwd pwd {};
	passwd *result = nullptr;
	char buf[1024] {0};

	auto res = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);
	sys_expected<std::string> expected {};

	if( res != 0 or not result )
		return expected.despair(sys_error());

	expected = pwd.pw_name;
	return expected;
}

sys_expected<path_t> home_directory() noexcept
{
	auto uid = getuid();
	passwd pwd {};
	passwd *result = nullptr;
	char buf[1024] {0};

	auto res = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);
	sys_expected<path_t> expected {};

	if( res != 0 or not result )
		return expected.despair(sys_error());

	std::string path {};
	if( pwd.pw_dir and strlen(pwd.pw_dir) > 0 )
		path = pwd.pw_dir;
	else
	{
		auto home = ::getenv("HOME");
		if( home and strlen(home) > 0 )
			path = home;
		else
			return expected.despair(sys_error());
	}
	if( path.ends_with("/") )
		path.pop_back();

	expected = std::move(path);
	return expected;
}

} //namespace libgs::app

#endif //__unix__
