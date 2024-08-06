
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

#include "library_impl.hii"
#include <libgs/core/app_utls.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace libgs
{

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

	[[nodiscard]] std::string message(int code) const override
	{
		std::string result;
		switch(code)
		{
		case 0     : result = "Success"                  ; break;
		case EACCES: result = "Permission denied"        ; break;
		case EMFILE: result = "Too many open files"      ; break;
		case ENOENT: result = "No such file or directory"; break;
		case ENOSPC: result = "No space left on device"  ; break;
		default: {
			char error_str[1024] {0};
			const auto buffer_size =
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
								  nullptr,
								  code,
								  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								  reinterpret_cast<LPSTR>(&error_str),
								  sizeof(error_str),
								  nullptr);
			result = std::string(error_str, buffer_size);
			break;}
		}
		return result;
	}
}
g_library_category;

void *library::impl::interface(std::string_view ifname) const
{
	return reinterpret_cast<void*>(GetProcAddress(m_handle, ifname.data()));
}

bool library::impl::exists(std::string_view ifname) const
{
	return reinterpret_cast<void*>(GetProcAddress(m_handle, ifname.data())) != nullptr;
}

void library::impl::set_file_name(std::string_view file_name)
{
	m_file_name = std::string(file_name.data(), file_name.size());
	std::vector<std::string> candidates
	{
		m_file_name,
		m_file_name + ".dll",
	};
	if( m_file_name.find('/') == std::string_view::npos )
	{
		candidates.emplace_back("lib" + m_file_name);
		candidates.emplace_back("lib" + m_file_name + ".dll");
	};
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
	std::wstring wfile_name = mbstowcs(m_file_name);
	m_handle = LoadLibraryW(wfile_name.c_str());
	if( m_handle )
		error = error_code(static_cast<int>(GetLastError()), g_library_category);
}

void library::impl::unload_native(error_code &error)
{
	error = error_code();
	if( not FreeLibrary(m_handle) )
		error = error_code(static_cast<int>(GetLastError()), g_library_category);
}

} //namespace libgs

#endif //Windows