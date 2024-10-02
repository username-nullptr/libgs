
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_URL_H
#define LIBGS_HTTP_CLIENT_DETAIL_URL_H

namespace libgs::http
{

namespace detail
{

template <core_concepts::char_type T>
struct _url_static_string;

#define LIBGS_HTTP_DETAIL_STRING_POOL(_type, ...) \
	static constexpr const _type *url_header = __VA_ARGS__##"http"      ; \
	static constexpr const _type *pro_http   = url_header               ; \
	static constexpr const _type *pro_https  = __VA_ARGS__##"https"     ; \
	static constexpr const _type *local_addr = __VA_ARGS__##"127.0.0.1" ; \
	static constexpr const _type *question   = __VA_ARGS__##"?"         ; \
	static constexpr const _type *ampersand  = __VA_ARGS__##"&"         ; \
	static constexpr const _type *assignment = __VA_ARGS__##"="         ; \
	static constexpr const _type *pro_sptr   = __VA_ARGS__##"://"       ;

template <>
struct _url_static_string<char> {
	LIBGS_HTTP_DETAIL_STRING_POOL(char);
};

template <>
struct _url_static_string<wchar_t> {
	LIBGS_HTTP_DETAIL_STRING_POOL(wchar_t,L);
};

#undef LIBGS_HTTP_DETAIL_STRING_POOL

} //namespace detail

template <core_concepts::char_type CharT>
class basic_url<CharT>::impl
{
	LIBGS_DISABLE_MOVE(impl)
	using string_list_t = basic_string_list<CharT>;
	struct string_pool : detail::string_pool<CharT>, detail::_url_static_string<CharT> {};

public:
	explicit impl(string_view_t url = string_pool::root) {
		set(url);
	}
	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	void set(string_view_t url)
	{
		if( url.empty() )
			return ;
		auto resource_line = str_trimmed(url);
		if( resource_line.size() >= 8 )
		{
			if( str_to_lower(resource_line.substr(0,4)) == string_pool::url_header )
			{
				if( resource_line[4] == 0x73/*s*/ and resource_line.size() > 8 )
				{
					if( resource_line[6] == 0x2F/*/*/ and resource_line[7] == 0x2F/*/*/ )
					{
#ifndef LIBGS_ENABLE_OPENSSL
						throw runtime_error("libgs::http::url: SSL is not supported. (No defined LIBGS_ENABLE_OPENSSL)");
#endif //LIBGS_ENABLE_OPENSSL
						m_protocol = string_pool::pro_https;
						resource_line = resource_line.substr(8);
					}
				}
				else if( resource_line[4] == 0x3A/*:*/ )
				{
					if( resource_line[5] == 0x2F/*/*/ and resource_line[6] == 0x2F/*/*/ )
					{
						m_protocol = string_pool::pro_http;
						resource_line = resource_line.substr(7);
					}
				}
			}
		}
		string_t addpth;
		auto pos = resource_line.find(string_pool::question);
		if( pos == string_t::npos )
			addpth = std::move(resource_line);
		else
		{
			addpth = resource_line.substr(0,pos);
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
		pos = addpth.find(string_pool::root);
		if( pos == string_t::npos )
		{
			m_address = std::move(addpth);
			m_path = string_pool::root;
		}
		else
		{
			m_address = addpth.substr(0,pos);
			m_path = addpth.substr(pos);
		}
		if( m_address.empty() )
			m_address = string_pool::local_addr;
		else
		{
			pos = m_address.rfind(":");
			if( pos == string_t::npos )
				m_port = m_protocol == string_pool::pro_https ? 443 : 80;
			else
			{
				m_port = stoui16(m_address.substr(pos+1));
				m_address = m_address.substr(0,pos);
			}
		}
	}

public:
	string_t m_protocol = string_pool::pro_http;
	string_t m_path = string_pool::root;
	string_t m_address = string_pool::local_addr;
	uint16_t m_port = 80;
	parameters_t m_parameters {};
};

template <core_concepts::char_type CharT>
template <typename Arg0, typename...Args>
basic_url<CharT>::basic_url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args) :
	basic_url(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...))
{

}

template <core_concepts::char_type CharT>
basic_url<CharT>::basic_url(string_view_t url) :
	m_impl(new impl(url))
{

}

template <core_concepts::char_type CharT>
basic_url<CharT>::basic_url() :
	basic_url(string_view_t())
{

}

template <core_concepts::char_type CharT>
basic_url<CharT>::~basic_url()
{
	delete m_impl;
}

template <core_concepts::char_type CharT>
basic_url<CharT>::basic_url(const basic_url &other) :
	m_impl(new impl(*other.m_impl))
{

}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::operator=(const basic_url &other) 
{
	m_impl = new impl(*other.m_impl);
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT>::basic_url(basic_url &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::operator=(basic_url &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <core_concepts::char_type CharT>
template <typename Arg0, typename...Args>
basic_url<CharT> &basic_url<CharT>::set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args)
{
	m_impl->set(std::format(fmt, std::forward<Arg0>(arg0), std::forward<Args>(args)...));
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::set(string_view_t url)
{
	m_impl->set(url);
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::set_address(string_view_t addr)
{
	m_impl->m_address = string_t(addr.data(), addr.size());
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::set_port(uint16_t port)
{
	m_impl->m_port = port;
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::set_path(string_view_t path)
{
	m_impl->set(path);
	return *this;
}

template <core_concepts::char_type CharT>
basic_url<CharT> &basic_url<CharT>::set_parameter(string_view_t key, value_t value) noexcept
{
	m_impl->m_parameters[string_t(key.data(), key.size())] = std::move(value);
	return *this;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_url<CharT>::protocol() const noexcept
{
	return m_impl->m_protocols;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_url<CharT>::address() const noexcept
{
	return m_impl->m_address;
}

template <core_concepts::char_type CharT>
uint16_t basic_url<CharT>::port() const noexcept
{
	return m_impl->m_port;
}

template <core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_url<CharT>::path() const noexcept
{
	return m_impl->m_path;
}

template <core_concepts::char_type CharT>
const basic_parameters<CharT> &basic_url<CharT>::parameter() const noexcept
{
	return m_impl->m_parameters;
}

template <core_concepts::char_type CharT>
std::basic_string<CharT> basic_url<CharT>::to_string() const noexcept
{
	using sp = detail::_url_static_string<CharT>;
	string_t buf = m_impl->m_protocol + sp::pro_sptr + m_impl->m_path + std::format(default_format_v<CharT>, m_impl->m_port);

	if( m_impl->m_parameters.empty() )
		return buf;

	buf += sp::question;
	for(auto &[key,value] : m_impl->m_parameters)
		buf += key + sp::assignment + value.to_string() + sp::ampersand;
	buf.pop_back();
	return buf;
}

template <core_concepts::char_type CharT>
basic_url<CharT>::operator std::basic_string<CharT>() const noexcept
{
	return to_string();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_URL_H
