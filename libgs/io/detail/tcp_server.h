
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

#ifndef LIBGS_IO_DETAIL_TCP_SERVER_H
#define LIBGS_IO_DETAIL_TCP_SERVER_H

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived>::start_token::start_token(size_t max, error_code &error) :
	no_time_token(error), max(max)
{

}

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived>::start_token::start_token(size_t max) :
	max(max)
{

}

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived>::basic_tcp_server(size_t tcount) :
	base_t(execution::io_context().get_executor()),
	m_native(execution::io_context()),
	m_pool(_tcount(tcount))
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_tcp_server<Exec,Derived>::basic_tcp_server(Context &context, size_t tcount) :
	base_t(context.get_executor()),
	m_native(context),
	m_pool(_tcount(tcount))
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_server<Exec,Derived>::basic_tcp_server(asio_basic_tcp_acceptor<Exec0> &&native, size_t tcount) :
	base_t(native.get_executor()),
	m_native(std::move(native)),
	m_pool(_tcount(tcount))
{

}

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived>::basic_tcp_server(const executor_t &exec, size_t tcount) :
	base_t(exec),
	m_native(exec),
	m_pool(_tcount(tcount))
{

}

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived>::~basic_tcp_server()
{
	cancel();
}

template <concept_execution Exec, typename Derived>
basic_tcp_server<Exec,Derived> &basic_tcp_server<Exec,Derived>::operator=(native_t &&native) noexcept
{
	m_native = std::move(native);
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::bind
(ip_endpoint ep, opt_token<error_code&> tk)
{
	error_code error;
	auto check_error = [&]() -> derived_t&
	{
		detail::check_error(tk.error, error, "libgs::io::tcp_server::bind");
		return this->derived();
	};
	if( not m_native.is_open() )
	{
		if( ep.addr.is_v4())
			m_native.open(asio::ip::tcp::v4(), error);
		else
			m_native.open(asio::ip::tcp::v6(), error);

		if( error )
			return check_error();
	}
	m_native.set_option(asio_tcp_acceptor::reuse_address(true), error);
	if( error )
		return check_error();
	
	m_native.bind({std::move(ep.addr), ep.port}, error);
	return check_error();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::start(start_token tk)
{
	error_code error;
	m_native.listen(static_cast<int>(tk.max), error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::start");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::accept
(opt_token<callback_t<socket_t,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, tk = std::move(tk)]() -> awaitable<void>
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
awaitable<typename basic_tcp_server<Exec,Derived>::socket_t> 
basic_tcp_server<Exec,Derived>::accept(opt_token<error_code&> tk)
{
	error_code error;
	socket_t socket(pool());

	if( tk.cnl_sig )
	{
		socket = co_await m_native.async_accept
			(m_pool, asio::bind_cancellation_slot(tk.cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
		socket = co_await m_native.async_accept(m_pool, use_awaitable_e[error]);

	detail::check_error(tk.error, error, "libgs::io::tcp_server::accept");
	co_return std::move(socket);
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::set_option(const auto &op, no_time_token tk)
{
	error_code error;
	m_native.set_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::set_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::get_option(auto &op, no_time_token tk)
{
	error_code error;
	m_native.get_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
const typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::get_option
(auto &op, no_time_token tk) const
{
	error_code error;
	m_native.get_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<void> basic_tcp_server<Exec,Derived>::co_wait() noexcept
{
	return co_await co_thread([this]{wait();});
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::wait() noexcept
{
	m_pool.wait();
	return this->derived();
}

template <concept_execution Exec, typename Derived>
awaitable<void> basic_tcp_server<Exec,Derived>::co_stop() noexcept
{
	co_return co_await co_thread([this]{stop();});
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::stop() noexcept
{
	cancel();
	error_code error;
	m_native.close(error);
	return wait();
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::cancel() noexcept
{
	error_code error;
	m_native.cancel(error);
	return this->derived();
}

template <concept_execution Exec, typename Derived>
const asio::thread_pool &basic_tcp_server<Exec,Derived>::pool() const noexcept
{
	return m_pool;
}

template <concept_execution Exec, typename Derived>
asio::thread_pool &basic_tcp_server<Exec,Derived>::pool() noexcept
{
	return m_pool;
}

template <concept_execution Exec, typename Derived>
const typename basic_tcp_server<Exec,Derived>::native_t &basic_tcp_server<Exec,Derived>::native() const noexcept
{
	return m_native;
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::native_t &basic_tcp_server<Exec,Derived>::native() noexcept
{
	return m_native;
}

template <concept_execution Exec, typename Derived>
size_t basic_tcp_server<Exec,Derived>::_tcount(size_t c) noexcept
{
	return c == 0 ? std::thread::hardware_concurrency() << 1 : c;
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SERVER_H
