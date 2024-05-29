
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

#ifndef LIBGS_IO_DETAIL_STREAM_H
#define LIBGS_IO_DETAIL_STREAM_H

#include <libgs/core/cxx/exception.h>

namespace libgs::io
{

template <typename Derived, concept_execution Exec>
basic_stream<Derived,Exec>::basic_stream(auto *native, const executor_t &exec) :
	base_t(exec), m_native(native)
{
	assert(m_native);
}

template <typename Derived, concept_execution Exec>
basic_stream<Derived,Exec>::~basic_stream()
{
	if( m_native )
		__delete_native();
}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
basic_stream<Derived,Exec>::basic_stream(basic_stream<Derived,Exec0> &&other) noexcept :
	base_t(std::move(other))
{

}

template <typename Derived, concept_execution Exec>
template <concept_execution Exec0>
basic_stream<Derived,Exec> &basic_stream<Derived,Exec>::operator=(basic_stream<Derived,Exec0> &&other) noexcept
{
	base_t::operator=(std::move(other));
	return *this;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		size_t size;
		error_code error;

		if( tk.cnl_sig )
			size = co_await read(buf, {tk.rc, tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await read(buf, {tk.rc, tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = this->derived().read_buffer_size();
	auto _buf = std::make_shared<char[]>(buf.size);

	auto _callback = [buf, _buf, callback = std::move(tk.callback)](size_t size, const error_code &error)
	{
		buf.data = std::string(_buf.get(), size);
		callback(size, error);
	};
	if( tk.cnl_sig )
		return read({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t, const error_code &error){
		callback(error);
	};
	if( tk.cnl_sig )
		return read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read
(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](const error_code&){
		callback();
	};
	if( tk.cnl_sig )
		return read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::read(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	size_t size = 0;
	error_code error;

	auto _read = [&]() -> awaitable<size_t>
	{
		do {
			auto res = co_await this->derived()._read_data
					({reinterpret_cast<char*>(buf.data) + size, buf.size - size}, std::move(tk.rc), tk.cnl_sig, error);
			if( error and error.value() != errc::try_again and error.value() != errc::interrupted )
				break;
			size += res;
		}
		while( size < buf.size );
		co_return size;
	};
	if( tk.rtime == 0s )
		co_await _read();
	else
	{
		auto var = co_await(_read() or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::read");
	co_return size;
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk)
{
	if( buf.size == 0 )
		buf.size = this->derived().read_buffer_size();

	auto _buf = std::make_shared<char[]>(buf.size);
	auto size = co_await read({_buf.get(), buf.size}, std::move(tk));

	buf.data = std::string(_buf.get(), size);
	co_return size;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	co_spawn_detached([this, valid = this->m_valid, buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not *valid )
			co_return ;

		size_t size;
		error_code error;

		if( tk.cnl_sig )
			size = co_await read_some(buf, {tk.rc, tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await read_some(buf, {tk.rc, tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return read_some(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read_some(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = this->derived().read_buffer_size();
	auto _buf = std::make_shared<char[]>(buf.size);

	auto _callback = [buf, _buf, callback = std::move(tk.callback)](size_t size, const error_code &error)
	{
		buf.data = std::string(_buf.get(), size);
		callback(size, error);
	};
	if( tk.cnl_sig )
		return read_some({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read_some({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return read_some(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read_some(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t, const error_code &error){
		callback(error);
	};
	if( tk.cnl_sig )
		return read_some(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read_some(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::read_some
(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](const error_code&){
		callback();
	};
	if( tk.cnl_sig )
		return read_some(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return read_some(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::read_some(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await this->derived()._read_data(buf, std::move(tk.rc), tk.cnl_sig, error);
	else
	{
		auto var = co_await(this->derived()._read_data(buf, std::move(tk.rc), tk.cnl_sig, error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::read");
	co_return size;
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::read_some(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk)
{
	if( buf.size == 0 )
		buf.size = this->derived().read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto size = co_await read_some({_buf.get(), buf.size}, std::move(tk));

	buf.data = std::string(_buf.get(), size);
	co_return size;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::write
(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	auto size = buf.size;
	auto _buf = std::make_shared<char[]>(size);
	memcpy(_buf.get(), buf.data.data(), size);

	co_spawn_detached([this, valid = this->m_valid, buf = std::move(_buf), size, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not *valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			size = co_await write({buf.get(), size}, {tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await write({buf.get(), size}, {tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::write
(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return write(buf, {tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return write(buf, {tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::write(buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	auto _buf = std::make_shared<char[]>(buf.size);
	memcpy(_buf.get(), buf.data.data(), buf.size);

	size_t size = 0;
	error_code error;

	auto _write = [&]() -> awaitable<size_t>
	{
		do {
			auto res = co_await this->derived()._write_data({_buf.get() + size, buf.size - size}, tk.cnl_sig, error);
			if( error and error.value() != errc::try_again and error.value() != errc::interrupted )
				break;
			size += res;
		}
		while( size < buf.size );
		co_return size;
	};
	if( tk.rtime == 0s )
		co_await _write();
	else
	{
		auto var = co_await (_write() or co_sleep_for(tk.rtime));
		if( var.index() == 1 )
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::write");
	co_return size;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::write_some
(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	auto size = buf.size;
	auto _buf = std::make_shared<char[]>(size);
	memcpy(_buf.get(), buf.data.data(), size);

	co_spawn_detached([this, valid = this->m_valid, buf = std::move(_buf), size, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not *valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			size = co_await write_some({buf.get(), size}, {tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await write_some({buf.get(), size}, {tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::write_some
(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept
{
	auto _callback = [callback = std::move(tk.callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		return write_some(buf, {tk.rtime, *tk.cnl_sig, std::move(_callback)});
	return write_some(buf, {tk.rtime, std::move(_callback)});
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::write_some(buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	auto _buf = std::make_shared<char[]>(buf.size);
	memcpy(_buf.get(), buf.data.data(), buf.size);

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await this->derived()._write_data({_buf.get(), buf.size}, tk.cnl_sig, error);
	else
	{
		auto var = co_await (this->derived()._write_data({_buf.get(), buf.size}, tk.cnl_sig, error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::write");
	co_return size;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::close(opt_token<callback_t<error_code>> tk)
{
	co_spawn_detached([this, valid = this->m_valid, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not *valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			co_await close({tk.rtime, *tk.cnl_sig, error});
		else
			co_await close({tk.rtime, error});

		if( not *valid )
			co_return ;

		tk.callback(error);
		co_return ;
	},
	this->m_exec);
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<void> basic_stream<Derived,Exec>::close(opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	error_code error;

	if( tk.rtime == 0s )
		error = co_await this->derived()._close(tk.cnl_sig);
	else
	{
		auto var = co_await (this->derived()._close(tk.cnl_sig) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			error = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::close");
	co_return ;
}

template <typename Derived, concept_execution Exec>
typename basic_stream<Derived,Exec>::derived_t &basic_stream<Derived,Exec>::cancel() noexcept
{
	_cancel();
	return this->derived();
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::_read_data
(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	m_read_cancel = false;
	if( rc.var.index() == 0 )
	{
		auto delim = std::get<0>(rc.var);
		if( delim.empty() )
		{
			if( cnl_sig )
			{
				buf.size = co_await this->derived().native()
						.async_read_some(asio::buffer(buf.data, buf.size),
										 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await this->derived().native()
						.async_read_some(asio::buffer(buf.data, buf.size), use_awaitable_e[error]);
			}
		}
		else
		{
			std::string _buf;
			if( cnl_sig )
			{
				buf.size = co_await asio::async_read_until
						(this->derived().native(), asio::dynamic_buffer(_buf, buf.size), delim,
						 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
			}
			else
			{
				buf.size = co_await asio::async_read_until
						(this->derived().native(), asio::dynamic_buffer(_buf, buf.size), delim, use_awaitable_e[error]);
			}
			if( buf.size > 0 )
				memcpy(buf.data, _buf.c_str(), buf.size);
		}
	}
	else
	{
		std::string _buf;
		if( cnl_sig )
		{
			buf.size = co_await asio::async_read_until
					(this->derived().native(), asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var),
					 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
		}
		else
		{
			buf.size = co_await asio::async_read_until
					(this->derived().native(), asio::dynamic_buffer(_buf, buf.size), std::get<1>(rc.var), use_awaitable_e[error]);
		}
		if( buf.size > 0 )
			memcpy(buf.data, _buf.c_str(), buf.size);
	}
	if( m_read_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_read_cancel = false;
	}
	co_return buf.size;
}

template <typename Derived, concept_execution Exec>
awaitable<size_t> basic_stream<Derived,Exec>::_write_data
(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept
{
	m_write_cancel = false;
	if( cnl_sig )
	{
		buf.size = co_await this->derived().native().async_write_some
				(asio::buffer(buf.data.data(), buf.size),
				 asio::bind_cancellation_slot(cnl_sig->slot(), use_awaitable_e[error]));
	}
	else
	{
		buf.size = co_await this->derived().native().async_write_some
				(asio::buffer(buf.data.data(), buf.size), use_awaitable_e[error]);
	}
	if( m_write_cancel )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::operation_aborted));
		m_write_cancel = false;
	}
	co_return buf.size;
}

template <typename Derived, concept_execution Exec>
awaitable<error_code> basic_stream<Derived,Exec>::_close(cancellation_signal*) noexcept
{
	error_code error;
	this->derived().native().close(error);
	co_return error;
}

template <typename Derived, concept_execution Exec>
void basic_stream<Derived,Exec>::_cancel() noexcept
{
	m_write_cancel = true;
	m_read_cancel = true;
	m_close_cancel = true;
	this->derived().native().cancel();
}

template <typename Derived, concept_execution Exec>
void basic_stream<Derived,Exec>::__delete_native()
{
	derived_t::_delete_native(m_native);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_STREAM_H
