
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

#ifndef LIBGS_HTTP_SERVER_RESPONSE_HELPER_H
#define LIBGS_HTTP_SERVER_RESPONSE_HELPER_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_response_helper
{
	LIBGS_DISABLE_COPY(basic_response_helper)
	using this_type = basic_response_helper;

public:
	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;

	using value_type = basic_value<CharT>;
	using value_list_type = basic_value_list<CharT>;

	using header_type = basic_header<CharT>;
	using headers_type = basic_headers<CharT>;

	using cookie_type = basic_cookie<CharT>;
	using cookies_type = basic_cookies<CharT>;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	explicit basic_response_helper(str_view_type version, const headers_type &headers);
	~basic_response_helper();

	basic_response_helper(basic_response_helper &&other) noexcept ;
	basic_response_helper &operator=(basic_response_helper &&other) noexcept;

public:
	this_type &set_status(uint32_t status);
	this_type &set_status(http::status status);

	this_type &set_header(str_view_type key, value_type value) noexcept;
	this_type &set_cookie(str_view_type key, cookie_type cookie) noexcept;

	this_type &set_redirect(str_view_type url, redirect_type type = redirect_type::moved_permanently);

	this_type &set_chunk_attribute(value_type attribute);
	this_type &set_chunk_attributes(value_list_type attributes);

public:
	using write_callback = std::function<awaitable<size_t>(std::string_view,error_code&)>;
	this_type &on_write(write_callback writer) const;

	[[nodiscard]] awaitable<size_t> write(opt_token<error_code&> tk = {}) const;
	[[nodiscard]] awaitable<size_t> write(buffer<const void*> body, opt_token<error_code&> tk = {}) const;
	[[nodiscard]] awaitable<size_t> write(buffer<const std::string&> body, opt_token<error_code&> tk = {}) const;
	[[nodiscard]] awaitable<size_t> write(std::ifstream &file, opt_token<error_code&> tk = {}) const;

public:
	[[nodiscard]] str_view_type version() const;
	[[nodiscard]] http::status status() const;

	[[nodiscard]] const headers_type &headers() const;
	[[nodiscard]] const cookies_type &cookies() const;

public:
	this_type &unset_header(str_view_type key);
	this_type &unset_cookie(str_view_type key);
	this_type &unset_chunk_attribute(value_type key);
	this_type &reset();

private:
	class impl;
	impl *m_impl;
};

using response_helper = basic_response_helper<char>;
using wresponse_helper = basic_response_helper<wchar_t>;

} //namespace libgs::http

namespace libgs::http
{

namespace detail
{

template <typename T>
struct _header_value_static_string;

template <>
struct _header_value_static_string<char> {
	static constexpr const char *content_type = "text/plain; charset=utf-8";
};

template <>
struct _header_value_static_string<wchar_t> {
	static constexpr const wchar_t *content_type = L"text/plain; charset=utf-8";
};

} //namespace detail

template <concept_char_type CharT>
class basic_response_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using key_static_string = detail::_key_static_string<CharT>;
	using header_value_static_string = detail::_header_value_static_string<CharT>;

public:
	impl() = default;
	impl(str_view_type version, const headers_type &headers) :
		m_version(version), m_request_headers(headers) {}

public:
	[[nodiscard]] awaitable<size_t> write_header(size_t body_size, opt_token<error_code&> tk)
	{
		m_headers_writed = true;
		auto it = m_response_headers.find(header_type::content_length);

		if( it == m_response_headers.end() )
		{
			if( stoi32(m_version) > 1.0 )
			{
				it = m_response_headers.find(header_type::transfer_encoding);
				if( it == m_response_headers.end() or str_to_lower(it->second) != key_static_string::chunked )
					m_response_headers[str_to_lower(header_type::content_length)] = body_size;
			}
			else
				m_response_headers[str_to_lower(header_type::content_length)] = body_size;
		}
		std::string buf;
		buf.reserve(4096);

		buf = std::format("HTTP/{} {} {}\r\n", m_version, m_status, to_status_description(m_status));
		m_response_headers.erase("set-cookie");

		for(auto &[key,value] : m_response_headers)
			buf += key + ": " + value + "\r\n";

		for(auto &[key,cookie] : m_cookies)
		{
			buf += "set-cookie: " + key + "=" + cookie + ";";
			for(auto &attr_pair : cookie.attributes())
				buf += attr_pair.first + "=" + attr_pair.second + ";";

			buf.pop_back();
			buf += "\r\n";
		}
		error_code error;
		auto res = co_await m_writer(buf, error);

		io::detail::check_error(tk.error, error, "libgs::http::response_helper::write");
		co_return res;
	}

	[[nodiscard]] awaitable<size_t> write_body(const void *buf, size_t size, opt_token<error_code&> tk)
	{
		error_code error;
		if( not is_chunked() )
		{
			size = co_await m_writer({static_cast<const char*>(buf), size}, error);
			io::detail::check_error(tk.error, error, "libgs::http::response_helper::write");
			co_return size;
		}
		else if( m_chunk_attributes.empty() )
		{
			size = co_await m_writer(std::format("{:X}\r\n", size), error);
			if( not io::detail::check_error(tk.error, error, "libgs::http::response_helper::write") )
				co_return size;
		}
		else
		{
			std::string attributes;
			for(auto &attr : m_chunk_attributes)
				attributes += attr + ";";

			m_chunk_attributes.clear();
			attributes.pop_back();

			size = co_await m_writer(std::format("{:X}; {}\r\n", size, attributes), error);
			if( not io::detail::check_error(tk.error, error, "libgs::http::response_helper::write") )
				co_return size;
		}
		size = co_await m_writer(std::string(static_cast<const char*>(buf), size) + "\r\n", error);
		io::detail::check_error(tk.error, error, "libgs::http::response_helper::write");
		co_return size;
	}

private:
	[[nodiscard]] bool is_chunked() const
	{
		if( stoi32(m_version) < 1.1 )
			return false;

		auto it = m_request_headers.find(header_type::transfer_encoding);
		return it != m_response_headers.end() and str_to_lower(it->second) == key_static_string::chunked;
	}

public:
	str_view_type m_version = key_static_string::v_1_1;
	const headers_type &m_request_headers;

	http::status m_status = status::ok;
	headers_type m_response_headers {
		{ header_type::content_type, header_value_static_string::content_type }
	};
	cookies_type m_cookies;
	value_list_type m_chunk_attributes;

	str_type m_redirect_url;
	write_callback m_writer {};

	bool m_chunk_end_writed = false;
	bool m_headers_writed = false;
};

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(str_view_type version, const headers_type &headers) :
	m_impl(new impl(version, headers))
{

}

template <concept_char_type CharT>
basic_response_helper<CharT>::~basic_response_helper()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_response_helper<CharT>::basic_response_helper(basic_response_helper &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::operator=(basic_response_helper &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_status(uint32_t status)
{
	status_check(status);
	m_impl->m_status = static_cast<http::status>(status);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_status(http::status status)
{
	status_check(status);
	m_impl->m_status = status;
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_header(str_view_type key, value_type value) noexcept
{
	m_impl->m_response_headers[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_cookie(str_view_type key, cookie_type cookie) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(cookie);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_redirect(str_view_type url, redirect_type type)
{
	switch(type)
	{
#define X_MACRO(e,v) case redirect_type::e : set_status(v); break;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
		default: throw runtime_error("libgs::http::response_helper::redirect: Invalid redirect type: '{}'.", type);
	}
	return set_header(header_type::location, {url.data(), url.size()});
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attribute(value_type attribute)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	m_impl->m_chunk_attributes.emplace_back(std::move(attribute));
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::set_chunk_attributes(value_list_type attributes)
{
	if( stof(version()) < 1.1 )
		throw runtime_error("libgs::http::response_helper::set_chunk_attribute: Only HTTP/1.1 supports 'Transfer-Coding: chunked'.");

	for(auto &value : attributes)
		m_impl->m_chunk_attributes.emplace_back(std::move(value));
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::on_write(write_callback writer) const
{
	m_impl->m_writer = std::move(writer);
	return *this;
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(opt_token<error_code&> tk) const
{
	if( m_impl->m_headers_writed )
		throw runtime_error("libgs::http::response_helper::write: The http protocol header is sent repeatedly.");
	co_return co_await write(std::format("{} ({})", to_status_description(status()), status()), std::move(tk));
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(buffer<const void*> body, opt_token<error_code&> tk) const
{
	if( not m_impl->m_writer )
		throw runtime_error("libgs::http::response_helper::write: No writer");

	else if( m_impl->m_headers_writed )
	{
		if( body.size == 0 )
			throw runtime_error("libgs::http::response_helper::write: The http protocol header is sent repeatedly.");
		co_return co_await m_impl->write_body(body.data, body.size, std::move(tk));
	}
	auto res = co_await m_impl->write_header(body.size, tk);
	if( tk.error and *tk.error )
		co_return res;
	else if( body.size > 0 )
		co_return res + (co_await m_impl->write_body(body.data, body.size, std::move(tk)));
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(buffer<const std::string&> body, opt_token<error_code&> tk) const
{
	co_return co_await write({body.data, body.size}, std::move(tk));
}

template <concept_char_type CharT>
awaitable<size_t> basic_response_helper<CharT>::write(std::ifstream &file, opt_token<error_code&> tk) const
{
	if( not m_impl->m_writer )
		throw runtime_error("libgs::http::response_helper::write: No writer");
	// TODO ...
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_response_helper<CharT>::version() const
{
	m_impl->m_version;
}

template <concept_char_type CharT>
http::status basic_response_helper<CharT>::status() const
{
	m_impl->m_status;
}

template <concept_char_type CharT>
const typename basic_response_helper<CharT>::headers_type &basic_response_helper<CharT>::headers() const
{
	m_impl->m_response_headers;
}

template <concept_char_type CharT>
const typename basic_response_helper<CharT>::cookies_type &basic_response_helper<CharT>::cookies() const
{
	m_impl->m_cookies;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_header(str_view_type key)
{
	m_impl->m_response_headers.erase(key);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_cookie(str_view_type key)
{
	m_impl->m_cookies.erase(key);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::unset_chunk_attribute(value_type key)
{
	m_impl->m_chunk_attributes.erase(key);
	return *this;
}

template <concept_char_type CharT>
basic_response_helper<CharT> &basic_response_helper<CharT>::reset()
{
	m_impl->m_version = detail::_key_static_string<CharT>::v_1_1;
	m_impl->m_status = status::ok;

	m_impl->m_response_headers = {
		{ header_type::content_type, detail::_header_value_static_string<CharT>::content_type }
	};
	m_impl->m_cookies.clear();
	m_impl->m_chunk_attributes.clear();
	m_impl->m_redirect_url.clear();

	m_impl->m_chunk_end_writed = false;
	m_impl->m_headers_writed = false;
	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_RESPONSE_HELPER_H