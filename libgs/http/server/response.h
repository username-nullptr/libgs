
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
#include <libgs/core/value.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_VAPI basic_server_response : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_server_response)
	using base_type = io::device_base<Exec>;
	using this_type = basic_server_response;

public:
	using executor_type = typename base_type::executor_type;
	using request = basic_server_request<CharT,Exec>;
	using request_ptr = basic_server_request_ptr<CharT,Exec>;
	using helper_type = basic_response_helper<CharT>;

	using str_type = typename request::str_type;
	using str_view_type = typename request::str_view_type;
	using socket_ptr = typename request::socket_ptr;

	using headers_type = typename request::headers_type;
	using cookie_type = basic_cookie<CharT>;
	using cookies_type = basic_cookies<CharT>;

	using value_type = typename request::value_type;
	using value_list_type = basic_value_list<CharT>;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	explicit basic_server_response(request_ptr request);
	~basic_server_response();

public:
	this_type &set_status(uint32_t status);
	this_type &set_status(http::status status);

	this_type &set_header(str_view_type key, value_type value);
	this_type &set_cookie(str_view_type key, cookie_type cookie);

public:
	[[nodiscard]] awaitable<size_t> write(opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(buffer<const void*> buf, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(buffer<const std::string&> buf, opt_token<error_code&> tk = {});

	[[nodiscard]] awaitable<size_t> redirect(str_view_type url, redirect_type type = redirect_type::moved_permanently);
	[[nodiscard]] awaitable<size_t> send_file(std::string_view file_name, opt_token<ranges,error_code&> tk = {});

public:
	this_type &set_chunk_attribute(value_type attribute);
	this_type &set_chunk_attributes(value_list_type attributes);
	[[nodiscard]] awaitable<size_t> chunk_end(const headers_type &headers = {});

public:
	[[nodiscard]] str_view_type version() const;
	[[nodiscard]] http::status status() const;

	[[nodiscard]] const headers_type &headers() const;
	[[nodiscard]] const cookies_type &cookies() const;

	[[nodiscard]] bool is_writed() const;
	void cancel() noexcept override;

public:
	this_type &unset_header(str_view_type key);
	this_type &unset_cookie(str_view_type key);

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
	explicit impl(request_ptr request) :
		m_helper(request->version(), request->headers()),
		m_request(std::move(request)) {}

public:
	helper_type m_helper;
	request_ptr m_request;
};

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec>::basic_server_response(request_ptr request) :
	io::device_base<Exec>(request->executor()),
	m_impl(new impl(std::move(request)))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec>::~basic_server_response()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_status(uint32_t status)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_status(http::status status)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_header(str_view_type key, value_type value)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_cookie(str_view_type key, cookie_type cookie)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::write(opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::write(buffer<const void*> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::write(buffer<const std::string&> buf, opt_token<error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::redirect(str_view_type url, redirect_type type)
{

}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::send_file(std::string_view file_name, opt_token<ranges,error_code&> tk)
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_chunk_attribute(value_type attribute)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_chunk_attributes(value_list_type attributes)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
[[nodiscard]] awaitable<size_t> basic_server_response<CharT,Exec>::chunk_end(const headers_type &headers)
{

}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_response<CharT,Exec>::str_view_type basic_server_response<CharT,Exec>::version() const
{

}

template <concept_char_type CharT, concept_execution Exec>
http::status basic_server_response<CharT,Exec>::status() const
{

}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_response<CharT,Exec>::headers_type &basic_server_response<CharT,Exec>::headers() const
{

}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_response<CharT,Exec>::cookies_type &basic_server_response<CharT,Exec>::cookies() const
{

}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_response<CharT,Exec>::is_writed() const
{

}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_response<CharT,Exec>::cancel() noexcept
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::unset_header(str_view_type key)
{
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::unset_cookie(str_view_type key)
{
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_RESPONSE_H
