
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
#include "libgs/core/log.h"
#include <unistd.h>

/* extern char **environ; */

namespace libgs::app
{

std::string file_path()
{
	char exe_name[1024] = "";
	if( readlink("/proc/self/exe", exe_name, sizeof(exe_name)) < 0 )
		libgs_log_error("libgs::appinfo::file_path: readlink: {}. ({})", strerror(errno), errno);
	return exe_name;
}

bool set_current_directory(std::string_view path)
{
	if( chdir(path.data()) < 0 )
	{
		libgs_log_error("libgs::appinfo::set_current_directory: chdir: {}. ({})", strerror(errno), errno);
		return false;
	}
	return true;
}

std::string current_directory()
{
	char buf[1024] = "";
	if( getcwd(buf, sizeof(buf)) == nullptr )
		libgs_log_error("libgs::appinfo::current_directory: getcwd: {}. ({})", strerror(errno), errno);
	std::string str(buf);
	if( not str.ends_with("/") )
		str += "/";
	return str;
}

std::string absolute_path(std::string_view path)
{
	std::string result(path.data(), path.size());
	if( not path.starts_with("/") )
	{
		if( path.starts_with("~/") )
		{
			auto tmp = ::getenv("HOME");
			if( tmp == nullptr )
				libgs_log_fatal("System environment 'HOME' is null.");

			std::string home(tmp);
			if( not home.ends_with("/") )
				home += "/";
			result = home + result.erase(0,2);
		}
		else
			result = dir_path() + result;
	}
	return result;
}

using envs_t = std::map<std::string, std::string>;

envs_t getenvs()
{
	envs_t envs;
	for(int i=0; environ[i]!=nullptr; i++)
	{
		std::string tmp = environ[i];
		auto pos = tmp.find("=");

		if( pos == tmp.npos )
			envs.emplace(tmp, "");
		else
			envs.emplace(tmp.substr(0,pos), tmp.substr(pos+1));
	}
	return envs;
}

} //namespace libgs::app

#endif //__unix__
