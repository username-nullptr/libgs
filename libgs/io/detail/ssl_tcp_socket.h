
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

#ifndef LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H
#define LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H

namespace libgs::io
{

template <concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(Context &context) :
	base_t(new native_t(context), context.get_executor())
{

}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(const executor_t &exec) :
	base_t(new native_t(exec), exec)
{

}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(native_t &&native) :
	base_t(reinterpret_cast<void*>(1), native.get_executor()),
{
	this->m_native = new native_t(std::move(native));
}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived>::basic_ssl_tcp_socket(basic_ssl_tcp_socket &&other) noexcept :
	base_t(std::move(other)),
{
	other.m_native = new native_t(this->executor());
}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived> &basic_ssl_tcp_socket<Exec,Derived>::operator=(basic_ssl_tcp_socket &&other) noexcept
{
	base_t::operator=(std::move(other));
	other.m_native = new native_t(this->executor());
	return *this;
}

template <concept_execution Exec, typename Derived>
basic_ssl_tcp_socket<Exec,Derived> &basic_ssl_tcp_socket<Exec,Derived>::operator=(native_t &&native) noexcept
{
	this->native() = std::move(native);
	return *this;
}

template <concept_execution Exec, typename Derived>
bool basic_ssl_tcp_socket<Exec,Derived>::is_open() const noexcept
{
	return native().next_layer().is_open();
}

template <concept_execution Exec, typename Derived>
awaitable<void> basic_ssl_tcp_socket<Exec,Derived>::close(opt_token<error_code&> tk)
{
	error_code error;
	if( tk.cnl_sig )
		co_await native().wave({tk.rtime, *tk.cnl_sig, error});
	else
		co_await native().wave({tk.rtime, error});

	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::close");
	if( not error )
	{
		native().next_layer().close(error);
		detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::close");
	}
	co_return ;
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::close(opt_token<callback_t<error_code>> tk)
{
	co_spawn_detached([this, valid = this->m_valid, buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		error_code error;
		if( tk.cnl_sig )
			co_await close(buf, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await close(buf, {tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t &basic_ssl_tcp_socket<Exec,Derived>::cancel() noexcept
{
	native().close(error);
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_ssl_tcp_socket<Exec,Derived>::remote_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = native().next_layer().remote_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::remote_endpoint");
	return ep;
}

template <concept_execution Exec, typename Derived>
ip_endpoint basic_ssl_tcp_socket<Exec,Derived>::local_endpoint(no_time_token tk) const
{
	error_code error;
	auto ep = native().next_layer().local_endpoint(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::local_endpoint");
	return ep;
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::set_option(const socket_option &op, no_time_token tk)
{
	error_code error;
	native().next_layer().set_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::set_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::get_option(socket_option op, no_time_token tk)
{
	error_code error;
	native().next_layer().get_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::derived_t&
basic_ssl_tcp_socket<Exec,Derived>::get_option(socket_option op, no_time_token tk) const
{
	error_code error;
	native().next_layer().get_option(error);
	detail::check_error(tk.error, error, "libgs::io::ssl_tcp_socket::get_option");
	return this->derived();
}

template <concept_execution Exec, typename Derived>
size_t basic_ssl_tcp_socket<Exec,Derived>::read_buffer_size() const noexcept
{
	return native().next_layer().read_buffer_size(error);
}

template <concept_execution Exec, typename Derived>
size_t basic_ssl_tcp_socket<Exec,Derived>::write_buffer_size() const noexcept
{
	return native().next_layer().read_buffer_size(error);
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::native_t &basic_ssl_tcp_socket<Exec,Derived>::native() const noexcept
{
	return *reinterpret_cast<const native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::native_t &basic_ssl_tcp_socket<Exec,Derived>::native() noexcept
{
	return *reinterpret_cast<const native_t*>(this->m_native);
}

template <concept_execution Exec, typename Derived>
const typename basic_ssl_tcp_socket<Exec,Derived>::resolver_t &basic_ssl_tcp_socket<Exec,Derived>::resolver() const noexcept
{
	return native().next_layer().resolver();
}

template <concept_execution Exec, typename Derived>
typename basic_ssl_tcp_socket<Exec,Derived>::resolver_t &basic_ssl_tcp_socket<Exec,Derived>::resolver() noexcept
{
	return native().next_layer().resolver();
}

template <concept_execution Exec, typename Derived>
void basic_ssl_tcp_socket<Exec,Derived>::_delete_native(void *native)
{
	delete reinterpret_cast<native_t*>(native);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_SSL_TCP_SOCKET_H
