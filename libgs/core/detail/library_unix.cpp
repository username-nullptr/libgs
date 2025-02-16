
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
#include <dlfcn.h>

namespace fs = std::filesystem;

namespace libgs
{

static class LIBGS_DECL_HIDDEN library_category : public std::error_category
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
	return dlsym(m_handle, ifname.data());
}

bool library::impl::exists(std::string_view ifname) const
{
	return dlsym(m_handle, ifname.data()) != nullptr;
}

void library::impl::set_file_name(fs::path file_name)
{
	m_file_name = std::move(file_name);
    auto _file_name = m_file_name.string();
#ifdef __APPLE__
	std::vector candidates
	{
		_file_name + ".bundle",
		_file_name + ".dylib",
		_file_name + "." + m_version + ".bundle",
		_file_name + "." + m_version + ".dylib"
	};
#else
	std::vector candidates
	{
		_file_name,
		_file_name + ".so",
		_file_name + ".so." + m_version
	};
	if( _file_name.find('/') == std::string_view::npos )
	{
		candidates.emplace_back("lib" + _file_name);
		candidates.emplace_back("lib" + _file_name + ".so");
		candidates.emplace_back("lib" + _file_name + ".so." + m_version);
	}
#endif
	for(auto &name : candidates)
	{
		auto abs_name = app::absolute_path(name);
		if( not fs::exists(abs_name) )
			continue;

		m_file_name = std::move(abs_name);
		break;
	}
}

void library::impl::load_native(error_code &error)
{
	error = error_code();
	if( not fs::exists(m_file_name) )
	{
		error = std::make_error_code(std::errc::no_such_file_or_directory);
		return ;
	}
    auto _file_name = m_file_name.string();
	m_handle = dlopen(_file_name.c_str(), RTLD_NOW);
	if( not m_handle )
		error = error_code(errno, g_library_category);
}

void library::impl::unload_native(error_code &error)
{
	error = error_code();
	if( dlclose(m_handle) )
		error = error_code(errno, g_library_category);
}

} //namespace libgs

#endif //__unix__