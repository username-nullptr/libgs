
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

#ifndef LIBGS_IO_STREAM_H
#define LIBGS_IO_STREAM_H

#include <libgs/io/device_base.h>
#include <libgs/io/types/stream_buffer.h>

namespace libgs::io
{

template <concept_execution Exec = asio::any_io_executor>
class LIBGS_CORE_TAPI basic_stream : public device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_stream)
	using base_type = device_base<Exec>;

public:
	using executor_type = base_type::executor_type;

public:
	using base_type::base_type;
	~basic_stream() override = default;

public:
	[[nodiscard]] virtual bool is_open() const noexcept = 0;
	virtual void close(error_code &error) noexcept = 0;
	void close();

public:
	void read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;

	void read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	void read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;
	void read(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept;
	void read(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

public:
	void write(buffer<const void*> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void write(buffer<const void*> buf, opt_token<callback_t<size_t>> tk) noexcept;

	void write(buffer<const std::string&> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	void write(buffer<const std::string&> buf, opt_token<callback_t<size_t>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> write(buffer<const void*> buf, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(buffer<const std::string&> buf, opt_token<error_code&> tk = {});

public:
	[[nodiscard]] virtual size_t read_buffer_size() const noexcept = 0;
	[[nodiscard]] virtual size_t write_buffer_size() const noexcept = 0;

protected:
	[[nodiscard]] virtual awaitable<size_t> read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept = 0;

	[[nodiscard]] virtual awaitable<size_t> write_data
	(buffer<const void*> buf, cancellation_signal *cnl_sig, error_code &error) noexcept = 0;
};

using stream = basic_stream<>;

template <concept_execution Exec = asio::any_io_executor>
using basic_stream_ptr = std::shared_ptr<basic_stream<Exec>>;

using stream_ptr = basic_stream_ptr<>;

} //namespace libgs::io
#include <libgs/io/detail/stream.h>


#endif //LIBGS_IO_STREAM_H
