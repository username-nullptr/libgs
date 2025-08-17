#include "generator.h"
#include <libgs/core/algorithm/misc.h>

namespace libgs::http::protocol
{

class LIBGS_DECL_HIDDEN generator<model::client>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	impl() = default;
	explicit impl(version_enum version, request_arg_t request) :
		m_req_arg(std::move(request))
	{
		version_t::check(version);
		if( version == version::v10 )
			m_generator = std::make_shared<base_generator_v10>();
		else if( version == version::v11 )
			m_generator = std::make_shared<base_generator_v11>();
		// else ... ...
	}

	next_layer_t m_generator;
	request_arg_t m_req_arg;
	headers_t m_auto_headers;
};

generator<model::client>::generator(request_arg_t request) :
	generator(version_enum::v11, std::move(request))
{

}

generator<model::client>::generator(version_enum version, request_arg_t request) :
	m_impl(new impl(version, std::move(request)))
{

}

generator<model::client>::~generator()
{
	delete m_impl;
}

generator<model::client>::generator(generator &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

generator<model::client> &generator<model::client>::operator=(generator &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

generator<model::client> &generator<model::client>::set_arg(request_arg_t arg)
{
	m_impl->m_req_arg = std::move(arg);
	return *this;
}

const request_arg &generator<model::client>::arg() const noexcept
{
	return m_impl->m_req_arg;
}

request_arg &generator<model::client>::arg() noexcept
{
	return m_impl->m_req_arg;
}

std::string generator<model::client>::header_data(method_enum method, size_t body_size)
{
	if( m_impl->m_generator->state() != generator_state::header )
		return {};

	auto &arg = this->arg();
	auto &url = arg.url();

	auto buf = std::string(method::string(method)) + " ";
	{
		std::string path(url.path());
		if( not url.parameters().empty() )
		{
			path += "?";
			std::string params;

			for(auto &[key,value] : url.parameters())
				params += key + "=" + *value + "&";

			params.pop_back();
			path += to_percent_encoding(params);
		}
		buf += path + " HTTP/"
			+ version::string(m_impl->m_generator->version())
			+ "\r\n";
	}
	for(auto &[key,value] : arg.headers())
		m_impl->m_generator->set_header(key, value);

	buf += m_impl->m_generator->header_data(body_size);
	if( not arg.cookies().empty() )
	{
		buf += "Cookie: ";
		for(auto &[key,value] : arg.cookies())
			buf += key + "=" + *value + ";";
		buf += "\r\n";
	}
	return buf + "\r\n";
}

std::string generator<model::client>::body_data(const const_buffer &buffer)
{
	return m_impl->m_generator->body_data(buffer);
}

std::string generator<model::client>::chunk_end_data(const headers_t &headers)
{
	return m_impl->m_generator->chunk_end_data(headers);
}

version_enum generator<model::client>::version() const noexcept
{
	return m_impl->m_generator->version();
}

generator<model::client> &generator<model::client>::reset() noexcept
{
	m_impl->m_auto_headers.clear();
	m_impl->m_generator.reset();
	return *this;
}

} //namespace libgs::http::protocol