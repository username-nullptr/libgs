
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SERVER_H
#define LIBGS_HTTP_SERVER_DETAIL_SERVER_H

#include <libgs/core/lock_free_queue.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using request_queue = lock_free_queue<request_ptr>;
	using callback_queue = lock_free_queue<callback_t<request_ptr,error_code>>;

public:
	template <typename...Args>
	explicit impl(Args&&...args) requires requires { tcp_server_type(std::forward<Args>(args)...); } :
		impl(std::make_shared<tcp_server_type>(std::forward<Args>(args)...)) {}

	explicit impl(tcp_server_ptr &&tcp_server) :
		m_tcp_server(std::move(tcp_server)) {}

public:
	void async_start(size_t max) noexcept
	{
		if( m_is_start )
			return ;
		m_is_start = true;

		co_spawn_detached([this, max]() -> awaitable<void>
		{
			try { co_await do_tcp_accept(max); }
			catch(...) {}

			co_await m_tcp_server->co_cancel();
			m_is_start = false;
			co_return ;
		});
	}

private:
	awaitable<void> do_tcp_accept(size_t max)
	{
		m_tcp_server->start(max);
		for(;;)
		{
			auto socket = co_await m_tcp_server->accept();
			auto ktime = m_keepalive_timeout;

			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = std::move(ktime)]() -> awaitable<void>
			{
				try { co_await do_tcp_service(std::move(socket), ktime); }
				catch(...) {}

				error_code error;
				socket->close(error);
			},
			m_tcp_server->pool());
		}
		co_return ;
	}

	awaitable<void> do_tcp_service(io::tcp_socket_ptr socket, const std::chrono::milliseconds &keepalive_time)
	{
		basic_server::parser parser;
		error_code error;

		char buf[65536] = {0};
		for(;;)
		{
			auto size = co_await socket->read({buf, 65536}, keepalive_time);
			if( size == 0 )
				break;

			bool parse_res = parser.append({buf, size}, error);
			if( error )
			{
				if( not m_is_start )
					break;
				else if( m_error_callback )
					co_await m_error_callback(error);
				else
					throw system_error(error, "libgs::http::server::impl::do_tcp_service");
			}
			else if( parse_res and m_request_callback )
			{
				auto req = std::make_shared<basic_server::request>(socket, parser);
				basic_server::response resp(req);

				co_await m_request_callback(*req, resp);
				if( not resp.headers_writed() )
					co_await resp.write();
				parser.reset();
			}
		}
		co_return ;
	}

public:
	tcp_server_ptr m_tcp_server {};
	request_handler m_request_callback {};
	error_handler m_error_callback {};

	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(size_t tcount) :
	base_type(execution::io_context().get_executor()),
	m_impl(new impl(tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution_context Context>
basic_server<CharT,Exec>::basic_server(Context &context, size_t tcount) :
	base_type(context.get_executor()),
	m_impl(new impl(context, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution Exec0>
basic_server<CharT,Exec>::basic_server(asio_acceptor &&acceptor, size_t tcount) :
	base_type(acceptor.get_executor()),
	m_impl(new impl(std::move(acceptor), tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(const executor_type &exec, size_t tcount) :
	base_type(exec),
	m_impl(new impl(exec, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(tcp_server_ptr tcp_server) :
	base_type(tcp_server.executor()),
	m_impl(new impl(std::move(tcp_server)))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::~basic_server()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::bind(io::ip_endpoint ep, opt_token<error_code&> tk)
{
	m_impl->m_tcp_server->bind(std::move(ep), tk);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::start(start_token tk)
{
	m_impl->async_start(tk.max);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_request(request_handler callback) noexcept
{
	m_impl->m_request_callback = std::move(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_error(error_handler callback) noexcept
{
	m_impl->m_error_callback = std::move(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_request() noexcept
{
	m_impl->m_request_callback = {};
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_error() noexcept
{
	m_impl->m_error_callback = {};
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename Rep, typename Period>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::set_keepalive_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_keepalive_timeout = duration_cast<milliseconds>(d);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<void> basic_server<CharT,Exec>::co_cancel() noexcept
{
	m_impl->m_is_start = false;
	co_await m_impl->m_tcp_server->co_cancel();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::cancel() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_tcp_server->cancel();
}

template <concept_char_type CharT, concept_execution Exec>
const io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object() const
{
	return *m_impl->m_tcp_server;
}

template <concept_char_type CharT, concept_execution Exec>
io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object()
{
	return *m_impl->m_tcp_server;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
