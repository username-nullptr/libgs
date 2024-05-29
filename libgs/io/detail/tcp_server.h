
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
template <concept_execution Exec0>
basic_tcp_server<Exec,Derived>::basic_tcp_server(basic_tcp_server<Exec0,Derived> &&other) noexcept :
	base_t(std::move(other))
{

}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_server<Exec,Derived> &basic_tcp_server<Exec,Derived>::operator=(basic_tcp_server<Exec0,Derived> &&other) noexcept
{
	base_t::operator=(std::move(other));
	return *this;
}

template <concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_tcp_server<Exec,Derived> &basic_tcp_server<Exec,Derived>::operator=(asio_basic_tcp_acceptor<Exec0> &&native) noexcept
{
	base_t::operator=(std::move(native));
	return *this;
}

template <concept_execution Exec, typename Derived>
typename basic_tcp_server<Exec,Derived>::derived_t &basic_tcp_server<Exec,Derived>::accept
(opt_token<callback_t<socket_t,error_code>> tk) noexcept
{
	return this->_accept(tk);
}

template <concept_execution Exec, typename Derived>
awaitable<typename basic_tcp_server<Exec,Derived>::socket_t> 
basic_tcp_server<Exec,Derived>::accept(opt_token<error_code&> tk)
{
	co_return co_await this->_accept(tk);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_TCP_SERVER_H
