
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H

namespace libgs::http
{

namespace detail
{

template <concept_char_type T>
struct _request_helper_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *ampersand  = __VA_ARGS__##"&"; \
	static constexpr const _type *assignment = __VA_ARGS__##"=";

template <>
struct _request_helper_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _request_helper_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <concept_char_type CharT>
class basic_request_helper<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using string_list_t = basic_string_list<CharT>;
	struct string_pool : detail::string_pool<CharT>, detail::_request_helper_static_string<CharT> {};

public:
	impl(string_view_t version)
	{
		if( version.empty() )
			m_version = string_pool::v_1_1;
		else
			m_version = string_t(version.data(), version.size());
	}

public:
	void set_path(string_t path) noexcept
	{
		auto resource_line = str_trimmed(path);
		auto pos = resource_line.find('?');

		if( pos == string_t::npos )
		{
			m_path = std::move(resource_line);
			return ;
		}
		m_path = resource_line.substr(0, pos);
		auto parameters_string = resource_line.substr(pos + 1);

		for(auto &para_str : string_list_t::from_string(parameters_string, string_pool::ampersand))
		{
			pos = para_str.find(string_pool::assignment);
			if( pos == string_t::npos )
				m_parameters.emplace(mbstoxx<CharT>(para_str), mbstoxx<CharT>(para_str));
			else
				m_parameters.emplace(mbstoxx<CharT>(para_str.substr(0, pos)), mbstoxx<CharT>(para_str.substr(pos+1)));
		}
	}

	std::string header_data(size_t body_size)
	{
		auto it = m_headers.find(header_t::content_length);
		if( it == m_headers.end() )
		{
			if( stoi32(m_version) > 1.0 )
			{
				it = m_headers.find(header_t::transfer_encoding);
				if( it == m_headers.end() or str_to_lower(it->second.to_string()) != string_pool::chunked )
					set_header(header_t::content_length, body_size);
			}
			else
				set_header(header_t::content_length, body_size);
		}
		auto buf = to_method_string(m_method) + " ";
		auto url = xxtombs<CharT>(m_path);

		if( not m_parameters.empty() )
		{
			url += "?";
			for(auto &[key,value] : m_parameters)
				url += xxtombs<CharT>(key) + "=" + xxtombs<CharT>(value) + "&";
		}
		buf += to_percent_encoding(url) + " HTTP/" + m_version + "\r\n";
		for(auto &[key,value] : m_headers)
			buf += xxtombs<CharT>(key) + ": " + xxtombs<CharT>(value) + "\r\n";

		if( not m_cookies.empty() )
		{
			buf += "Cookie: ";
			for(auto &[key,value] : m_cookies)
				buf += xxtombs<CharT>(key) + "=" + xxtombs<CharT>(value) + ";";
			buf += "\r\n";
		}
		return buf + "\r\n";
	}

	void set_header(string_view_t key, value_t value) noexcept {
		m_headers[str_to_lower(key)] = std::move(value);
	}

public:
	string_t m_version;
	string_t m_path = string_pool::root;
	parameters_t m_parameters {};
	http::method m_method = http::method::GET;
	headers_t m_headers {};
	cookies_t m_cookies {};
};

template <concept_char_type CharT>
basic_request_helper<CharT>::basic_request_helper(string_view_t version) :
		m_impl(new impl(version))
{

}

template <concept_char_type CharT>
basic_request_helper<CharT>::~basic_request_helper()
{
	delete m_impl;
}

template <concept_char_type CharT>
basic_request_helper<CharT>::basic_request_helper(basic_request_helper &&other) noexcept :
		m_impl(other.m_impl)
{
	other.m_impl = new impl({});
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::operator=(basic_request_helper &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl({});
	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::set_path(value_t path) noexcept
{
	m_impl->set_path(std::move(path.to_string()));
	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::set_parameter(string_view_t key, value_t value) noexcept
{
	m_impl->m_parameters[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::set_method(http::method method)
{
	method_check(method);
	m_impl->m_method = method;
	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::set_header(string_view_t key, value_t value) noexcept
{
	m_impl->set_header(key, std::move(value));
	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::set_cookie(string_view_t key, value_t value) noexcept
{
	m_impl->m_cookies[str_to_lower(key)] = std::move(value);
	return *this;
}

template <concept_char_type CharT>
std::string basic_request_helper<CharT>::header_data(size_t body_size)
{
	return m_impl->header_data(body_size);
}

template <concept_char_type CharT>
std::string basic_request_helper<CharT>::body_data(const const_buffer &buffer)
{

}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_request_helper<CharT>::version() const noexcept
{

}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_request_helper<CharT>::path() const noexcept
{

}

template <concept_char_type CharT>
http::method basic_request_helper<CharT>::method() const noexcept
{

}

template <concept_char_type CharT>
const basic_headers<CharT> &basic_request_helper<CharT>::headers() const noexcept
{

}

template <concept_char_type CharT>
const basic_cookie_values<CharT> &basic_request_helper<CharT>::cookies() const noexcept
{

}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::unset_header(string_view_t key)
{

	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::unset_cookie(string_view_t key)
{

	return *this;
}

template <concept_char_type CharT>
basic_request_helper<CharT> &basic_request_helper<CharT>::reset()
{

	return *this;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_HELPER_H
