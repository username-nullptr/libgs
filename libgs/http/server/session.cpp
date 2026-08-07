
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#include "session.h"

namespace libgs::http
{

session::session(const executor_t &exec) :
	session(std::chrono::seconds(60), exec)
{

}

session::~session()
{
	delete m_impl;
}

std::string_view session::id() const noexcept
{
	return m_impl->m_id;
}

session::time_point_t session::create_time() const noexcept
{
	return m_impl->m_create_time;
}

bool session::is_valid() const noexcept
{
	return m_impl->m_valid;
}

const session::attributes_t &session::attributes() const noexcept
{
	return m_impl->m_attributes;
}

session::attributes_t &session::attributes() noexcept
{
	return m_impl->m_attributes;
}

std::chrono::seconds session::lifecycle() const noexcept
{
	return std::chrono::seconds(m_impl->m_second);
}

void session::invalidate()
{
	m_impl->m_valid = false;
	m_impl->m_timer.cancel();
}

session &session::expand()
{
	m_impl->m_restart = true;
	m_impl->start();
	return *this;
}

session &session::unbind_timeout()
{
	m_impl->m_timeout_handle = nullptr;
	return *this;
}

session &session::unbind_error()
{
	m_impl->m_error_handle = nullptr;
	return *this;
}

} //namespace libgs::http
