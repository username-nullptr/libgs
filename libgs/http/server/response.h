
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

#ifndef LIBGS_HTTP_SERVER_RESPONSE_H
#define LIBGS_HTTP_SERVER_RESPONSE_H

#include <libgs/http/server/request.h>
#include <libgs/http/server/response_helper.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_VAPI basic_server_response : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY(basic_server_response)

public:
	using request = basic_server_request<CharT,Exec>;
	using request_ptr = basic_server_request_ptr<CharT,Exec>;

public:
	explicit basic_server_response(request_ptr request);
	~basic_server_response() = default;

public:

private:
	class impl;
	impl *m_impl;
};

using server_response = basic_server_response<char>;
using server_wresponse = basic_server_response<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
using basic_server_response_ptr = std::shared_ptr<basic_server_response<CharT,Exec>>;

using server_response_ptr = std::shared_ptr<basic_server_response<char>>;
using server_wresponse_ptr = std::shared_ptr<basic_server_response<char>>;

} //namespace libgs::http



namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server_response<CharT, Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
};

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec>::basic_server_response(basic_server_response::request_ptr request)
{

}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_RESPONSE_H
