
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

#ifndef LIBGS_IO_TCP_SERVER_H
#define LIBGS_IO_TCP_SERVER_H

#include <libgs/io/tcp_socket.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor = asio::basic_socket_acceptor<asio::ip::tcp, Exec>;

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor_ptr = std::shared_ptr<asio_basic_tcp_acceptor<Exec>>;

using asio_tcp_acceptor = asio_basic_tcp_acceptor<>;
using asio_tcp_acceptor_ptr = asio_basic_tcp_acceptor_ptr<>;

namespace io
{

template <concept_execution Exec = asio::any_io_executor>
class basic_tcp_server : public device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_tcp_server)
	using base_type = device_base<Exec>;

public:
	using executor_type = base_type::executor_type;
	using asio_acceptor = asio_basic_tcp_acceptor<Exec>;
	using asio_acceptor_ptr = asio_basic_tcp_acceptor_ptr<Exec>;

public:
	template <concept_execution_context Context = asio::io_context>
	explicit basic_tcp_server(Context &context = io_context());

	template <concept_execution Exec0>
	basic_tcp_server(asio_acceptor &&acceptor);

	explicit basic_tcp_server(const executor_type &exec);
	~basic_tcp_server() override;

public:
	// TODO ...

protected:
	template <typename ASIO_Acceptor, concept_callable Func>
	explicit basic_tcp_server(ASIO_Acceptor *acceptor, Func &&del_acceptor);

private:
	const asio_acceptor &acceptor() const;
	asio_acceptor &acceptor();

protected:
	void *m_acceptor;
	std::function<void()> m_del_acceptor {
		[this]{ delete reinterpret_cast<asio_acceptor*>(m_acceptor); }
	};
};

using tcp_server = basic_tcp_server<>;

} //namespace io

} //namespace libgs
// #include <libgs/io/detail/tco_server.h>

namespace libgs::io
{

template <concept_execution Exec>
template <concept_execution_context Context>
basic_tcp_server<Exec>::basic_tcp_server(Context &context) :
	base_type(context.get_executor()),
	m_acceptor(new asio_acceptor(context))
{

}

template <concept_execution Exec>
template <concept_execution Exec0>
basic_tcp_server<Exec>::basic_tcp_server(asio_acceptor &&acceptor) :
	base_type(acceptor.get_executor()),
	m_acceptor(new asio_acceptor(std::move(acceptor)))
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::basic_tcp_server(const executor_type &exec) :
	base_type(exec),
	m_acceptor(new asio_acceptor(exec))
{

}

template <concept_execution Exec>
basic_tcp_server<Exec>::~basic_tcp_server()
{
	if( m_acceptor and m_del_acceptor )
		m_del_acceptor();

	// TODO ...
}


// TODO ...


template <concept_execution Exec>
template <typename ASIO_Acceptor, concept_callable Func>
basic_tcp_server<Exec>::basic_tcp_server(ASIO_Acceptor *acceptor, Func &&del_acceptor) :
	base_type(acceptor->get_executor()),
	m_acceptor(acceptor),
	m_del_acceptor(std::forward<Func>(del_acceptor))
{
	assert(acceptor);
	assert(m_del_acceptor);
}

template <concept_execution Exec>
const typename basic_tcp_server<Exec>::asio_acceptor &basic_tcp_server<Exec>::acceptor() const
{
	return *reinterpret_cast<const asio_acceptor*>(m_acceptor);
}

template <concept_execution Exec>
typename basic_tcp_server<Exec>::asio_acceptor &basic_tcp_server<Exec>::acceptor()
{
	return *reinterpret_cast<asio_acceptor*>(m_acceptor);
}

} //namespace libgs::io


#endif //LIBGS_IO_TCP_SERVER_H
