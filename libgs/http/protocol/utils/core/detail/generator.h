
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

#ifndef LIBGS_HTTP_TOOLS_CORE_DETAIL_GENERATOR_H
#define LIBGS_HTTP_TOOLS_CORE_DETAIL_GENERATOR_H

#include <libgs/core/algorithm/uuid.h>

namespace libgs::http::protocol
{

template <typename Opt>
sys_expected<body_norms_t> generator<model::base>::set_header(Opt &&opt)
	noexcept requires file_opt_token_v<Opt>
{
	auto token = make_file_opt_token(std::forward<decltype(opt)>(opt));
	if( not token )
		return sys_unexpected(token.error());

	body_norms_t mode;
	if( token->ranges.empty() )
	{
		set_header(header_t::content_length, token->file_size);
		set_header(header_t::content_type  , token->mime_type);
		mode = basic_body_norms();
	}
	else if( token.ranges.size() == 1 )
	{
		auto &range = token->ranges.front();
		auto end = range.begin + range.total - 1;

		if( range.total == 0 or end >= token->file_size )
		{
			return sys_unexpected (
				std::make_error_code(std::errc::invalid_seek)
			);
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
				return sys_unexpected (
					std::make_error_code(std::errc::invalid_seek)
				);
			}
			auto &package = norms.packages.emplace_back();
			package.range = range;

			auto cr_line = std::format("{}: bytes {}-{}/{}",
				header_t::content_range, range.begin, end, token->file_size
			);
			package.headers.emplace(ct_line);
			package.headers.emplace(cr_line);

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
	return mode;
}

template <typename Opt>
auto generator<model::base>::make_file_opt_token(Opt &&opt)
	noexcept requires file_opt_token_v<Opt>
{
	using opt_t = std::remove_cvref_t<Opt>;
	if constexpr( is_any_string_v<opt_t> or is_fstream_v<opt_t,char> or is_ifstream_v<opt_t,char> )
	{
		using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
		using type = token_t::type;
		return make_file_opt_token (
			http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt))
		);
	}
	else if constexpr( opt_t::optype == file_optype::single )
	{
		using type = opt_t::type;
		return make_file_opt_token (
			http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt))
		);
	}
	else
		return opt.init(std::ios::in | std::ios::binary);
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_TOOLS_CORE_DETAIL_GENERATOR_H