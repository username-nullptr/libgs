
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

#ifndef LIBGS_IO_SSL_STREAM_H
#define LIBGS_IO_SSL_STREAM_H

#ifdef __ENABLE_SSL__

#include <libgs/io/stream.h>

namespace libgs::io
{

// TODO ...
template <concept_base_of_stream Stream, concept_execution Exec = asio::any_io_executor>
class LIBGS_CORE_TAPI basic_ssl_stream : public basic_stream<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_ssl_stream)
	using base_type = basic_stream<Exec>;

public:
	using stream_type = Stream;
	using stream_ptr = std::shared_ptr<stream_type>;
	using executor_type = base_type::executor_type;

public:
	basic_ssl_stream(stream_ptr stream);
	basic_ssl_stream(stream_type *stream);
	~basic_ssl_stream() override = default;

public:
	[[nodiscard]] awaitable<size_t> read_data
	(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept override;

	[[nodiscard]] awaitable<size_t> write_data
	(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept override;

public:
//	[[nodiscard]] const stream_ptr native_object() const;
//	[[nodiscard]] stream_ptr native_object();

protected:
	stream_ptr m_stream;
};

template <concept_base_of_stream Stream, concept_execution Exec>
basic_ssl_stream<Stream,Exec>::basic_ssl_stream(stream_ptr stream) :
	m_stream(std::move(stream))
{

}

template <concept_base_of_stream Stream, concept_execution Exec>
basic_ssl_stream<Stream,Exec>::basic_ssl_stream(stream_type *stream) :
	m_stream(stream)
{

}

template <concept_base_of_stream Stream, concept_execution Exec>
awaitable<size_t> basic_ssl_stream<Stream,Exec>::read_data
(buffer<void*> buf, read_condition rc, cancellation_signal *cnl_sig, error_code &error) noexcept
{

}

template <concept_base_of_stream Stream, concept_execution Exec>
awaitable<size_t> basic_ssl_stream<Stream,Exec>::write_data
(buffer<std::string_view> buf, cancellation_signal *cnl_sig, error_code &error) noexcept
{

}

template <concept_base_of_stream Stream>
using ssl_stream = basic_ssl_stream<Stream>;

template <concept_base_of_stream Stream, concept_execution Exec = asio::any_io_executor>
using basic_ssl_stream_ptr = std::shared_ptr<basic_ssl_stream<Stream, Exec>>;

template <concept_base_of_stream Stream>
using ssl_stream_ptr = basic_stream_ptr<Stream>;

} //namespace libgs::io
//#include <libgs/io/detail/ssl_stream.h>

#endif //__ENABLE_SSL__

#endif //LIBGS_IO_SSL_STREAM_H
