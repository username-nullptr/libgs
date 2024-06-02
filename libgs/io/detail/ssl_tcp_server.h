
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

#ifndef LIBGS_IO_DETAIL_SSL_TCP_SERVER_H
#define LIBGS_IO_DETAIL_SSL_TCP_SERVER_H

#ifdef LIBGS_ENABLE_OPENSSL

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
template <typename...Args>
basic_ssl_tcp_server<Exec,Derived>::basic_ssl_tcp_server(ssl::context &ssl, Args&&...args)
	requires detail::concept_tcp_server_base<derived_t,executor_t,Args...> :
	base_t(std::forward<Args>(args)...), m_ssl(&ssl)
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_server<Exec,Derived>::basic_ssl_tcp_server(basic_ssl_tcp_server<Exec0,Derived> &&other) noexcept :
	base_t(std::move(other)), m_ssl(&other.m_ssl)
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_server<Exec,Derived> &basic_ssl_tcp_server<Exec,Derived>::operator=
(basic_ssl_tcp_server<Exec0,Derived> &&other) noexcept
{
	base_t::operator=(std::move(other));
	m_ssl = &other.m_ssl;
	return *this;
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_ssl_tcp_server<Exec,Derived> &basic_ssl_tcp_server<Exec,Derived>::operator=(asio_basic_tcp_acceptor<Exec0> &&native) noexcept
{
	base_t::operator=(std::move(native));
	return *this;
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_server<Exec,Derived>::derived_t &basic_ssl_tcp_server<Exec,Derived>::accept
(opt_token<callback_t<socket_t,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		socket_t socket;
		error_code error;

		if( tk.cnl_sig )
			socket = co_await accept({tk.rtime, *tk.cnl_sig, error});
		else
			socket = co_await accept({tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(std::move(socket), error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<typename basic_ssl_tcp_server<Exec,Derived>::socket_t>
basic_ssl_tcp_server<Exec,Derived>::accept(opt_token<error_code&> tk)
{
	error_code error;
	socket_t socket(this->pool(), *m_ssl, socket_t::handshake_t::server);

	auto _accept = [&]() mutable -> awaitable<void>
	{
		if( tk.cnl_sig )
			socket = ssl_stream<socket_base_t>(co_await this->_accept({*tk.cnl_sig, error}), *m_ssl);
		else
			socket = ssl_stream<socket_base_t>(co_await this->_accept(error), *m_ssl);

		if( error )
			co_return ;
		else if( tk.cnl_sig )
			co_await socket.ssl_stream().handshake(socket_t::handshake_t::server, {*tk.cnl_sig, error});
		else
			co_await socket.ssl_stream().handshake(socket_t::handshake_t::server, error);
		co_return ;
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await _accept();
	else
	{
		auto var = co_await (_accept() or co_sleep_for(tk.rtime));
		if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_server::accept");
	co_return std::move(socket);
}

} //namespace libgs::io

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_IO_DETAIL_SSL_TCP_SERVER_H
