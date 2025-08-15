#include "url.h"

#include "libgs/core/string_vector.h"

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN url::impl
{
	LIBGS_DISABLE_MOVE(impl)

public:
	explicit impl(std::string_view url = "/") {
		set(url);
	}
	impl(const impl &other) = default;
	impl &operator=(const impl &other) = default;

public:
	void set(std::string_view url)
	{
		if( url.empty() )
			return ;
		auto resource_line = strtls::trimmed(url);
		if( resource_line.size() >= 8 )
		{
			if( strtls::to_lower(resource_line.substr(0,4)) == "http" )
			{
				if( resource_line[4] == 's' and resource_line.size() > 8 )
				{
					if( resource_line[5] == ':' and resource_line[6] == '/' and resource_line[7] == '/' )
					{
						m_protocol = "https";
						resource_line = resource_line.substr(8);
					}
				}
				else if( resource_line[4] == ':' )
				{
					if( resource_line[5] == '/' and resource_line[6] == '/' )
					{
						m_protocol = "http";
						resource_line = resource_line.substr(7);
					}
				}
			}
		}
		std::string addpth;
		auto pos = resource_line.find("?");
		if( pos == std::string::npos )
			addpth = std::move(resource_line);
		else
		{
			addpth = resource_line.substr(0,pos);
			auto parameters_string = resource_line.substr(pos + 1);

			for(auto &para_str : string_vector::from_string(parameters_string, "&"))
			{
				pos = para_str.find("=");
				if( pos == std::string::npos )
				{
					auto key = para_str;
					m_parameters.emplace(std::move(key), std::move(para_str));
				}
				else
					m_parameters.emplace(para_str.substr(0, pos), para_str.substr(pos+1));
			}
		}
		pos = addpth.find("/");
		if( pos == std::string::npos )
		{
			m_address = std::move(addpth);
			m_path = "/";
		}
		else
		{
			m_address = addpth.substr(0,pos);
			m_path = addpth.substr(pos);
		}
		if( m_address.empty() )
			m_address = "127.0.0.1";
		else
		{
			pos = m_address.rfind(":");
			if( pos == std::string::npos )
				m_port = m_protocol == "https" ? 443 : 80;
			else
			{
				m_port = strtls::to_uint16(m_address.substr(pos+1));
				m_address = m_address.substr(0,pos);
			}
		}
	}

public:
	std::string m_protocol = "http";
	std::string m_path = "/";
	std::string m_address = "127.0.0.1";
	uint16_t m_port = 80;
	parameters_t m_parameters {};
};

url::url(std::string_view url) :
	m_impl(new impl(url))
{

}

url::url() : url(std::string_view())
{

}

url::~url()
{
	delete m_impl;
}

url::url(const url &other) :
	m_impl(new impl(*other.m_impl))
{

}

url &url::operator=(const url &other)
{
	m_impl = new impl(*other.m_impl);
	return *this;
}

url::url(url &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

url &url::operator=(url &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

url &url::set(std::string_view url)
{
	m_impl->set(url);
	return *this;
}

url &url::set_address(std::string addr)
{
	m_impl->m_address = std::move(addr);
	return *this;
}

url &url::set_port(uint16_t port)
{
	m_impl->m_port = port;
	return *this;
}

url &url::set_path(std::string_view path)
{
	m_impl->set(path);
	return *this;
}

std::string_view url::protocol() const noexcept
{
	return m_impl->m_protocol;
}

std::string_view url::address() const noexcept
{
	return m_impl->m_address;
}

uint16_t url::port() const noexcept
{
	return m_impl->m_port;
}

std::string_view url::path() const noexcept
{
	return m_impl->m_path;
}

const parameters &url::parameters() const noexcept
{
	return m_impl->m_parameters;
}

parameters &url::parameters() noexcept
{
	return m_impl->m_parameters;
}

std::string url::to_string() const noexcept
{
	auto buf =
		m_impl->m_protocol + "://" + m_impl->m_path +
		std::format("{}", m_impl->m_port);

	if( m_impl->m_parameters.empty() )
		return buf;

	buf += "?";
	for(auto &[key,value] : m_impl->m_parameters)
		buf += key + "=" + value.to_string() + "&";
	buf.pop_back();
	return buf;
}

url::operator std::string() const noexcept
{
	return to_string();
}

} //namespace libgs::http::protocol