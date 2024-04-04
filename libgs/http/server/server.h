
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

#ifndef LIBGS_HTTP_SERVER_SERVER_H
#define LIBGS_HTTP_SERVER_SERVER_H

#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>
#include <libgs/io/tcp_server.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY(basic_server)
	using base_type = io::device_base<Exec>;

public:
	using parser = basic_request_parser<CharT>;
	using requset = basic_server_request<CharT>;
	using response = basic_server_response<CharT>;

	using request_ptr = basic_server_request_ptr<CharT,Exec>;
	using response_ptr = basic_server_response_ptr<CharT,Exec>;

	using executor_type = typename base_type::executor_type;
	using tcp_server_type = io::basic_tcp_server<Exec>;
	using tcp_server_ptr = io::basic_tcp_server_ptr<Exec>;

	using bind_token = typename tcp_server_type::bind_token;
	using tcp_socket_ptr = io::tcp_socket_ptr;

	using asio_acceptor = asio_basic_tcp_acceptor<Exec>;
	using asio_acceptor_ptr = asio_basic_tcp_acceptor_ptr<Exec>;

public:
	explicit basic_server(size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution_context Context>
	explicit basic_server(Context &context, size_t tcount = std::thread::hardware_concurrency() << 1);

	template <concept_execution Exec0>
	explicit basic_server(asio_acceptor &&acceptor, size_t tcount = std::thread::hardware_concurrency() << 1);

	explicit basic_server(const executor_type &exec, size_t tcount = std::thread::hardware_concurrency() << 1);
	~basic_server() override;

public:
	void bind(io::ip_endpoint ep, bind_token tk = {});
	void async_accept(opt_token<callback_t<request_ptr,response_ptr,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<bool> co_accept(request_ptr &request, response_ptr &response, opt_token<error_code&> tk = {});
	bool accept(request_ptr &request, response_ptr &response, opt_token<error_code&> tk = {});


public:
	awaitable<void> co_cancel() noexcept;
	void cancel() noexcept override;

public:
	[[nodiscard]] const tcp_server_type &native_object() const;
	[[nodiscard]] tcp_server_type &native_object();

protected:
	explicit basic_server(tcp_server_ptr tcp_server);

private:
	class impl;
	impl *m_impl;
};

using server = basic_server<char>;
using wserver = basic_server<wchar_t>;

} //namespace libgs::http
//#include <libgs/http/server/detail/server.h>

#include <libgs/core/lock_free_queue.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	template <typename...Args>
	explicit impl(Args&&...args) requires requires { tcp_server_type(std::forward<Args>(args)...); } :
		impl(std::make_shared<tcp_handle_type>(std::forward<Args>(args)...)) {}

	explicit impl(tcp_server_ptr &&tcp_server) :
		m_tcp_server(std::move(tcp_server))
	{
		co_spawn_detached([]() -> awaitable<void>
		{
			// TODO ...
		});
	}

public:
	tcp_server_ptr m_tcp_server;
	lock_free_queue<request_ptr> m_request_queue;
};

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(size_t tcount) :
	m_impl(new impl(tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution_context Context>
basic_server<CharT,Exec>::basic_server(Context &context, size_t tcount) :
	m_impl(new impl(context, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution Exec0>
basic_server<CharT,Exec>::basic_server(asio_acceptor &&acceptor, size_t tcount) :
	m_impl(new impl(std::move(acceptor), tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(const executor_type &exec, size_t tcount) :
	m_impl(new impl(exec, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(tcp_server_ptr tcp_server) :
	m_impl(new impl(std::move(tcp_server)))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::~basic_server()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::bind(io::ip_endpoint ep, bind_token tk)
{
	m_impl->m_tcp_server.bind(std::move(ep), std::move(tk));
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::async_accept(opt_token<callback_t<request_ptr,response_ptr,error_code>> tk) noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<bool> basic_server<CharT,Exec>::co_accept(request_ptr &request, response_ptr &response, opt_token<error_code&> tk)
{
	auto co_tcp_server_accept = [&]() -> awaitable<bool>
	{
		auto socket_ptr = co_await m_impl->m_tcp_server.co_accept();



		co_return true;
	};
	co_await (co_tcp_server_accept() or tk.rtime);
	co_return true;
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server<CharT,Exec>::accept(request_ptr &request, response_ptr &response, opt_token<error_code&> tk)
{

}



template <concept_char_type CharT, concept_execution Exec>
awaitable<void> basic_server<CharT,Exec>::co_cancel() noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
const io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object() const
{

}

template <concept_char_type CharT, concept_execution Exec>
io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object()
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::cancel() noexcept
{
	m_impl->m_tcp_server->cancel();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_SERVER_H

