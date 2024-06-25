
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

#ifndef LIBGS_IO_DETAIL_TCP_SERVER_BASE_H
#define LIBGS_IO_DETAIL_TCP_SERVER_BASE_H

#include <libgs/io/tcp_socket.h>

namespace libgs
{

template <concept_execution Exec = asio::any_io_executor>
using asio_basic_tcp_acceptor = asio::basic_socket_acceptor<asio::ip::tcp, Exec>;

using asio_tcp_acceptor = asio_basic_tcp_acceptor<>;

namespace io
{

template <typename Derived, concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI tcp_server_base : public device_base<crtp_derived_t<Derived,tcp_server_base<Derived,Exec>>,Exec>
{
	LIBGS_DISABLE_COPY(tcp_server_base)

public:
	using derived_t = crtp_derived_t<Derived,tcp_server_base>;
	using base_t = device_base<derived_t,Exec>;

	using executor_t = base_t::executor_t;
	using socket_base_t = tcp_socket;
	using native_t = asio_basic_tcp_acceptor<executor_t>;

	struct start_token : no_time_token
	{
		size_t max = asio::socket_base::max_listen_connections;
		using no_time_token::no_time_token;
		start_token(size_t max, error_code &error);
		start_token(size_t max);
	};

public:
	explicit tcp_server_base(size_t tcount = 0);

	template <concept_execution_context Context>
	explicit tcp_server_base(Context &context, size_t tcount = 0);

	template <concept_execution Exec0>
	explicit tcp_server_base(asio_basic_tcp_acceptor<Exec0> &&native, size_t tcount = 0);

	explicit tcp_server_base(const executor_t &exec, size_t tcount = 0);
	virtual ~tcp_server_base() = 0;

public:
	template <concept_execution Exec0>
	tcp_server_base(tcp_server_base<Derived,Exec0> &&other) noexcept;

	template <concept_execution Exec0>
	tcp_server_base &operator=(tcp_server_base<Derived,Exec0> &&other) noexcept;

	template <concept_execution Exec0>
	tcp_server_base &operator=(asio_basic_tcp_acceptor<Exec0> &&native) noexcept;

public:
	derived_t &bind(ip_endpoint ep, no_time_token tk = {});
	derived_t &start(start_token tk = {});

public:
	derived_t &set_option(const auto &op, no_time_token tk = {});
	derived_t &get_option(auto &op, no_time_token tk = {});
	const derived_t &get_option(auto &op, no_time_token tk = {}) const;

public:
	[[nodiscard]] awaitable<void> co_wait() noexcept;
	derived_t &wait() noexcept;

	[[nodiscard]] awaitable<void> co_stop() noexcept;
	derived_t &stop() noexcept;
	derived_t &cancel() noexcept;

	[[nodiscard]] const asio::thread_pool &pool() const noexcept;
	[[nodiscard]] asio::thread_pool &pool() noexcept;

public:
	const native_t &native() const noexcept;
	native_t &native() noexcept;

protected:
	derived_t &_accept(opt_token<callback_t<socket_base_t,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<socket_base_t> _accept(opt_token<error_code&> tk = {});
	static size_t _tcount(size_t c) noexcept;

	native_t m_native;
	asio::thread_pool m_pool;
};

template <typename Derived, concept_execution Exec>
tcp_server_base<Derived,Exec>::start_token::start_token(size_t max, error_code &error) :
	no_time_token(error), max(max)
{

}

template <typename Derived, concept_execution Exec>
tcp_server_base<Derived,Exec>::start_token::start_token(size_t max) :
	max(max)
{

}

template <typename Derived, concept_execution Exec>
tcp_server_base<Derived,Exec>::tcp_server_base(size_t tcount) :
	base_t(execution::io_context().get_executor()),
	m_native(execution::io_context()),
	m_pool(_tcount(tcount))
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution_context Context>
tcp_server_base<Derived,Exec>::tcp_server_base(Context &context, size_t tcount) :
	base_t(context.get_executor()),
	m_native(context),
	m_pool(_tcount(tcount))
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
tcp_server_base<Derived,Exec>::tcp_server_base(asio_basic_tcp_acceptor<Exec0> &&native, size_t tcount) :
	base_t(native.get_executor()),
	m_native(std::move(native)),
	m_pool(_tcount(tcount))
{

}

template <typename Derived, concept_execution Exec>
tcp_server_base<Derived,Exec>::tcp_server_base(const executor_t &exec, size_t tcount) :
	base_t(exec),
	m_native(exec),
	m_pool(_tcount(tcount))
{

}

template <typename Derived, concept_execution Exec>
tcp_server_base<Derived,Exec>::~tcp_server_base()
{
	stop();
}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
tcp_server_base<Derived,Exec>::tcp_server_base(tcp_server_base<Derived,Exec0> &&other) noexcept :
	base_t(std::move(other)),
	m_native(std::move(other.m_native)),
	m_pool(std::move(other.m_pool))
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
tcp_server_base<Derived,Exec> &tcp_server_base<Derived,Exec>::operator=(tcp_server_base<Derived,Exec0> &&other) noexcept
{
	base_t::operator=(std::move(other));
	m_native = std::move(other.m_native);
	m_pool = std::move(other.m_pool);
	return *this;
}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
tcp_server_base<Derived,Exec> &tcp_server_base<Derived,Exec>::operator=(asio_basic_tcp_acceptor<Exec0> &&native) noexcept
{
	m_native = std::move(native);
	return *this;
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::bind
(ip_endpoint ep, no_time_token tk)
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
	m_native.set_option(asio::socket_base::reuse_address(true), error);
	if( error )
		return check_error();

	m_native.bind({std::move(ep.addr), ep.port}, error);
	return check_error();
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::start(start_token tk)
{
	error_code error;
	m_native.listen(static_cast<int>(tk.max), error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::start");
	return this->derived();
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::set_option(const auto &op, no_time_token tk)
{
	error_code error;
	m_native.set_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::set_option");
	return this->derived();
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::get_option(auto &op, no_time_token tk)
{
	error_code error;
	m_native.get_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::get_option");
	return this->derived();
}

template <typename Derived, concept_execution Exec>
const typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::get_option
		(auto &op, no_time_token tk) const
{
	error_code error;
	m_native.get_option(op, error);
	detail::check_error(tk.error, error, "libgs::io::tcp_server::get_option");
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<void> tcp_server_base<Derived,Exec>::co_wait() noexcept
{
	co_return co_await co_thread([this]{wait();});
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::wait() noexcept
{
	m_pool.wait();
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<void> tcp_server_base<Derived,Exec>::co_stop() noexcept
{
	co_return co_await co_thread([this]{stop();});
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::stop() noexcept
{
	cancel();
	error_code error;
	m_native.close(error);
	return wait();
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::cancel() noexcept
{
	error_code error;
	m_native.cancel(error);
	return this->derived();
}

template <typename Derived, concept_execution Exec>
const asio::thread_pool &tcp_server_base<Derived,Exec>::pool() const noexcept
{
	return m_pool;
}

template <typename Derived, concept_execution Exec>
asio::thread_pool &tcp_server_base<Derived,Exec>::pool() noexcept
{
	return m_pool;
}

template <typename Derived, concept_execution Exec>
const typename tcp_server_base<Derived,Exec>::native_t &tcp_server_base<Derived,Exec>::native() const noexcept
{
	return m_native;
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::native_t &tcp_server_base<Derived,Exec>::native() noexcept
{
	return m_native;
}

template <typename Derived, concept_execution Exec>
typename tcp_server_base<Derived,Exec>::derived_t &tcp_server_base<Derived,Exec>::_accept
(opt_token<callback_t<socket_base_t,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		socket_base_t socket;
		error_code error;

		if( tk.cnl_sig )
			socket = co_await this->derived()._accept({tk.rtime, *tk.cnl_sig, error});
		else
			socket = co_await this->derived()._accept({tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(std::move(socket), error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<typename tcp_server_base<Derived,Exec>::socket_base_t>
tcp_server_base<Derived,Exec>::_accept(opt_token<error_code&> tk)
{
	error_code error;
	socket_base_t socket(pool());

	if( tk.cnl_sig )
	{
		socket = co_await m_native.async_accept
				(m_pool, asio::bind_cancellation_slot(tk.cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
		socket = co_await m_native.async_accept(m_pool, use_awaitable_e[error]);

	detail::check_error(tk.error, error, "libgs::io::tcp_server_base::_accept");
	co_return std::move(socket);
}

template <typename Derived, concept_execution Exec>
size_t tcp_server_base<Derived,Exec>::_tcount(size_t c) noexcept
{
	return c == 0 ? std::thread::hardware_concurrency() << 1 : c;
}

namespace detail
{

template <typename Derived, typename Exec>
class _tcp_server_base : public tcp_server_base<Derived,Exec> {
public: using tcp_server_base<Derived,Exec>::tcp_server_base;
};

template <typename Derived, typename Exec, typename...Args>
concept concept_tcp_server_base = concept_constructible<_tcp_server_base<Derived,Exec>,Args...>;

} //namespace detail

}} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SERVER_BASE_H
