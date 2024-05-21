
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

template <typename Derived, concept_execution Exec>
basic_socket<Derived,Exec>::~basic_socket() = default;

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept
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
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<void> basic_socket<Derived,Exec>::connect(host_endpoint ep, opt_token<error_code&> tk)
{
	error_code error;
	auto addr = ip_address::from_string(ep.host, error);

	if( not error )
		co_await connect({addr, ep.port}, tk);
	else
	{
		auto _connect = [&]() mutable -> awaitable<error_code>
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
			error = co_await _connect();
		else
		{
			auto var = co_await (_connect() or co_sleep_for(tk.rtime));
			if( var.index() == 1 )
				error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
		}
	}
	detail::check_error(tk.error, error, "libgs::io::stream::connect");
	co_return ;
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::connect(ip_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept
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
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<void> basic_socket<Derived,Exec>::connect(ip_endpoint ep, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	error_code error;

	if( tk.rtime == 0s )
		error = co_await this->derived()._connect(ep, tk.cnl_sig);
	else
	{
		auto var = co_await (this->derived()._connect(ep, tk.cnl_sig) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			error = std::get<0>(var);
		else if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::connect");
	co_return ;
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept
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
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<typename basic_socket<Derived,Exec>::address_vector> basic_socket<Derived,Exec>::dns(string_wrapper domain, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	address_vector vector;
	error_code error;

	if( tk.rtime == 0s )
		vector = co_await this->derived()._dns(domain, tk.cnl_sig, error);
	else
	{
		auto var = co_await (this->derived()._dns(domain, tk.cnl_sig, error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			vector = std::move(std::get<0>(var));
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::dns");
	co_return vector;
}

template <typename Derived, concept_execution Exec>
awaitable<error_code> basic_socket<Derived,Exec>::_connect
(const ip_endpoint &ep, cancellation_signal *cnl_sig) noexcept
{
	error_code error;
	m_connect_cancel = false;

	if( cnl_sig )
	{
		co_await this->derived().native()
				.async_connect({ep.addr, ep.port}, asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		co_await this->derived().native()
				.async_connect({ep.addr, ep.port}, use_awaitable_e[error]);
	}
	if( m_connect_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_connect_cancel = false;
	}
	co_return error;
}

template <typename Derived, concept_execution Exec>
awaitable<typename basic_socket<Derived,Exec>::address_vector> basic_socket<Derived,Exec>::_dns
(const std::string &domain, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	using resolver_t = derived_t::resolver_t;
	typename resolver_t::results_type results;

	address_vector vector;
	m_dns_cancel = false;

	if( cnl_sig )
	{
		results = co_await this->derived().resolver().async_resolve
				(domain, "0", asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		results = co_await this->derived().resolver().async_resolve
				(domain, "0", use_awaitable_e[error]);
	}
	if( m_dns_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_dns_cancel = false;
		co_return vector;
	}
	else if( error )
		co_return vector;

	for(auto &ep : results)
		vector.emplace_back(ep.endpoint().address());
	co_return vector;
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_SOCKET_H
