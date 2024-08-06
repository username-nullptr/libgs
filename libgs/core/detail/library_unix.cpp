
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

#include "library_impl.hii"
#include <libgs/core/app_utls.h>
#include <filesystem>
#include <dlfcn.h>

namespace fs = std::filesystem;

namespace libgs
{

std::vector<std::string> get_suffixes_sys(std::string_view version)
{
	std::vector<std::string> result;
#ifdef __APPLE__
	if( version.empty() )
	{
		result.emplace_back(".bundle");
		result.emplace_back(".dylib");
	}
	else
	{
		result.emplace_back(std::string(".") + version + ".bundle");
		result.emplace_back(std::string(".") + version + ".dylib");
	}
#else
	if( version.empty() )
		result.emplace_back(".so");
	else
		result.emplace_back(std::string(".so.") + version.data());
#endif
	return result;
}

static class library_category : public std::error_category
{
	LIBGS_DISABLE_COPY_MOVE(library_category)

public:
	library_category() = default;
	~library_category() override = default;

public:
	[[nodiscard]] const char *name() const noexcept override {
		return "libgs::library_error_category";
	}

	[[nodiscard]] std::string message(int) const override
	{
		auto err = dlerror();
		return err? err : "";
	}
}
g_library_category;

void *library::impl::interface(std::string_view ifname) const
{

}

bool library::impl::exists(std::string_view ifname) const
{

}

void library::impl::load_native(error_code &error)
{
	error = error_code();
	std::vector<std::string> prefix_list = {"lib"};
	std::vector<std::string> suffix_list = get_suffixes_sys(m_version);

	if( app::is_absolute_path(m_file_name) )
	{
		suffix_list.insert(suffix_list.begin(), std::string());
		prefix_list.insert(prefix_list.begin(), std::string());
	}
	else
	{
		suffix_list.emplace_back();
		prefix_list.emplace_back();
	}
	int dl_flags = RTLD_NOW;
	auto retry = true;
	std::string attempt;

	for(auto prefix=0u; retry and not m_handle and prefix<prefix_list.size(); ++prefix)
	{
		for(auto suffix=0u; retry and not m_handle and suffix<suffix_list.size(); ++suffix)
		{
			if( (not prefix_list[prefix].empty() and m_file_name.starts_with(prefix_list[prefix])) or
				(not suffix_list[suffix].empty() and m_file_name.ends_with(suffix_list[suffix])) )
				continue;

			attempt = prefix_list[prefix] + m_file_name + suffix_list[suffix];
			m_handle = dlopen(attempt.c_str(), dl_flags);

			if( not m_handle and app::is_absolute_path(m_file_name) and fs::exists(attempt) )
				retry = false;
		}
	}
	if( m_handle )
		m_qualifed_file_name = attempt;
	else
		error = error_code(errno, g_library_category);
}

void library::impl::unload_native(error_code &error)
{
	error = error_code();
	if( dlclose(m_handle) )
		error = error_code(errno, g_library_category);
	else
		m_qualifed_file_name.clear();
}

} //namespace libgs

#endif //__unix__