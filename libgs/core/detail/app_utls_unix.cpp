
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

#ifdef __unix__

#include "libgs/core/app_utls.h"
#include "libgs/core/shared_mutex.h"
#include "libgs/core/algorithm/base.h"
#include "libgs/core/log.h"
#include <unistd.h>

/* extern char **environ; */

namespace libgs::app
{

std::string file_path()
{
	char exe_name[1024] = "";
	if( readlink("/proc/self/exe", exe_name, sizeof(exe_name)) < 0 )
		libgs_log_error("libgs::app::file_path: readlink: {}. ({})", strerror(errno), errno);
	return exe_name;
}

bool set_current_directory(std::string_view path)
{
	if( chdir(path.data()) < 0 )
	{
		libgs_log_error("libgs::app::set_current_directory: chdir: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

std::string current_directory()
{
	char buf[1024] = "";
	if( getcwd(buf, sizeof(buf)) == nullptr )
		libgs_log_error("libgs::app::current_directory: getcwd: {}. ({})", strerror(errno), errno);
	std::string str(buf);
	if( not str.ends_with("/") )
		str += "/";
	return str;
}

bool is_absolute_path(std::string_view path)
{
	if( path.starts_with("/") )
		return true;
	else if( path.starts_with("~") )
		return path.size() == 1 or path[1] == '/';
	return false;
}

std::string absolute_path(std::string_view path)
{
	std::string result(path.data(), path.size());
	if( not is_absolute_path(path) )
		result = dir_path() + result;

	else if( path.starts_with("~") )
	{
		auto tmp = ::getenv("HOME");
		if( tmp == nullptr )
			libgs_log_fatal("libgs::app::absolute_path: System environment 'HOME' is null.");

		std::string home(tmp);
		if( home.ends_with("/") )
			home.pop_back();

		result = home + result.erase(0,1);
	}
	if( not result.ends_with("/") )
		result += "/";

	str_replace(result, "/./", "/", false);
	str_replace(result, "//", "/", false);
	return result;
}

using optional_string = std::optional<std::string>;

using envs_t = std::map<std::string, std::string>;

static shared_mutex g_env_mutex;

optional_string getenv(std::string_view key)
{
	g_env_mutex.lock_shared();
	auto value = ::getenv(key.data());

	g_env_mutex.unlock();
	return value? optional_string(value) : optional_string();
}

envs_t getenvs()
{
	envs_t envs;
	g_env_mutex.lock_shared();

	for(int i=0; environ[i]!=nullptr; i++)
	{
		std::string tmp = environ[i];
		auto pos = tmp.find("=");

		if( pos == tmp.npos )
			envs.emplace(tmp, "");
		else
			envs.emplace(tmp.substr(0,pos), tmp.substr(pos+1));
	}
	g_env_mutex.unlock();
	return envs;
}

bool setenv(std::string_view key, std::string_view value, bool overwrite)
{
	unique_lock locker(g_env_mutex);
	if( ::setenv(key.data(), value.data(), overwrite) != 0 )
	{
		libgs_log_error("libgs::appinfo::setenv: setenv: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

bool unsetenv(std::string_view key)
{
	unique_lock locker(g_env_mutex);
	if( ::unsetenv(key.data()) != 0 )
	{
		libgs_log_error("libgs::appinfo::unsetenv: unsetenv: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

} //namespace libgs::app

#endif //__unix__
