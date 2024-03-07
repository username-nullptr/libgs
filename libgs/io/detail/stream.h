
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

template <concept_execution Exec>
void basic_stream<Exec>::close()
{
	error_code error;
	close(error);

	if( error )
		throw system_error(error, "libgs::io::stream::close");
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<void*> buf, read_cb_token<size_t,error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		auto size = co_await co_read(buf, {tk.rc, tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<void*> buf, read_cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_read(buf, {std::move(tk.rc), tk.rtime, [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::co_read(buffer<void*> buf, read_token<error_code&> tk)
{
	using namespace std::chrono;
	if( buf.size == 0 )
		co_return 0;

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await read_data(buf.data, buf.size, std::move(tk.rc), error);
	else
	{
		auto var = co_await(read_data(buf.data, buf.size, std::move(tk.rc), error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	if( error )
	{
		if( tk.error == nullptr )
			throw system_error(error, "libgs::io::stream::co_read");
		*tk.error = error;
	}
	co_return size;
}

template <concept_execution Exec>
size_t basic_stream<Exec>::read(buffer<void*> buf)
{
	return read(buf,{});
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<std::string&> buf, read_cb_token<size_t,error_code> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto callback = std::move(tk.callback);

	async_read({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, [buf, _buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		buf.data = std::string(_buf.get(), size);
		callback(size, error);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<std::string&> buf, read_cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_read(buf, {std::move(tk.rc), tk.rtime, [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<std::string&> buf, read_cb_token<error_code> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_read(buf, {std::move(tk.rc), tk.rtime, [callback = std::move(callback)](size_t, const error_code &error){
		callback(error);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::async_read(buffer<std::string&> buf, read_cb_token<> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_read(buf, {std::move(tk.rc), tk.rtime, [callback = std::move(callback)](const error_code&){
		callback();
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::co_read(buffer<std::string&> buf, read_token<error_code&> tk)
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto size = co_await co_read({_buf.get(), buf.size}, std::move(tk));

	buf.data = std::string(_buf.get(), size);
	co_return size;
}

template <concept_execution Exec>
size_t basic_stream<Exec>::read(buffer<std::string&> buf, read_token<error_code&> tk)
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto size = read({_buf.get(), buf.size}, std::move(tk));

	buf.data = std::string(_buf.get(), size);
	return size;
}

template <concept_execution Exec>
void basic_stream<Exec>::async_write(buffer<const void*> buf, opt_cb_token<size_t,error_code> tk) noexcept
{
	auto size = buf.size;
	auto _buf = std::make_shared<char[]>(size);

	memcpy(_buf.get(), buf.data, size);
	auto valid = this->m_valid;

	co_spawn_detached([this, valid = std::move(valid), buf = std::move(_buf), size, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		size = co_await co_write({buf.get(), size}, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::async_write(buffer<const void*> buf, opt_cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_write(buf, {tk.rtime, [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::co_write(buffer<const void*> buf, opt_token<error_code&> tk)
{
	using namespace std::chrono;
	if( buf.size == 0 )
		co_return 0;

	auto _buf = std::make_shared<char[]>(buf.size);
	memcpy(_buf.get(), buf.data, buf.size);

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await write_data(_buf.get(), buf.size, error);
	else
	{
		auto no_time_out = [&]() -> awaitable<size_t>
		{
			size_t sum = 0;
			do {
				auto res = co_await write_data(_buf.get() + sum, buf.size - sum, error);
				if( error and error.value() != errc::try_again )
					break;
				sum += res;
			}
			while( sum < buf.size );
			co_return sum;
		};
		auto var = co_await (no_time_out() or co_sleep_for(tk.rtime));

		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	if( error )
	{
		if( tk.error == nullptr )
			throw system_error(error, "libgs::io::stream::co_write");
		*tk.error = error;
	}
	co_return size;
}

template <concept_execution Exec>
size_t basic_stream<Exec>::write(buffer<const void*> buf)
{
	return write(buf,{});
}

template <concept_execution Exec>
void basic_stream<Exec>::async_write(buffer<const std::string&> buf, opt_cb_token<size_t,error_code> tk) noexcept
{
	async_write({buf.data.c_str(), buf.size}, std::move(tk));
}

template <concept_execution Exec>
void basic_stream<Exec>::async_write(buffer<const std::string&> buf, opt_cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_write(buf, {tk.rtime, [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::co_write(buffer<const std::string&> buf, opt_token<error_code&> tk)
{
	return co_write({buf.data.c_str(), buf.size}, tk);
}

template <concept_execution Exec>
size_t basic_stream<Exec>::write(buffer<const std::string&> buf, opt_token<error_code&> tk)
{
	return write({buf.data.c_str(), buf.size}, tk);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_STREAM_H
