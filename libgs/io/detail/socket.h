
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
void basic_socket<Exec>::connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await connect(ep, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await connect(ep, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(host_endpoint ep, opt_token<error_code&> tk)
{
	error_code error;
	auto addr = ip_address::from_string(ep.host, error);

	if( not error )
		co_await connect({addr, ep.port}, std::move(tk));
	else
	{
		auto no_time_out = [&]() mutable -> awaitable<error_code>
		{
			auto addrs = co_await dns(std::move(ep.host), error);
			if( error)
				co_return error;

			for(auto &addr : addrs)
			{
				co_await connect({std::move(addr), ep.port}, error);
				if( not error or error.value() == asio::error::operation_aborted )
					break ;
			}
			co_return error;
		};
		using namespace std::chrono_literals;
		if( tk.rtime == 0s )
			error = co_await no_time_out();
		else
		{
			auto var = co_await (no_time_out() or co_sleep_for(tk.rtime));
			if( var.index() == 1 )
				error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
		}
	}
	detail::check_error(tk.error, error, "libgs::io::stream::connect");
	co_return ;
}

template <concept_execution Exec>
void basic_socket<Exec>::connect(ip_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await connect(ep, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await connect(ep, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<void> basic_socket<Exec>::connect(ip_endpoint ep, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	error_code error;

	if( tk.rtime == 0s )
		error = co_await do_connect(ep, tk.cnl_sig);
	else
	{
		auto var = co_await (do_connect(ep, tk.cnl_sig) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			error = std::get<0>(var);
		else if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::connect");
	co_return ;
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
void basic_socket<Exec>::shutdown(error_code &error) noexcept
{
	shutdown(error, shutdown_type::shutdown_both);
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
	get_option(op, error);

	if( error )
		throw system_error(error, "libgs::io::socket::set_option");
}

template <concept_execution Exec>
void basic_socket<Exec>::dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), domain = std::move(domain), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		address_vector vector;
		error_code error;

		if( tk.cnl_sig )
			vector = co_await dns(domain, {tk.rtime, *tk.cnl_sig, error});
		else
			vector = co_await dns(domain, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(std::move(vector), error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
awaitable<typename basic_socket<Exec>::address_vector> basic_socket<Exec>::dns(string_wrapper domain, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	address_vector vector;
	error_code error;

	if( tk.rtime == 0s )
		co_await do_dns(domain, tk.cnl_sig, error);
	else
	{
		auto var = co_await (do_dns(domain, tk.cnl_sig, error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			vector = std::move(std::get<0>(var));
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::dns");
	co_return vector;
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
