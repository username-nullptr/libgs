
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

#ifndef LIBGS_IO_DETAIL_DEVICE_H
#define LIBGS_IO_DETAIL_DEVICE_H

namespace libgs::io
{

template <concept_char_type CharT>
void device::async_read_some(std::basic_string<CharT> &buf, size_t size, rw_callback_t callback) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	async_read_some(ptr.get(), size, [ptr, &buf, callback = std::move(callback)](size_t size, const error_code &error)
	{
		if constexpr( is_char_v<CharT> )
			buf = std::string(ptr.get(), size);
		else
			buf = mbstowcs(std::string(ptr.get(), size));
		callback(size, error);
	});
}

template <concept_char_type CharT>
size_t device::read_some(std::basic_string<CharT> &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = read_some(ptr.get(), size, error);

	if constexpr( is_char_v<CharT> )
		buf = std::string(ptr.get(), size);
	else
		buf = mbstowcs(std::string(ptr.get(), size));
	return res;
}

template <concept_char_type CharT>
awaitable<size_t> device::co_read_some(std::basic_string<CharT> &buf, size_t size, error_code *error) noexcept
{
	auto ptr = std::make_shared<char[]>(size);
	auto res = co_await co_read_some(ptr.get(), size, error);

	if constexpr( is_char_v<CharT> )
		buf = std::string(ptr.get(), size);
	else
		buf = mbstowcs(std::string(ptr.get(), size));
	co_return res;
}

template <concept_char_type CharT>
void device::async_write_some(const std::basic_string<CharT> &buf, rw_callback_t callback) noexcept
{
	if constexpr( is_char_v<CharT> )
		async_write_some(buf.c_str(), buf.size(), std::move(callback));
	else
	{
		auto tmp = wcstombs(buf);
		async_write_some(tmp.c_str(), tmp.size(), std::move(callback));
	}
}

template <concept_char_type CharT>
size_t device::write_some(const std::basic_string<CharT> &buf, error_code *error) noexcept
{
	if constexpr( is_char_v<CharT> )
		return write_some(buf.c_str(), buf.size(), error);
	else
	{
		auto tmp = wcstombs(buf);
		return write_some(tmp.c_str(), tmp.size(), error);
	}
}

template <concept_char_type CharT>
awaitable<size_t> device::co_write_some(const std::basic_string<CharT> &buf, error_code *error) noexcept
{
	if constexpr( is_char_v<CharT> )
		co_return co_write_some(buf.c_str(), buf.size(), error);
	else
	{
		auto tmp = wcstombs(buf);
		co_return co_write_some(tmp.c_str(), tmp.size(), error);
	}
}

} //namespace libgs::io


#endif //LIBGS_IO_DETAIL_DEVICE_H
