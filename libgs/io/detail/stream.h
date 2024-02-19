
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

namespace libgs::io
{

template <concept_execution Exec>
void basic_stream<Exec>::close() noexcept
{
	error_code error;
	close(error);
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<void*> buf, cb_token<size_t,error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		auto size = co_await read(buf, {tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<void*> buf, cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	read(buf, {tk.rtime, [callback = std::move(callback)](size_t size, const error_code&) {
		callback(size);
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<void*> buf, opt_token<ua_redirect_error_t> tk) noexcept
{
	using namespace std::chrono;
	if( buf.size == 0 )
		co_return 0;

	else if( tk.rtime == 0s )
		co_return co_await read_data(buf.data, buf.size, tk.uare.ec_);
	
	auto var = co_await (read_data(buf.data, buf.size, tk.uare.ec_) or co_sleep_for(tk.rtime));
	size_t size = 0;

	if( var.index() == 0 )
		size = std::get<0>(var);
	else
		tk.uare.ec_ = std::make_error_code(std::errc::timed_out);
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<void*> buf, opt_token<use_awaitable_t&> tk) noexcept
{
	error_code error;
	co_return co_await read(buf, {tk.rtime, use_awaitable_e[error]});
}

template <concept_execution Exec>
size_t basic_stream<Exec>::read(buffer<void*> buf) noexcept
{
	error_code error;
	return read(buf, error);
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, cb_token<size_t,error_code> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = std::make_shared<char[]>(buf.size);
	auto callback = std::move(tk.callback);

	read({_buf.get(), buf.size}, {tk.rtime, [buf, _buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		buf.data = std::string(_buf.get(), size);
		callback(size, error);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	read(buf, {tk.rtime, [callback = std::move(callback)](size_t size, const error_code&) {
		callback(size);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, cb_token<error_code> tk) noexcept
{
	auto callback = std::move(tk.callback);
	read(buf, {tk.rtime, [callback = std::move(callback)](size_t, const error_code &error){
		callback(error);
	}});
}

template <concept_execution Exec>
void basic_stream<Exec>::read(buffer<std::string&> buf, cb_token<> tk) noexcept
{
	auto callback = std::move(tk.callback);
	read(buf, {tk.rtime, [callback = std::move(callback)](const error_code&) {
		callback();
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<ua_redirect_error_t> tk) noexcept
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = new char[buf.size] {0};
	auto size = co_await read({_buf, buf.size}, std::move(tk));

	buf.data = std::string(_buf, size);
	delete[] _buf;
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::read(buffer<std::string&> buf, opt_token<use_awaitable_t&> tk) noexcept
{
	error_code error;
	co_return co_await read(buf, {tk.rtime, use_awaitable_e[error]});
}

template <concept_execution Exec>
size_t basic_stream<Exec>::read(buffer<std::string&> buf, error_code &error) noexcept
{
	if( buf.size == 0 )
		buf.size = read_buffer_size();
	
	auto _buf = new char[buf.size] {0};
	auto size = read({_buf, buf.size}, error);

	buf.data = std::string(_buf, size);
	delete[] _buf;
	return size;
}

template <concept_execution Exec>
size_t basic_stream<Exec>::read(buffer<std::string&> buf) noexcept
{
	error_code error;
	return read(buf, error);
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<const void*> buf, cb_token<size_t,error_code> tk) noexcept
{
	auto valid = this->m_valid;
	co_spawn_detached([this, valid = std::move(valid), buf, tk = std::move(tk)]() -> awaitable<void>
	{
		if( not valid )
			co_return ;

		error_code error;
		auto size = co_await write(buf, {tk.rtime, use_awaitable_e[error]});

		if( not valid )
			co_return ;

		tk.callback(size, error);
		co_return ;
	},
	this->m_exec);
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<const void*> buf, cb_token<size_t> tk) noexcept
{
	auto callback = std::move(tk.callback);
	async_write(buf, {tk.rtime, [callback = std::move(callback)](size_t size, const error_code&){
		callback(size);
	}});
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::write(buffer<const void*> buf, opt_token<ua_redirect_error_t> tk) noexcept
{
	using namespace std::chrono;
	if( buf.size == 0 )
		co_return 0;

	auto _buf = std::make_shared<char[]>(buf.size);
	memcpy(_buf.get(), buf.data, buf.size);

	if( tk.rtime == 0s )
		co_return co_await write_data(_buf.get(), buf.size, tk.uare.ec_);

	auto no_time_out = [&]() -> awaitable<size_t>
	{
		size_t sum = 0;
		do {
			auto res = co_await write_data(_buf.get() + sum, buf.size - sum, tk.uare.ec_);
			if( tk.uare.ec_ and tk.uare.ec_ != errc::try_again )
				break;
			sum += res;
		}
		while( sum < buf.size );
		co_return sum;
	};

	auto var = co_await (no_time_out() or co_sleep_for(tk.rtime));
	size_t size = 0;

	if( var.index() == 0 )
		size = std::get<0>(var);
	else
		tk.uare.ec_ = std::make_error_code(std::errc::timed_out);
	co_return size;
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::write(buffer<const void*> buf, opt_token<use_awaitable_t&> tk) noexcept
{
	error_code error;
	co_return co_await write(buf, {tk.rtime, use_awaitable_e[error]});
}

template <concept_execution Exec>
size_t basic_stream<Exec>::write(buffer<const void*> buf) noexcept
{
	error_code error;
	return write(buf, error);
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<const std::string&> buf, cb_token<size_t,error_code> tk) noexcept
{
	write({buf.data, buf.size}, std::move(tk));
}

template <concept_execution Exec>
void basic_stream<Exec>::write(buffer<const std::string&> buf, cb_token<size_t> tk) noexcept
{
	write({buf.data, buf.size}, std::move(tk));
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::write(buffer<const std::string&> buf, opt_token<ua_redirect_error_t> tk) noexcept
{
	return write({buf.data, buf.size}, std::move(tk));
}

template <concept_execution Exec>
awaitable<size_t> basic_stream<Exec>::write(buffer<const std::string&> buf, opt_token<use_awaitable_t&> tk) noexcept
{
	return write({buf.data, buf.size}, std::move(tk));
}

template <concept_execution Exec>
size_t basic_stream<Exec>::write(buffer<const std::string&> buf, error_code &error) noexcept
{
	return write({buf.data, buf.size}, error);
}

template <concept_execution Exec>
size_t basic_stream<Exec>::write(buffer<const std::string&> buf) noexcept
{
	return write({buf.data, buf.size});
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_STREAM_H
