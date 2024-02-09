
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

#ifndef LIBGS_IO_DEVICE_H
#define LIBGS_IO_DEVICE_H

#include <libgs/io/global.h>
#include <libgs/core/coroutine.h>

namespace libgs::io
{

class device
{
	LIBGS_DISABLE_COPY_MOVE(device)

protected:
	using callback_t = std::function<void(error_code)>;
	using rw_callback_t = std::function<void(size_t,error_code)>;

	using await_size_t = libgs::awaitable<size_t>;
	using await_error_t = libgs::awaitable<error_code>;
	using await_bool_t = libgs::awaitable<error_code>;

	using duration = std::chrono::milliseconds;

public:
	device() = default;
	virtual ~device() = 0;

public:
	[[nodiscard]] virtual bool is_open() const noexcept = 0;
	[[nodiscard]] virtual error_code close() noexcept = 0;

public:
	virtual void async_read_some(void *buf, size_t size, rw_callback_t callback) noexcept = 0;
	[[nodiscard]] virtual size_t read_some(void *buf, size_t size, error_code *error = nullptr) noexcept = 0;
	[[nodiscard]] virtual await_size_t co_read_some(void *buf, size_t size, error_code *error = nullptr) noexcept = 0;

	template <concept_char_type CharT>
	void async_read_some(std::basic_string<CharT> &buf, size_t size, rw_callback_t callback) noexcept;

	template <concept_char_type CharT>
	[[nodiscard]] size_t read_some(std::basic_string<CharT> &buf, size_t size, error_code *error = nullptr) noexcept;
	
	template <concept_char_type CharT>
	[[nodiscard]] await_size_t co_read_some(std::basic_string<CharT> &buf, size_t size, error_code *error = nullptr) noexcept;

public:
	virtual void async_write_some(const void *buf, size_t size, rw_callback_t callback) noexcept = 0;
	[[nodiscard]] virtual size_t write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept = 0;
	[[nodiscard]] virtual await_size_t co_write_some(const void *buf, size_t size, error_code *error = nullptr) noexcept = 0;

	template <concept_char_type CharT>
	void async_write_some(const std::basic_string<CharT> &buf, rw_callback_t callback) noexcept;

	template <concept_char_type CharT>
	[[nodiscard]] size_t write_some(const std::basic_string<CharT> &buf, error_code *error = nullptr) noexcept;
	
	template <concept_char_type CharT>
	[[nodiscard]] await_size_t co_write_some(const std::basic_string<CharT> &buf, error_code *error = nullptr) noexcept;
};

} //namespace libgs::io
#include <libgs/io/detail/device.h>


#endif //LIBGS_IO_DEVICE_H