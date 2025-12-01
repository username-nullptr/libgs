
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H

#include <libgs/core/algorithm/uuid.h>

namespace libgs::http::protocol
{

template <typename Derived>
const_parameters<Derived>::const_parameters(const parameters_t *parameters) :
	m_parameters(parameters)
{

}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it == parameters().end() )
		return nullopt;
	return it->second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const core_concepts::text_p<char> auto &key, const value_t &value) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it != parameters().end() )
		return it->second == value;
	return false;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	return it != parameters().end();
}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(size_t index) const
{
	if( not contains_parameter(index) )
		runtime_error::loc_throw("index out of range.");
	return parameters()[index].second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter(size_t index) const noexcept
{
	return index >= parameters().size();
}

template <typename Derived>
const const_parameters<Derived>::parameters_t&
const_parameters<Derived>::parameters() const noexcept
{
	return *m_parameters;
}

template <typename Derived>
const_headers<Derived>::const_headers(const headers_t *headers) :
	m_headers(headers)
{

}

template <typename Derived>
optional<typename const_headers<Derived>::value_t>
const_headers<Derived>::header(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	if( it == headers().end() )
		return nullopt;
	return it->second;
}

template <typename Derived>
bool const_headers<Derived>::contains_header
(const core_concepts::text_p<char> auto &key, const value_t &value) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	if( it != headers().end() )
		return it->second == value;
	return false;
}

template <typename Derived>
bool const_headers<Derived>::contains_header
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = headers().find(strtls::to_string(key));
	return it != headers().end();
}

template <typename Derived>
const const_headers<Derived>::headers_t&
const_headers<Derived>::headers() const noexcept
{
	return *m_headers;
}

template <typename Cookie, typename Derived>
const_cookies<Cookie,Derived>::const_cookies(const cookies_t *cookies) :
	m_cookies(cookies)
{

}

template <typename Cookie, typename Derived>
optional<typename const_cookies<Cookie,Derived>::cookie_t>
const_cookies<Cookie,Derived>::cookie(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = cookies().find(strtls::to_string(key));
	if( it == cookies().end() )
		return nullopt;
	return it->second;
}

template <typename Cookie, typename Derived>
bool const_cookies<Cookie,Derived>::contains_cookie
(const core_concepts::text_p<char> auto &key) const noexcept
{
	auto it = cookies().contains(strtls::to_string(key));
	return it != cookies().end();
}

template <typename Cookie, typename Derived>
const const_cookies<Cookie,Derived>::cookies_t&
const_cookies<Cookie,Derived>::cookies() const noexcept
{
	return *m_cookies;
}

template <typename Derived>
const_chunk_attributes<Derived>::const_chunk_attributes(const values_t *chunk_attributes) :
	m_chunk_attributes(chunk_attributes)
{

}

template <typename Derived>
bool const_chunk_attributes<Derived>::contains_chunk_attribute(const value_t &attr) const noexcept
{
	return chunk_attributes().find(attr) != chunk_attributes().end();
}

template <typename Derived>
const const_chunk_attributes<Derived>::values_t&
const_chunk_attributes<Derived>::chunk_attributes() const noexcept
{
	return *m_chunk_attributes;
}

template <typename Derived>
mutable_parameters<Derived>::base_t::derived_t &mutable_parameters<Derived>::set_parameter
(core_concepts::text_p<char> auto &&key, typename base_t::value_t value) noexcept
{
	parameters()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_parameters<Derived>::base_t::derived_t &mutable_parameters<Derived>::unset_parameter
(const core_concepts::text_p<char> auto &key) noexcept
{
	parameters().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_parameters<Derived>::base_t::parameters_t&
mutable_parameters<Derived>::parameters() noexcept
{
	return remove_const(*this->m_parameters);
}

template <typename Derived>
mutable_headers<Derived>::base_t::derived_t &mutable_headers<Derived>::set_header
(core_concepts::text_p<char> auto &&key, typename base_t::value_t value) noexcept
{
	headers()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_headers<Derived>::base_t::derived_t &mutable_headers<Derived>::unset_header
(const core_concepts::text_p<char> auto &key) noexcept
{
	headers().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_headers<Derived>::base_t::headers_t &mutable_headers<Derived>::headers() noexcept
{
	return remove_const(*this->m_headers);
}

template <typename Derived>
template <typename Opt>
auto mutable_headers<Derived>::set_header(Opt &&opt)
	noexcept requires file_opt_token_v<Opt>
{
	auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
	using token_t = std::remove_cvref_t<decltype(*token)>;

	using pair_t = std::pair<body_norms_t,token_t>;
	using expected_t = sys_expected<pair_t>;

	if( not token )
		return expected_t(sys_unexpected(token.error()));

	using header_t = base_t::header_t;
	body_norms_t mode;

	if( token->ranges.empty() )
	{
		set_header(header_t::content_length, token->file_size);
		set_header(header_t::content_type  , token->mime_type);
		mode = basic_body_norms();
	}
	else if( token->ranges.size() == 1 )
	{
		auto &range = token->ranges.front();
		auto end = range.begin + range.total - 1;

		if( range.total == 0 or end >= token->file_size )
		{
			return expected_t(sys_unexpected (
				std::make_error_code(std::errc::invalid_seek)
			));
		}
		set_header(header_t::content_length, range.total     );
     	set_header(header_t::content_type  , token->mime_type);
		set_header(header_t::accept_ranges , "bytes"         );

		set_header(header_t::content_range, value {
			"{}-{}/{}", range.begin, end, range.total
		});
		mode = range;
	}
	else
	{
		multipart_body_norms norms;
		{
			constexpr std::string_view prefix =
				"multipart/byteranges; boundary=";

			auto generate_boundary = [&]
			{
				using namespace std::chrono;
				norms.boundary = std::format("{}_{}",
					uuid::generate().to_string(),
					duration_cast<milliseconds>(
						system_clock::now().time_since_epoch()
					).count()
				);
				set_header(header_t::content_type,
					std::string(prefix) + norms.boundary
				);
			};
			auto it = headers().find(header_t::content_type);
			if( it == headers().end() )
				generate_boundary();
			else
			{
				if( it->second->size() > prefix.size() and it->second->starts_with(prefix) )
					norms.boundary = it->second->substr().substr(prefix.size());
				else
					generate_boundary();
			}
		}
		size_t content_length = 0;
		/*
			--boundary<CR><LF>
			Content-Type: xxx<CR><LF>
			Content-Range: bytes 3-11/96<CR><LF>
			<CR><LF>
			012345678<CR><LF>
			--boundary<CR><LF>
			Content-Type: xxx<CR><LF>
			Content-Range: bytes 0-7/96<CR><LF>
			<CR><LF>
			01235467<CR><LF>
			--boundary--<CR><LF>
		*/
		auto ct_line = std::format("{}: {}",
			header_t::content_type, token->mime_type
		);
		for(auto &range : token->ranges)
		{
			auto end = range.begin + range.total - 1;
			if( range.total == 0 or end >= token->file_size )
			{
				return expected_t(sys_unexpected (
					std::make_error_code(std::errc::invalid_seek)
				));
			}
			auto &package = norms.packages.emplace_back();
			package.range = range;

			auto cr_line = std::format("{}: bytes {}-{}/{}",
				header_t::content_range, range.begin, end, token->file_size
			);
			package.headers.emplace_back(ct_line);
			package.headers.emplace_back(cr_line);

			content_length += 2 + norms.boundary.size() + 2 +  // --boundary<CR><LF>
							  ct_line.size() + 2 +             // Content-Type: xxx<CR><LF>
							  cr_line.size() + 2 +             // Content-Range: bytes 3-11/96<CR><LF>
							  2 +                              // <CR><LF>
							  range.total + 2;                 // 012345678<CR><LF>
		}
		content_length += 2 + norms.boundary.size() + 2 + 2;   // --boundary--<CR><LF>

		set_header(header_t::content_length, content_length);
		set_header(header_t::accept_ranges , "bytes"       );
		mode = norms;
	}
	return expected_t(pair_t (
		std::move(mode), std::move(*token)
	));
}

template <typename Derived>
template <typename Opt>
auto mutable_headers<Derived>::make_file_opt_token(Opt &&opt)
	noexcept requires file_opt_token_v<Opt>
{
	using opt_t = std::remove_cvref_t<Opt>;
	if constexpr( is_any_string_v<opt_t> or is_fstream_v<opt_t,char> or is_ifstream_v<opt_t,char> )
	{
		using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
		using type = token_t::type;

		using res_token_t = file_opt_token<type,file_optype::multiple> ;
		res_token_t token(std::forward<Opt>(opt));

		auto expected = token.init(std::ios::in | std::ios::binary);
		if( expected )
			return sys_expected<res_token_t>(std::move(token));
		return sys_expected<res_token_t>(sys_unexpected(expected.error()));
	}
	else if constexpr( opt_t::optype == file_optype::single )
	{
		using type = opt_t::type;
		using res_token_t = file_opt_token<type,file_optype::multiple> ;

		res_token_t token(std::forward<Opt>(opt));
		auto expected = token.init(std::ios::in | std::ios::binary);

		if( expected )
			return sys_expected<res_token_t>(std::move(token));
		return sys_expected<res_token_t>(sys_unexpected(expected.error()));
	}
	else
	{
		auto expected = opt.init(std::ios::in | std::ios::binary);
		if( expected )
			return sys_expected<opt_t>(std::forward<Opt>(opt));
		return sys_expected<opt_t>(sys_unexpected(expected.error()));
	}
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::derived_t &mutable_cookies<Cookie,Derived>::set_cookie
(core_concepts::text_p<char> auto &&key, typename base_t::cookie_t value) noexcept
{
	cookies()[strtls::to_string(std::forward<decltype(key)>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::derived_t &mutable_cookies<Cookie,Derived>::unset_cookie
(const core_concepts::text_p<char> auto &key) noexcept
{
	cookies().erase(strtls::to_string(key));
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Cookie, typename Derived>
mutable_cookies<Cookie,Derived>::base_t::cookies_t&
mutable_cookies<Cookie,Derived>::cookies() noexcept
{
	return remove_const(*this->m_cookies);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::derived_t&
mutable_chunk_attributes<Derived>::set_chunk_attribute(typename base_t::value_t attr) noexcept
{
	if( auto [it, inserted] = chunk_attributes().emplace(std::move(attr)); not inserted )
	{
		chunk_attributes().erase(it);
		chunk_attributes().emplace(std::move(attr));
	}
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::derived_t&
mutable_chunk_attributes<Derived>::unset_chunk_attribute(const typename base_t::value_t &attr) noexcept
{
	chunk_attributes().erase(attr);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_chunk_attributes<Derived>::base_t::values_t&
mutable_chunk_attributes<Derived>::chunk_attributes() noexcept
{
	return remove_const(*this->m_chunk_attributes);
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_CONTAINER_HELPER_H