
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

#ifndef LIBGS_IO_DETAIL_SOCKET_H
#define LIBGS_IO_DETAIL_SOCKET_H

namespace libgs::io
{

template <concept_execution Exec>
void basic_socket<Exec>::connect(host_endpoint ep, opt_cb_token<error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		co_await connect(std::move(ep), {tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(host_endpoint ep, opt_token<ua_redirect_error_t> tk) noexcept
{
	auto addr = ip_address::from_string(ep.host, tk.uare.ec_);
	if( not tk.uare.ec_ )
		co_return co_await connect({addr, ep.port}, std::move(tk));

	auto no_time_out = [&]() -> awaitable<error_code>
	{
		error_code error;
		auto addrs = co_await dns(std::move(ep.host), use_awaitable_e[error]);

		if( error)
			co_return error;

		for(auto &addr : addrs)
		{
			co_await connect({std::move(addr), ep.port}, use_awaitable_e[error]);
			if( not error or error.value() == asio::error::operation_aborted )
				break ;
		}
		co_return error;
	};

	using namespace std::chrono;
	if( tk.rtime == 0s )
	{
		tk.uare.ec_ = co_await no_time_out();
		co_return ;
	}
	auto var = co_await (no_time_out() or co_sleep_for(tk.rtime));
	if( var.index() == 0 )
		tk.uare.ec_ = std::get<0>(var);
	else
		tk.uare.ec_ = std::make_error_code(std::errc::timed_out);
	co_return ;
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(host_endpoint ep, opt_token<use_awaitable_t&> tk)
{
	error_code error;
	co_await connect(std::move(ep), {tk.rtime, use_awaitable_e[error]});

	if( error )
		throw system_error(error, "libgs::io::socket::connect");
	co_return ;
}

template <concept_execution Exec>
void basic_socket<Exec>::connect(host_endpoint ep, error_code &error) noexcept
{
	auto addrs = dns(std::move(ep.host), error);
	if( error )
		return ;

	for(auto &addr : addrs)
	{
		connect({addr, ep.port}, error);
		if( not error )
			break;
	}
}

template <concept_execution Exec>
void basic_socket<Exec>::connect(host_endpoint ep)
{
	error_code error;
	connect(std::move(ep), error);

	if( error )
		throw system_error(error, "libgs::io::socket::connect");
}

template <concept_execution Exec>
void basic_socket<Exec>::connect(ip_endpoint ep, opt_cb_token<error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		co_await connect(std::move(ep), {tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(ip_endpoint ep, opt_token<ua_redirect_error_t> tk) noexcept
{
	using namespace std::chrono;
	if( tk.rtime == 0s )
	{
		tk.uare.ec_ = co_await do_connect(std::move(ep));
		co_return ;
	}
	auto var = co_await (do_connect(std::move(ep)) or co_sleep_for(tk.rtime));
	if( var.index() == 0 )
		tk.uare.ec_ = std::get<0>(var);
	else if( var.index() == 1 )
		tk.uare.ec_ = std::make_error_code(std::errc::timed_out);
	co_return ;
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(ip_endpoint ep, opt_token<use_awaitable_t&> tk)
{
	error_code error;
	co_await connect(std::move(ep), tk.rtime, use_awaitable_e[error]);

	if( error )
		throw system_error(error, "libgs::io::socket::connect");
	co_return ;
}

template <concept_execution Exec>
void basic_socket<Exec>::connect(ip_endpoint ep)
{
	error_code error;
	connect(std::move(ep), error);

	if( error )
		throw system_error(error, "libgs::io::socket::connect");
}

template <concept_execution Exec>
ip_endpoint basic_socket<Exec>::remote_endpoint() const
{
	error_code error;
	auto ep = remote_endpoint(error);

	if( error )
		throw system_error(error, "libgs::io::socket::remote_endpoint");
	return ep;
}

template <concept_execution Exec>
ip_endpoint basic_socket<Exec>::local_endpoint() const
{
	error_code error;
	auto ep = local_endpoint(error);

	if( error )
		throw system_error(error, "libgs::io::socket::local_endpoint");
	return ep;
}

template <concept_execution Exec>
void basic_socket<Exec>::shutdown(shutdown_type what)
{
	error_code error;
	shutdown(error, what);

	if( error )
		throw system_error(error, "libgs::io::socket::shutdown");
}

template <concept_execution Exec>
void basic_socket<Exec>::close(error_code &error, bool _shutdown) noexcept
{
	if( _shutdown )
	{
		shutdown(error, shutdown_type::shutdown_both);
		if( error )
			return ;
	}
	close(error);
}

template <concept_execution Exec>
void basic_socket<Exec>::close(bool _shutdown)
{
	error_code error;
	close(error, _shutdown);

	if( error )
		throw system_error(error, "libgs::io::socket::close");
}

template <concept_execution Exec>
void basic_socket<Exec>::set_option(const socket_option &op)
{
	error_code error;
	set_option(op, error);

	if( error )
		throw system_error(error, "libgs::io::socket::set_option");
}

template <concept_execution Exec>
void basic_socket<Exec>::get_option(socket_option op) const
{
	error_code error;
	get_option(std::move(op), error);

	if( error )
		throw system_error(error, "libgs::io::socket::set_option");
}

template <concept_execution Exec>
void basic_socket<Exec>::dns(string_wrapper domain, opt_cb_token<address_vector,error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), domain = std::move(domain), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		auto vector = co_await dns(std::move(domain), {tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(std::move(vector), error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<typename basic_socket<Exec>::address_vector> basic_socket<Exec>::dns
(string_wrapper domain, opt_token<ua_redirect_error_t> tk) noexcept
{
	using namespace std::chrono;
	if( tk.rtime == 0s )
		co_return co_await do_dns(domain, tk.uare.ec_);
	
	auto var = co_await (do_dns(domain, tk.uare.ec_) or co_sleep_for(tk.rtime));
	address_vector vector;

	if( var.index() == 0 )
		vector = std::move(std::get<0>(var));
	else
		tk.uare.ec_ = std::make_error_code(std::errc::timed_out);
	co_return vector;
}

template <concept_execution Exec>
awaitable<typename basic_socket<Exec>::address_vector> basic_socket<Exec>::dns
(string_wrapper domain, opt_token<use_awaitable_t&> tk)
{
	error_code error;
	auto res = co_await dns(std::move(domain), {tk.rtime, use_awaitable_e[error]});

	if( error )
		throw system_error(error, "libgs::io::socket::dns");
	co_return res;
}

template <concept_execution Exec>
basic_socket<Exec>::address_vector basic_socket<Exec>::dns(string_wrapper domain)
{
	error_code error;
	auto res = dns(std::move(domain), error);

	if( error )
		throw system_error(error, "libgs::io::socket::dns");
	return res;
}

template <concept_execution Exec>
size_t basic_socket<Exec>::read_buffer_size() const noexcept
{
	asio::socket_base::receive_buffer_size op;
	error_code error;
	get_option(op, error);
	return op.value();
}

template <concept_execution Exec>
size_t basic_socket<Exec>::write_buffer_size() const noexcept
{
	asio::socket_base::send_buffer_size op;
	error_code error;
	get_option(op, error);
	return op.value();
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_SOCKET_H
