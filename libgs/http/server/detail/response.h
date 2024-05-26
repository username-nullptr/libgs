
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

template <typename Request, concept_char_type CharT, typename Derived>
class basic_server_response<Request,CharT,Derived>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	explicit impl(next_layer_t &&next_layer) :
		m_helper(next_layer.version(), next_layer.headers()),
		m_next_layer(std::move(next_layer)) {}

	impl(impl &&other) noexcept = default;
	impl &operator=(impl &&other) noexcept = default;

public:
	helper_t m_helper;
	next_layer_t m_next_layer;
};

template <typename Request, concept_char_type CharT, typename Derived>
basic_server_response<Request,CharT,Derived>::basic_server_response(next_layer_t &&next_layer) :
	base_t(next_layer.executor()), m_impl(new impl(std::move(next_layer)))
{
	m_impl->m_helper.on_write([this](std::string_view buf, error_code &error) -> awaitable<size_t> {
		co_return co_await m_impl->m_next_layer.next_layer().write(buf, error);
	});
}

template <typename Request, concept_char_type CharT, typename Derived>
basic_server_response<Request,CharT,Derived>::~basic_server_response()
{
	delete m_impl;
}

template <typename Request, concept_char_type CharT, typename Derived>
basic_server_response<Request,CharT,Derived>::basic_server_response(basic_server_response &&other) noexcept :
	base_t(std::move(other)), m_impl(new impl(std::move(*other.m_impl)))
{

}

template <typename Request, concept_char_type CharT, typename Derived>
basic_server_response<Request,CharT,Derived> &basic_server_response<Request,CharT,Derived>::operator=
(basic_server_response &&other) noexcept
{
	base_t::operator=(std::move(other));
	*m_impl = std::move(*other.m_impl);
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_status(uint32_t status)
{
	m_impl->m_helper.set_status(status);
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_status(http::status status)
{
	m_impl->m_helper.set_status(status);
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_header(string_view_t key, value_t value)
{
	m_impl->m_helper.set_header(key, std::move(value));
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_cookie(string_view_t key, cookie_t cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_response<Request,CharT,Derived>::write(opt_token<error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(tk);
}

template <typename Request, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_response<Request,CharT,Derived>::write
(buffer<std::string_view> buf, opt_token<error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(buf, tk);
}

template <typename Request, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_response<Request,CharT,Derived>::redirect
(string_view_t url, opt_token<http::redirect,error_code&> tk)
{
	co_return co_await m_impl->m_helper.set_redirect(url, tk.type).write(static_cast<opt_token<error_code&>&>(tk));
}

template <typename Request, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_response<Request,CharT,Derived>::send_file
(std::string_view file_name, opt_token<ranges,error_code&> tk)
{
	co_return co_await m_impl->m_helper.write(file_name, std::move(tk));
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_chunk_attribute(value_t attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::set_chunk_attributes(value_list_t attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
[[nodiscard]] awaitable<size_t> basic_server_response<Request,CharT,Derived>::chunk_end(opt_token<const headers_t&, error_code&> tk)
{
	co_return co_await m_impl->m_helper.chunk_end(tk);
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::string_view_t
basic_server_response<Request,CharT,Derived>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <typename Request, concept_char_type CharT, typename Derived>
http::status basic_server_response<Request,CharT,Derived>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <typename Request, concept_char_type CharT, typename Derived>
const typename basic_server_response<Request,CharT,Derived>::headers_t&
basic_server_response<Request,CharT,Derived>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <typename Request, concept_char_type CharT, typename Derived>
const typename basic_server_response<Request,CharT,Derived>::cookies_t&
basic_server_response<Request,CharT,Derived>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <typename Request, concept_char_type CharT, typename Derived>
bool basic_server_response<Request,CharT,Derived>::headers_writed() const noexcept
{
	return m_impl->m_helper.headers_writed();
}

template <typename Request, concept_char_type CharT, typename Derived>
bool basic_server_response<Request,CharT,Derived>::chunk_end_writed() const noexcept
{
	return m_impl->m_helper.chunk_end_writed();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::unset_header(string_view_t key)
{
	m_impl->m_helper.unset_header(key);
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::unset_cookie(string_view_t key)
{
	m_impl->m_helper.unset_cookie(key);
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::derived_t&
basic_server_response<Request,CharT,Derived>::unset_chunk_attribute(const value_t &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return this->derived();
}

template <typename Request, concept_char_type CharT, typename Derived>
const typename basic_server_response<Request,CharT,Derived>::next_layer_t&
basic_server_response<Request,CharT,Derived>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <typename Request, concept_char_type CharT, typename Derived>
typename basic_server_response<Request,CharT,Derived>::next_layer_t&
basic_server_response<Request,CharT,Derived>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
