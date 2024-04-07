
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

	using start_token = typename tcp_server_type::start_token;
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
	void bind(io::ip_endpoint ep, opt_token<error_code&> tk = {});
	void start(start_token tk = {});

public:
	void async_accept(opt_token<callback_t<request_ptr,error_code>> tk) noexcept;
	void async_accept(opt_token<callback_t<request_ptr>> tk) noexcept;

	[[nodiscard]] awaitable<request_ptr> co_accept(opt_token<error_code&> tk = {});
	request_ptr accept(opt_token<error_code&> tk = {});

public:
	awaitable<void> co_cancel() noexcept;
	void cancel() noexcept override;

	void set_non_block(bool flag, error_code &error) noexcept override;
	[[nodiscard]] bool is_non_block() const noexcept override;

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
	using request_queue = lock_free_queue<request_ptr>;
	using callback_queue = lock_free_queue<callback_t<request_ptr,error_code>>;

public:
	template <typename...Args>
	explicit impl(Args&&...args) requires requires { tcp_server_type(std::forward<Args>(args)...); } :
		impl(std::make_shared<tcp_handle_type>(std::forward<Args>(args)...)) {}

	explicit impl(tcp_server_ptr &&tcp_server) :
		m_tcp_server(std::move(tcp_server)) {}

public:
	void async_start(size_t max) noexcept
	{
		if( m_core_run )
			return ;
		m_core_run = true;

		co_spawn_detached([this, max]() -> awaitable<void>
		{
			try { co_await do_tcp_accept(max); }
			catch(...) {}

			co_await m_tcp_server->co_cancel();
			m_core_run = false;
			co_return ;
		});
	}

	request_ptr do_one_blocking(error_code &error) noexcept
	{
		auto socket = m_tcp_server->accept(error);
		if( error )
			return {};
		return do_one_tcp_service(std::move(socket), error);
	}

private:
	awaitable<void> do_tcp_accept(size_t max)
	{
		m_tcp_server->start(max);
		for(;;)
		{
			auto socket = co_await m_tcp_server->co_accept();
			libgs::co_spawn_detached([this, socket = std::move(socket)]() -> awaitable<void>
			{
				try { co_await do_tcp_service(std::move(socket)); }
				catch(...) {}
			},
			m_tcp_server.pool());
		}
		co_return ;
	}

	awaitable<void> do_tcp_service(io::tcp_socket_ptr socket)
	{
		libgs::http::request_parser parser;
		error_code error;

		char buf[65536] = {0};
		for(;;)
		{
			auto size = co_await socket->co_read({buf, 65536}, m_keepalive_timeout);
			if( size == 0 )
				break;

			bool parse_res = parser.append({buf, size}, error);
			if( error )
			{
				while( auto callback_opt = m_callback_queue.dequeue() )
					(*callback_opt)(nullptr, error);
				break;
			}
			else if( not parse_res )
				continue;

			m_request_queue.emplace(socket, parser);
			auto request_opt = m_request_queue.dequeue();

			while( auto callback_opt = m_callback_queue.dequeue() )
				(*callback_opt)(*request_opt, error_code());
		}
		co_return ;
	}

private:
	request_ptr do_one_tcp_service(io::tcp_socket_ptr socket, error_code &error)
	{
		libgs::http::request_parser parser;
		char buf[65536] = {0};
		for(;;)
		{
			auto size = socket->read({buf, 65536}, error);
			if( size == 0 )
				break;

			bool parse_res = parser.append({buf, size}, error);
			if( error )
			{
				while( auto callback_opt = m_callback_queue.dequeue() )
					(*callback_opt)(nullptr, error);
				break;
			}
			else if( not parse_res )
				continue;

			m_request_queue.emplace(socket, parser);
			return *m_request_queue.dequeue();
		}
		return {};
	}

public:
	tcp_server_ptr m_tcp_server;
	request_queue m_request_queue;

	callback_queue m_callback_queue;

	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_core_run {false};
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
void basic_server<CharT,Exec>::bind(io::ip_endpoint ep, opt_token<error_code&> tk)
{
	m_impl->m_tcp_server.bind(std::move(ep), tk);
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::start(start_token tk)
{
	m_impl->async_start(tk.max);
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::async_accept(opt_token<callback_t<request_ptr,error_code>> tk) noexcept
{
	auto request_opt = m_impl->m_request_queue.dequeue();
	if( not request_opt )
	{
		m_impl->m_callback_queue.emplace(std::move(tk.callback));
		return ;
	}
	auto callback = std::move(tk.callback);
	asio::post([callback = std::move(tk.callback)]{
		callback(*request_opt, error_code());
	});
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::async_accept(opt_token<callback_t<request_ptr>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_accept([callback = std::move(tk.callback)](request_ptr request, const error_code&){
		callback(std::move(request));
	});
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<basic_server_request_ptr<CharT,Exec>> basic_server<CharT,Exec>::co_accept(opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_request_ptr<CharT,Exec> basic_server<CharT,Exec>::accept(opt_token<error_code&> tk)
{
	error_code error;
	auto request = m_impl->do_one_blocking(error);
	if( not error )
		return request;

	if( tk.error )
		*tk.error = error;
	else
		throw system_error(error, "libgs::http::server::accept");
	return request;
}







template <concept_char_type CharT, concept_execution Exec>
awaitable<void> basic_server<CharT,Exec>::co_cancel() noexcept
{
	// TODO ...
	co_await m_impl->m_tcp_server->co_cancel();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::cancel() noexcept
{
	// TODO ...
	m_impl->m_tcp_server->cancel();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::set_non_block(bool flag, error_code &error) noexcept
{
	if( flag )
	{
		if( m_impl->m_core_run )
			error = error_code();
		else
			m_impl->m_tcp_server->set_non_block(flag, error);
	}
	else if( m_impl->m_core_run )
		error = std::make_error_code(std::errc::device_or_resource_busy);
	else
		m_impl->m_tcp_server->set_non_block(flag, error);
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server<CharT,Exec>::is_non_block() const noexcept
{
	return m_impl->m_tcp_server->is_non_block();
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


#endif //LIBGS_HTTP_SERVER_SERVER_H

