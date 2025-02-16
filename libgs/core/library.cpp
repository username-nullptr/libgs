
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

#include "detail/library_impl.hii"

namespace fs = std::filesystem;

namespace libgs
{

library::library(fs::path file_name, std::string_view version) :
	m_impl(new impl(std::move(file_name), version))
{

}

library::~library()
{
	delete m_impl;
}

library::library(library &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

library &library::operator=(library &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

void library::load(error_code &error) noexcept
{
	m_impl->load(error);
}

void library::load()
{
	error_code error;
	load(error);
	if( error )
		throw system_error(error, "Cannot load library: '{}'", file_name());
}

void library::unload(error_code &error) noexcept
{
	m_impl->load(error);
}

void library::unload()
{
	error_code error;
	unload(error);
	if( error )
		throw system_error(error, "Cannot unload library: '{}'", file_name());
}

void *library::interface(std::string_view ifname) const
{
	if( not is_loaded() )
		throw runtime_error("libgs::library::interface: dll not load.");
	return m_impl->interface(ifname);
}

bool library::exists(std::string_view ifname) const
{
	if( not is_loaded() )
		throw runtime_error("libgs::library::exists: dll not load.");
	return m_impl->exists(ifname);
}

bool library::is_loaded() const noexcept
{
	return m_impl->m_handle != nullptr;
}

fs::path library::file_name() const noexcept
{
	return m_impl->m_file_name;
}

} //namespace libgs
