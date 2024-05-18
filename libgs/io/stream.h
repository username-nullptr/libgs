
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

template <typename Derived, concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI basic_stream : public device_base<crtp_derived<Derived,basic_stream<Exec,Derived>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_stream)
	using base_type = device_base<crtp_derived<Derived,basic_stream>,Exec>;

public:
	using executor_type = base_type::executor_type;
	using derived_type = crtp_derived_t<Derived, basic_stream>;

public:
	using base_type::device_base;
	~basic_stream() override = 0;

	basic_stream(basic_stream &&other) noexcept = default;
	basic_stream &operator=(basic_stream &&other) noexcept = default;

public:
	derived_type &read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_type &read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;

	derived_type &read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_type &read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;
	derived_type &read(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept;
	derived_type &read(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

	derived_type &read_some(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_type &read_some(buffer<void*> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;

	derived_type &read_some(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_type &read_some(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t>> tk) noexcept;
	derived_type &read_some(buffer<std::string&> buf, opt_token<read_condition,callback_t<error_code>> tk) noexcept;
	derived_type &read_some(buffer<std::string&> buf, opt_token<read_condition,callback_t<>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read_some(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read_some(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

public:
	derived_type &write(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_type &write(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept;
	[[nodiscard]] awaitable<size_t> write(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

	derived_type &write_some(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	derived_type &write_some(buffer<std::string_view> buf, opt_token<callback_t<size_t>> tk) noexcept;
	[[nodiscard]] awaitable<size_t> write_some(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

/*** Derived class implementation required:
 *
 *  [[nodiscard]] size_t read_buffer_size() const noexcept;
 *  [[nodiscard]] size_t write_buffer_size() const noexcept;
 *
 *  [[nodiscard]] awaitable<size_t> _read_some
 *  (buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept;
 *
 *  [[nodiscard]] awaitable<size_t> _write_some
 *  (buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept;
**/
};

} //namespace libgs::io
#include <libgs/io/detail/stream.h>


#endif //LIBGS_IO_STREAM_H
