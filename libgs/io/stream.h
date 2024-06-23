
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

template <typename Stream, typename Derived>
class ssl_stream;

template <typename Derived, concept_execution Exec = asio::any_io_executor>
class LIBGS_IO_TAPI basic_stream : public device_base<crtp_derived_t<Derived,basic_stream<Derived,Exec>>,Exec>
{
	LIBGS_DISABLE_COPY(basic_stream)

public:
	using derived_t = crtp_derived_t<Derived,basic_stream>;
	using base_t = device_base<derived_t,Exec>;
	using executor_t = base_t::executor_t;

public:
	basic_stream(auto *native, const executor_t &exec);
	~basic_stream() override = 0;

	template <concept_execution Exec0>
	basic_stream(basic_stream<Derived,Exec0> &&other) noexcept;

	template <concept_execution Exec0>
	basic_stream &operator=(basic_stream<Derived,Exec0> &&other) noexcept;

public:
	derived_t &read(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

	derived_t &read_some(buffer<void*> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;
	derived_t &read_some(buffer<std::string&> buf, opt_token<read_condition,callback_t<size_t,error_code>> tk) noexcept;

	[[nodiscard]] awaitable<size_t> read_some(buffer<void*> buf, opt_token<read_condition,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read_some(buffer<std::string&> buf, opt_token<read_condition,error_code&> tk = {});

public:
	derived_t &write(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<size_t> write(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

	derived_t &write_some(buffer<std::string_view> buf, opt_token<callback_t<size_t,error_code>> tk) noexcept;
	[[nodiscard]] awaitable<size_t> write_some(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

public:
	derived_t &close(opt_token<callback_t<error_code>> tk);
	[[nodiscard]] awaitable<void> close(opt_token<error_code&> tk = {});
	derived_t &cancel() noexcept;

/*** Derived class implementation required:
 *
 *  [[nodiscard]] size_t read_buffer_size() const noexcept;
 *  [[nodiscard]] size_t write_buffer_size() const noexcept;
 *
 *  [[nodiscard]] const native_type &native() const noexcept;
 *  [[nodiscard]] native_type &native() noexcept;
**/
protected:
	[[nodiscard]] awaitable<size_t> _read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept;

	[[nodiscard]] awaitable<size_t> _write_data
	(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept;

	[[nodiscard]] awaitable<error_code> _close(cancellation_signal *cnl_sig) noexcept;
	void _cancel() noexcept;

	bool m_write_cancel = false;
	bool m_read_cancel = false;
	bool m_close_cancel = false;

	std::function<void(void*)> m_delete_helper;
	void *m_native;

private:
	template <typename Stream, typename Derived0>
	friend class ssl_stream;
	void __delete_native();
};

} //namespace libgs::io
#include <libgs/io/detail/stream.h>


#endif //LIBGS_IO_STREAM_H
