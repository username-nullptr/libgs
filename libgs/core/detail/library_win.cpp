
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2014 - 2018 Axel Menzel <info@rttr.org>                           *
*                                                                                   *
*   This file is part of RTTR (Run Time Type Reflection)                            *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
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
// https://github.com/rttrorg/rttr

#if defined(__WINNT__) || defined(_WINDOWS)

#include "library_impl.hii"
#include <libgs/core/app_utls.h>

namespace libgs
{

static std::wstring convert_utf8_to_utf16(std::string_view source)
{
	if( source.empty() )
		return {};

	const auto size_needed = ::MultiByteToWideChar(CP_UTF8, 0, source.data(), static_cast<int>(source.size()), nullptr, 0);
	std::wstring result(size_needed, 0);

	::MultiByteToWideChar(CP_UTF8, 0, source.data(), static_cast<int>(source.size()), result.data(), size_needed);
	return result;
}

static std::string convert_utf16_to_utf8(std::wstring_view source)
{
	if( source.empty() )
		return {};

	const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, source.data(), static_cast<int>(source.size()), nullptr, 0, nullptr, nullptr);
	std::string result(size_needed, 0);

	WideCharToMultiByte(CP_UTF8, 0, source.data(), static_cast<int>(source.size()), result.data(), size_needed, nullptr, nullptr);
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

	[[nodiscard]] std::string message(int code) const override
	{
		std::string result;
		if( code == -1 )
			code = ::GetLastError();
		switch(code)
		{
		case 0     :                                     ; break;
		case EACCES: result = "Permission denied"        ; break;
		case EMFILE: result = "Too many open files"      ; break;
		case ENOENT: result = "No such file or directory"; break;
		case ENOSPC: result = "No space left on device"  ; break;
		default: {
			wchar_t error_str[1024] {0};
			const auto buffer_size =
					FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
								   nullptr,
								   code,
								   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								   reinterpret_cast<LPWSTR>(&error_str),
								   sizeof(error_str) / sizeof(wchar_t),
								   nullptr);
			result = convert_utf16_to_utf8(std::wstring(error_str, error_str + buffer_size));
			break;}
		}
		return result;
	}
}
g_library_category;

void library::impl::load_native(error_code &error)
{
	error = error_code();
	std::wstring wfile_name = convert_utf8_to_utf16(m_file_name);
	std::vector<std::wstring> paths_to_try;

	std::vector<std::wstring> prefix_list = {L"lib"};
	std::vector<std::wstring> suffix_list = {L".dll"};

	if( app::is_absolute_path(m_file_name) )
	{
		prefix_list.insert(prefix_list.begin(), std::wstring());
		suffix_list.insert(suffix_list.begin(), std::wstring());
	}
	else
	{
		prefix_list.insert(prefix_list.begin(), std::wstring());
		suffix_list.emplace_back();
	}
	std::wstring attempt;
	auto retry = true;

	for(auto prefix=0u; retry and not m_handle and prefix<prefix_list.size(); ++prefix)
	{
		for(auto suffix=0u; retry and not m_handle and suffix<suffix_list.size(); ++suffix)
		{
			if( (not prefix_list[prefix].empty() and wfile_name.starts_with(prefix_list[prefix])) or
				(not suffix_list[suffix].empty() and wfile_name.ends_with(suffix_list[suffix])) )
				continue;

			attempt = prefix_list[prefix] + wfile_name + suffix_list[suffix];
			m_handle = LoadLibraryW(attempt.c_str());

			if( ::GetLastError() != ERROR_MOD_NOT_FOUND )
				retry = false;
		}
	}
	if( m_handle )
		error = error_code(-1, g_library_category);
	else
		m_qualifed_file_name = convert_utf16_to_utf8(attempt);
}

void library::impl::unload_native(error_code &error)
{
	error = error_code();
	if( FreeLibrary(m_handle) )
		m_qualifed_file_name.clear();
	else
		error = error_code(-1, g_library_category);
}

} //namespace libgs

#endif //Windows