
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

#ifndef LIBGS_IO_DETAIL_SSL_STREAM_H
#define LIBGS_IO_DETAIL_SSL_STREAM_H

namespace libgs::io
{

template <typename Stream, typename Derived>
template <concept_execution_context Context>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(Context &context, ssl::context &ssl) :
	base_type(new native_t(native_next_layer_t(context), ssl), context.get_executor()),
	m_next_layer(context)
{
	m_next_layer.external_reference(&native().next_layer());
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(next_layer_t &&next_layer, ssl::context &ssl) :
	base_type(new native_t(std::move(next_layer.native()), ssl), next_layer.executor()),
	m_next_layer(std::move(next_layer))
{
	m_next_layer.external_reference(&native().next_layer());
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(const executor_t &exec, ssl::context &ssl) :
	base_type(new native_t(native_next_layer_t(exec), ssl), exec),
	m_next_layer(exec)
{
	m_next_layer.external_reference(&native().next_layer());
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(ssl::context &ssl) :
	base_type(new native_t(native_next_layer_t(execution::get_executor()), ssl), execution::get_executor()),
	m_next_layer(execution::get_executor())
{
	m_next_layer.external_reference(&native().next_layer());
}

template <typename Stream, typename Derived>
template <typename NativeNextLayer>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(NativeNextLayer &&native, ssl::context &ssl)
requires requires { native_next_layer_t(std::move(native)); } :
	base_type(new native_t(native_next_layer_t(std::move(native)), ssl), native.get_executor()),
	m_next_layer(this->native().get_executor())
{
	m_next_layer.external_reference(&this->native().next_layer());
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived>::basic_ssl_stream(native_t &&native) :
	base_type(new native_t(std::move(native)), native.get_executor()),
	m_next_layer(this->native().get_executor())
{
	m_next_layer.external_reference(&this->native().next_layer());
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived>::~basic_ssl_stream()
{
	error_code error;
	close(error);
}

template <typename Stream, typename Derived>
template <typename NativeNextLayer>
basic_ssl_stream<Stream,Derived> &basic_ssl_stream<Stream,Derived>::operator=(NativeNextLayer &&native)
requires requires { (typename Stream::native_t)(std::move(native)); }
{
	this->m_native.next_layer() = std::move(native);
}

template <typename Stream, typename Derived>
basic_ssl_stream<Stream,Derived> &basic_ssl_stream<Stream,Derived>::operator=(native_t &&native)
{
	this->m_native = std::move(native);
	return this->derived();
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::handshake
(handshake_type type, opt_token<callback_t<error_code>> tk)
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), type, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await handshake(type, {tk.rtime, *tk.cnl_sig, error});
		else
			co_await handshake(type, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <typename Stream, typename Derived>
awaitable<void> basic_ssl_stream<Stream,Derived>::handshake(handshake_type type, opt_token<error_code&> tk)
{
	error_code error;
	auto _handshake = [&]() -> awaitable<void>
	{
		m_handshake_cancel = false;
		if( tk.cnl_sig )
			co_await native().async_handshake(type, asio::bind_cancellation_slot(tk.cnl_sig->slot(), use_awaitable_e[error]));
		else
			co_await native().async_handshake(type, use_awaitable_e[error]);

		if( m_handshake_cancel )
		{
			error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
			m_handshake_cancel = false;
		}
		co_return ;
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0s )
		co_await _handshake();
	else
	{
		auto var = co_await (_handshake() or co_sleep_for(tk.rtime));
		if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::ssl_stream::handshake");
	co_return ;
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::close(no_time_token tk)
{
	next_layer().close(tk);
	return this->derived();
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::cancel() noexcept
{
	next_layer().cancel();
	return this->derived();
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::set_verify_mode(ssl::verify_mode mode, no_time_token tk)
{
	error_code error;
	native().set_verify_mode(mode, error);
	detail::check_error(tk.error, error, "libgs::io::ssl_stream::set_verify_mode");
	return this->derived();
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::set_verify_depth(int depth, no_time_token tk)
{
	error_code error;
	native().set_verify_depth(depth, error);
	detail::check_error(tk.error, error, "libgs::io::ssl_stream::set_verify_depth");
	return this->derived();
}

template <typename Stream, typename Derived>
template <ssl::concept_verify_callback Func>
typename basic_ssl_stream<Stream,Derived>::derived_t &basic_ssl_stream<Stream,Derived>::set_verify_callback(Func &&func, no_time_token tk)
{
	error_code error;
	native().set_verify_callback(std::forward<Func>(func), error);
	detail::check_error(tk.error, error, "libgs::io::ssl_stream::set_verify_callback");
	return this->derived();
}

template <typename Stream, typename Derived>
size_t basic_ssl_stream<Stream,Derived>::read_buffer_size() const noexcept
{
	return next_layer().read_buffer_size();
}

template <typename Stream, typename Derived>
size_t basic_ssl_stream<Stream,Derived>::write_buffer_size() const noexcept
{
	return next_layer().write_buffer_size();
}

template <typename Stream, typename Derived>
const typename basic_ssl_stream<Stream,Derived>::native_t &basic_ssl_stream<Stream,Derived>::native() const noexcept
{
	return reinterpret_cast<const native_t&>(*this->m_native);
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::native_t &basic_ssl_stream<Stream,Derived>::native() noexcept
{
	return reinterpret_cast<native_t&>(*this->m_native);
}

template <typename Stream, typename Derived>
const typename basic_ssl_stream<Stream,Derived>::next_layer_t &basic_ssl_stream<Stream,Derived>::next_layer() const noexcept
{
	return m_next_layer;
}

template <typename Stream, typename Derived>
typename basic_ssl_stream<Stream,Derived>::next_layer_t &basic_ssl_stream<Stream,Derived>::next_layer() noexcept
{
	return m_next_layer;
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_SSL_STREAM_H
