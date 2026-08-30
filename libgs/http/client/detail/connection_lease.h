
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_LEASE_H
#define LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_LEASE_H

namespace libgs::http
{

template <core_concepts::exec Exec>
class LIBGS_HTTP_TAPI basic_connection_lease<Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl(connection_ptr conn, std::function<void(connection_ptr)> give_back) :
		m_give_back(std::move(give_back)),
		m_connection(std::move(conn)) {}

	std::function<void(connection_ptr)> m_give_back {};
	connection_ptr m_connection {};
};

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::basic_connection_lease
(connection_ptr conn, std::function<void(connection_ptr)> give_back) :
	m_impl(new impl(std::move(conn), std::move(give_back)))
{

}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::~basic_connection_lease()
{
	if( m_impl->m_connection )
	{
		auto conn = std::move(m_impl->m_connection);
		if( m_impl->m_give_back )
		{
			try { m_impl->m_give_back({}); }
			catch(...) {}
		}
		ignore_unused(conn->close());
	}
	delete m_impl;
}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::connection_t&
basic_connection_lease<Exec>::get() noexcept
{
	if( not m_impl->m_connection )
		std::terminate();
	return *m_impl->m_connection;
}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::connection_t&
basic_connection_lease<Exec>::operator*() noexcept
{
	return get();
}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::connection_t*
basic_connection_lease<Exec>::operator->() noexcept
{
	return &get();
}

template <core_concepts::exec Exec>
const basic_connection_lease<Exec>::connection_t&
basic_connection_lease<Exec>::get() const noexcept
{
	if( not m_impl->m_connection )
		std::terminate();
	return *m_impl->m_connection;
}

template <core_concepts::exec Exec>
const basic_connection_lease<Exec>::connection_t&
basic_connection_lease<Exec>::operator*() const noexcept
{
	return get();
}

template <core_concepts::exec Exec>
const basic_connection_lease<Exec>::connection_t*
basic_connection_lease<Exec>::operator->() const noexcept
{
	return &get();
}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::connection_ptr
basic_connection_lease<Exec>::take() noexcept
{
	if( not m_impl->m_connection )
		return {};

	auto conn = std::move(m_impl->m_connection);
	if( m_impl->m_give_back )
	{
		try {
			m_impl->m_give_back({});
		}
		catch(...) {}
		m_impl->m_give_back = {};
	}
	return conn;
}

template <core_concepts::exec Exec>
void basic_connection_lease<Exec>::release()
{
	if( not m_impl->m_connection )
		return ;

	auto conn = std::move(m_impl->m_connection);
	if( not m_impl->m_give_back )
	{
		ignore_unused(conn->close());
		return ;
	}
	auto give_back = std::move(m_impl->m_give_back);
	try {
		give_back(std::move(conn));
	}
	catch(...)
	{
		if( conn )
			ignore_unused(conn->close());
		throw ;
	}
}

template <core_concepts::exec Exec>
bool basic_connection_lease<Exec>::is_valid() const noexcept
{
	return static_cast<bool>(m_impl->m_connection);
}

template <core_concepts::exec Exec>
basic_connection_lease<Exec>::operator bool() const noexcept
{
	return is_valid();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_LEASE_H
