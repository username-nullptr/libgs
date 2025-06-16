#include "request_arg.h"

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN request_arg::impl
{
public:
	explicit impl(url_t &&url) :
		m_url(std::move(url)) {}

	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	url_t m_url;
	headers_t m_headers {{
		header_t::content_type,
		"text/plain; charset=utf-8"
	}};
	cookies_t m_cookies {};
	std::set<value_t> m_chunk_attributes {};
};

request_arg::request_arg(url_t url) :
	m_impl(new impl(std::move(url)))
{

}

request_arg::request_arg() :
	request_arg(url_t())
{

}

request_arg::~request_arg()
{
	delete m_impl;
}

request_arg::request_arg(const request_arg &other) noexcept :
	m_impl(new impl(*other.m_impl))
{

}

request_arg &request_arg::operator=(const request_arg &other) noexcept
{
	*m_impl = *other.m_impl;
	return *this;
}

request_arg::request_arg(request_arg &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl({});
}

request_arg &request_arg::operator=(request_arg &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl({});
	return *this;
}

request_arg &request_arg::set_url(url_t url)
{
	m_impl->m_url = std::move(url);
	return *this;
}

const url &request_arg::url() const noexcept
{
	return m_impl->m_url;
}

url &request_arg::url() noexcept
{
	return m_impl->m_url;
}

const headers &request_arg::headers() const noexcept
{
	return m_impl->m_headers;
}

headers &request_arg::headers() noexcept
{
	return m_impl->m_headers;
}

const cookie_values &request_arg::cookies() const noexcept
{
	return m_impl->m_cookies;
}

cookie_values &request_arg::cookies() noexcept
{
	return m_impl->m_cookies;
}

request_arg &request_arg::set_chunk_attribute(value_t attr) noexcept
{
	if( auto [it, inserted] = m_impl->m_chunk_attributes.emplace(std::move(attr)); not inserted )
	{
		m_impl->m_chunk_attributes.erase(it);
		m_impl->m_chunk_attributes.emplace(std::move(attr));
	}
	return *this;
}

request_arg &request_arg::unset_chunk_attribute(const value_t &attr) noexcept
{
	m_impl->m_chunk_attributes.erase(attr);
	return *this;
}

const std::set<value> &request_arg::chunk_attributes() const noexcept
{
	return m_impl->m_chunk_attributes;
}

std::set<value> &request_arg::chunk_attributes() noexcept
{
	return m_impl->m_chunk_attributes;
}

} //namespace libgs::http::protocol