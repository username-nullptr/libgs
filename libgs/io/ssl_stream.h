
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
	using executor_type = base_type::executor_type;

public:
	basic_ssl_stream(std::shared_ptr<stream_type> stream);
	basic_ssl_stream(stream_type *stream);
	~basic_ssl_stream() override = default;

public:
};

template <concept_base_of_stream Stream>
using ssl_stream = basic_ssl_stream<Stream>;

template <concept_base_of_stream Stream, concept_execution Exec = asio::any_io_executor>
using basic_ssl_stream_ptr = std::shared_ptr<basic_ssl_stream<Stream, Exec>>;

template <concept_base_of_stream Stream>
using ssl_stream_ptr = basic_stream_ptr<Stream>;

} //namespace libgs::io


#endif //LIBGS_IO_SSL_STREAM_H
