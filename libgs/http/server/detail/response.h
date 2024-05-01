
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server_response<CharT, Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(request_ptr request) :
		m_helper(request->version(), request->headers()),
		m_request(std::move(request))
	{

	}

public:
	helper_type m_helper;
	request_ptr m_request {};
};

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec>::basic_server_response(request_ptr request) :
	io::device_base<Exec>(request->executor())
{
	if( request->m_impl->m_bound )
		throw runtime_error("libgs::http::server::response: The 'request' has been occupied by another 'response'");

	m_impl = new impl(std::move(request));
	m_impl->m_helper.on_write([this](std::string_view buf, error_code &error) -> awaitable<size_t> {
		co_return co_await m_impl->m_request->m_impl->m_socket->write(buf, error);
	});
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec>::~basic_server_response()
{
	m_impl->m_request->m_impl->m_bound = false;
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_status(uint32_t status)
{
	m_impl->m_helper.set_status(status);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_status(http::status status)
{
	m_impl->m_helper.set_status(status);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_header(str_view_type key, value_type value)
{
	m_impl->m_helper.set_header(key, std::move(value));
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_cookie(str_view_type key, cookie_type cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::write(opt_token<error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(tk);
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::write(buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(buf, tk);
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::redirect(str_view_type url, opt_token<redirect_type,error_code&> tk)
{
	co_return co_await m_impl->m_helper.set_redirect(url, tk.type).write(static_cast<opt_token<error_code&>&>(tk));
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<size_t> basic_server_response<CharT,Exec>::send_file(std::string_view file_name, opt_token<ranges,error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(file_name, std::move(tk));
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_chunk_attribute(value_type attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::set_chunk_attributes(value_list_type attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
[[nodiscard]] awaitable<size_t> basic_server_response<CharT,Exec>::chunk_end(opt_token<const headers_type&, error_code&> tk)
{
	co_return co_await m_impl->m_helper.chunk_end(tk);
}

template <concept_char_type CharT, concept_execution Exec>
typename basic_server_response<CharT,Exec>::str_view_type basic_server_response<CharT,Exec>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <concept_char_type CharT, concept_execution Exec>
http::status basic_server_response<CharT,Exec>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_response<CharT,Exec>::headers_type &basic_server_response<CharT,Exec>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <concept_char_type CharT, concept_execution Exec>
const typename basic_server_response<CharT,Exec>::cookies_type &basic_server_response<CharT,Exec>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_response<CharT,Exec>::headers_writed() const noexcept
{
	return m_impl->m_helper.headers_writed();
}

template <concept_char_type CharT, concept_execution Exec>
bool basic_server_response<CharT,Exec>::chunk_end_writed() const noexcept
{
	return m_impl->m_helper.chunk_end_writed();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server_response<CharT,Exec>::cancel() noexcept
{
	m_impl->m_request->m_impl->m_socket->cancel();
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::unset_header(str_view_type key)
{
	m_impl->m_helper.unset_header(key);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::unset_cookie(str_view_type key)
{
	m_impl->m_helper.unset_cookie(key);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server_response<CharT,Exec> &basic_server_response<CharT,Exec>::unset_chunk_attribute(const value_type &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
