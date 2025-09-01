
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

#ifndef LIBGS_CORE_DETAIL_LIBRARY_IMPL_HII
#define LIBGS_CORE_DETAIL_LIBRARY_IMPL_HII

#include <libgs/core/library.h>

#if defined(__WINNT__) || defined(_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
#endif //Windows

namespace libgs
{

class LIBGS_DECL_HIDDEN library::impl
{
	LIBGS_DISABLE_COPY(impl)

	using handle_t =
#if defined(__WINNT__) || defined(_WINDOWS)
	HMODULE
#else // Unix/Linux
	void*
#endif //OS
	;

public:
	impl(path_t file_name, std::string_view version)
#ifdef __linux__
		: m_version(version) {
#else
	{
		ignore_unused(version);
#endif //__linux__
		set_file_name(std::move(file_name));
	}

	impl(impl &&other) noexcept :
		m_file_name(std::move(other.m_file_name)),
		m_load_count(other.m_load_count.load()),
		m_handle(other.m_handle)
#ifdef __linux__
		, m_version(std::move(other.m_version))
#endif //__linux__
	{}
	impl &operator=(impl &&other) noexcept
	{
		m_file_name = std::move(other.m_file_name);
		m_load_count = other.m_load_count.load();
		m_handle = other.m_handle;
#ifdef __linux__
		m_version = std::move(other.m_version);
#endif //__linux__
		return *this;
	}

public:
	[[nodiscard]] error_code load()
	{
		error_code error;
		if( m_handle )
		{
			++m_load_count;
			return error;
		}
		error = load_native();
		if( not error )
			++m_load_count;
		return error;
	}

	[[nodiscard]] error_code unload()
	{
		error_code error;
		if( not m_handle )
			return error;
		--m_load_count;

		if( not m_load_count )
		{
			error = unload_native();
			if( not error )
				m_handle = nullptr;
		}
		return error;
	}

public:
	[[nodiscard]] void *interface(std::string_view ifname) const;
	[[nodiscard]] bool exists(std::string_view ifname) const;

private:
	void set_file_name(path_t file_name);
	[[nodiscard]] error_code load_native();
	[[nodiscard]] error_code unload_native();

public:
	path_t m_file_name;
	std::atomic_int m_load_count {0};
	handle_t m_handle = nullptr;

#ifdef __linux__
	std::string m_version;
#endif //__linux__
};

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_LIBRARY_IMPL_HII
