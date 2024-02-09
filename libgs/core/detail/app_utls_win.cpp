#error "< TODO ... >"

#if defined(__WINNT__) || defined(_WINDOWS)
#include <Windows.h>

#include "app_info.h"
#include "libgs/core/algorithm/base.h"

#include <iostream>
#include <cstring>
#include <format>

namespace libgs::app
{

std::string file_path()
{
	char buf[1024] = "";
	if( QueryFullProcessImageName(nullptr, 0, buf, 1023) == FALSE )
		std::cerr << std::format("*** Error: appinfo::file_path: QueryFullProcessImageName: {}. ({})", strerror(errno), errno) << std::endl;
	return buf;
}

std::string tmp_dir_path()
{
	return dir_path() + instance_name() + "/";
}

std::string current_directory()
{
	char buf[1024] = "";
	if( GetCurrentDirectory(1023, buf) == 0 )
		std::cerr << std::format("*** Error: appinfo::current_directory: GetCurrentDirectory: {}. ({})", strerror(errno), errno) << std::endl;
	return buf;
}

bool set_current_directory(const std::string& path)
{
	if( SetCurrentDirectory(path.c_str()) )
	{
		std::cerr << std::format("*** Error: appinfo::set_current_directory: SetCurrentDirectory: {}. ({})", strerror(errno), errno) << std::endl;
		return false;
	}
	return true;
}

} //namespace libgs::app


#endif //__WINNT__ || _WINDOWS
