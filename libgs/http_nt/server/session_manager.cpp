
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

#include "session_manager.h"

namespace libgs::http_nt
{

session_ptr session_manager::impl::find(std::string_view id, bool _throw)
{
	spin_shared_shared_lock locker(m_map_mutex); LIBGS_UNUSED(locker);
	auto it = m_session_map.find(id);

	if( it == m_session_map.end() )
	{
		if( _throw )
		{
			throw runtime_error (
				"libgs::http_nt::session_manager: <map>: id '{}' not exists.", id
			);
		}
		return {};
	}
	it->second->expand();
	return it->second;
}

std::pair<std::map<std::string_view,session_ptr>::iterator,bool>
session_manager::impl::emplace(session_ptr session)
{
	m_map_mutex.lock();
	auto pair = m_session_map.emplace(session->id(), std::move(session));
	m_map_mutex.unlock();
	return pair;
}

void session_manager::impl::erase(std::string_view id)
{
	m_map_mutex.lock();
	m_session_map.erase(std::string(id.data(), id.size()));
	m_map_mutex.unlock();
}

session_manager::session_manager() :
	m_impl(new impl())
{

}

session_manager::~session_manager()
{
	delete m_impl;
}

session_manager::session_manager(session_manager &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

session_manager &session_manager::operator=(session_manager &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

std::chrono::seconds session_manager::lifecycle() const noexcept
{
	return std::chrono::seconds(m_impl->m_lifecycle);
}

std::string_view session_manager::cookie_key() const noexcept
{
	return m_impl->m_cookie_key;
}

session_manager &session_manager::unbind_error()
{
	m_impl->m_error_handle = nullptr;
	return *this;
}

} //namespace libgs::http_nt
