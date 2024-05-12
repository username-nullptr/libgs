
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
void basic_stream<Exec>::read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		size_t size;
		error_code error;

		if( tk.cnl_sig )
			size = co_await read(buf, {tk.rc, tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await read(buf, {tk.rc, tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	auto _callback = [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		async_read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		async_read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto callback = std::move(tk.callback);

	auto _callback = [buf, _buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		buf.data = std::string(_buf.get(), size);
		callback(size, error);
	};
	if( tk.cnl_sig )
		async_read({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		async_read({_buf.get(), buf.size}, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	auto _callback = [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		async_read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		async_read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	auto _callback = [callback = std::move(callback)](size_t, const error_code &error){
		callback(error);
	};
	if( tk.cnl_sig )
		async_read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		async_read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	auto _callback = [callback = std::move(callback)](const error_code&){
		callback();
	};
	if( tk.cnl_sig )
		async_read(buf, {std::move(tk.rc), tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		async_read(buf, {std::move(tk.rc), tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<void*> buf, opt_token<read_condition,error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await read_data(buf, std::move(tk.rc), tk.cnl_sig, error);
	else
	{
		auto var = co_await(read_data(buf, std::move(tk.rc), tk.cnl_sig, error) or co_sleep_for(tk.rtime));
		if( var.index() == 0 )
			size = std::get<0>(var);
		else
			error = std::make_error_code(static_cast<std::errc>(errc::timed_out));
	}
	detail::check_error(tk.error, error, "libgs::io::stream::read");
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk)
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto size = co_await read({_buf.get(), buf.size}, std::move(tk));

	buf.data = std::string(_buf.get(), size);
	co_return size;
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept
{
	auto size = buf.size;
	auto _buf = std::make_shared<char[]>(size);

	memcpy(_buf.get(), buf.data.data(), size);
	auto valid = this->m_valid;

	co_spawn_detached([this, valid = std::move(valid), buf = std::move(_buf), size, tk = std::move(tk)]() mutable -> awaitable<void>
	{
		if( not valid )
			co_return ;
		error_code error;

		if( tk.cnl_sig )
			size = co_await write({buf.get(), size}, {tk.rtime, *tk.cnl_sig, error});
		else
			size = co_await write({buf.get(), size}, {tk.rtime, error});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept
{
	auto callback = std::move(tk.callback);
	auto _callback = [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	};
	if( tk.cnl_sig )
		write(buf, {tk.rtime, *tk.cnl_sig, std::move(_callback)});
	else
		write(buf, {tk.rtime, std::move(_callback)});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::write(buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	using namespace std::chrono_literals;
	if( buf.size == 0 )
		co_return 0;

	auto _buf = std::make_shared<char[]>(buf.size);
	memcpy(_buf.get(), buf.data.data(), buf.size);

	size_t size = 0;
	error_code error;

	if( tk.rtime == 0s )
		size = co_await write_data({_buf.get(), buf.size}, tk.cnl_sig, error);
	else
	{
		auto no_time_out = [&]() -> awaitable<size_t>
		{
			size_t sum = 0;
			do {
				auto res = co_await write_data({_buf.get() + sum, buf.size - sum}, tk.cnl_sig, error);
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
	detail::check_error(tk.error, error, "libgs::io::stream::write");
	co_return size;
}

template <concept_execution Exec>
basic_stream<Exec>::basic_stream(auto *handle, concept_callable auto &&del_handle) :
	base_type(handle->get_executor()),
	m_handle(handle),
	m_del_handle(std::forward<std::decay_t<decltype(del_handle)>>(del_handle))
{
	assert(m_handle);
	assert(m_del_handle);
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_STREAM_H
