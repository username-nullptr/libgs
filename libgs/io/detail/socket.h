
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
template <concept_execution Exec0>
basic_socket<Derived,Exec>::basic_socket(basic_socket<Derived,Exec0> &&other) noexcept :
	base_t(std::move(other))
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
basic_socket<Derived,Exec> &basic_socket<Derived,Exec>::operator=(basic_socket<Derived,Exec0> &&other) noexcept
{
	base_t::operator=(std::move(other));
	return *this;
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::connect(host_endpoint ep, opt_token<callback_t<error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await connect(ep, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await connect(ep, {tk.rtime, error});

		if( not *valid )
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
		co_await connect({std::move(addr), ep.port}, tk);
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
	co_spawn_detached([this, valid = this->m_valid, ep = std::move(ep), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await connect(ep, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await connect(ep, {tk.rtime, error});

		if( not *valid )
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
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::bind(ip_endpoint ep, no_time_token tk)
{
	error_code error;
	auto &native = this->derived().native();

	auto check_error = [&]() -> derived_t&
	{
		detail::check_error(tk.error, error, "libgs::io::basic_socket::bind");
		return this->derived();
	};
	if( not native.is_open() )
	{
		using protocol_t = typename derived_t::native_t::protocol_type;
		if( ep.addr.is_v4())
			native.open(protocol_t::v4(), error);
		else
			native.open(protocol_t::v6(), error);

		if( error )
			return check_error();
	}
	native.set_option(asio::socket_base::reuse_address(true), error);
	if( error )
		return check_error();

	native.bind({std::move(ep.addr), ep.port}, error);
	return check_error();
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::dns(string_wrapper domain, opt_token<callback_t<address_vector,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, domain = std::move(domain), tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		address_vector vector;
		error_code error;

		if( tk.cnl_sig )
			vector = co_await dns(domain, {tk.rtime, *tk.cnl_sig, error});
		else
			vector = co_await dns(domain, {tk.rtime, error});

		if( not *valid )
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
ip_endpoint basic_socket<Derived,Exec>::remote_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = this->derived().native().remote_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::socket::remote_endpoint");
	return {ep.address(), ep.port()};
}

template <typename Derived, concept_execution Exec>
ip_endpoint basic_socket<Derived,Exec>::local_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = this->derived().native().local_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::socket::local_endpoint");
	return {ep.address(), ep.port()};
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::set_option
(const concept_socket_option auto &op, no_time_token tk)
{
	using namespace asio;
	using option_t = std::decay_t<decltype(op)>;

	auto &_derived = this->derived();
	error_code error;

	if constexpr( std::is_same_v<option_t, socket_base::broadcast> )
		_derived.native().set_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::debug> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::do_not_route> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::enable_connection_aborted> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::keep_alive> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::linger> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::receive_buffer_size> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::receive_low_watermark> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::reuse_address> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::send_buffer_size> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, socket_base::send_low_watermark> )
		_derived.native().set_option(op, error);
	
	else if constexpr( std::is_same_v<option_t, ip::v6_only> )
		_derived.native().set_option(op, error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
	detail::check_error(tk.error, error, "libgs::io::socket::set_option");
	return _derived;
}

template <typename Derived, concept_execution Exec>
typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::get_option
(concept_socket_option auto &op, no_time_token tk)
{
	using namespace asio;
	using option_t = std::decay_t<decltype(op)>;

	auto &_derived = this->derived();
	error_code error;

	if constexpr( std::is_same_v<option_t, socket_base::broadcast> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::debug> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::do_not_route> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::enable_connection_aborted> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::keep_alive> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::linger> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::receive_buffer_size> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::receive_low_watermark> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::reuse_address> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::send_buffer_size> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, socket_base::send_low_watermark> )
		_derived.native().get_option(op, error);

	else if constexpr( std::is_same_v<option_t, ip::v6_only> )
		_derived.native().get_option(op, error);

	else error = std::make_error_code(static_cast<std::errc>(errc::invalid_argument));
	detail::check_error(tk.error, error, "libgs::io::socket::get_option");
	return _derived;
}

template <typename Derived, concept_execution Exec>
const typename basic_socket<Derived,Exec>::derived_t &basic_socket<Derived,Exec>::get_option
(concept_socket_option auto &op, no_time_token tk) const
{
	return remove_const(this)->get_option(op,tk);
}

template <typename Derived, concept_execution Exec>
size_t basic_socket<Derived,Exec>::read_buffer_size() const noexcept
{
	asio::socket_base::receive_buffer_size op;
	error_code error;
	this->derived().get_option(op, error);
	return op.value();
}

template <typename Derived, concept_execution Exec>
size_t basic_socket<Derived,Exec>::write_buffer_size() const noexcept
{
	asio::socket_base::send_buffer_size op;
	error_code error;
	this->derived().get_option(op, error);
	return op.value();
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
