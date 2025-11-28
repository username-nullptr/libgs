
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_PARSER_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_PARSER_H

namespace libgs::http::protocol
{

template <typename Opt>
auto parser<model::base>::make_file_opt_token(Opt &&opt)
	noexcept requires file_opt_token_v<Opt>
{
	using opt_t = std::remove_cvref_t<Opt>;
	if constexpr( is_any_string_v<opt_t> or is_fstream_v<opt_t,char> or is_ofstream_v<opt_t,char> )
	{
		using token_t = decltype(http::make_file_opt_token(std::forward<Opt>(opt)));
		using type = token_t::type;
		return make_file_opt_token (
			http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt))
		);
	}
	else if constexpr( Opt::optype == file_optype::single )
	{
		using type = opt_t::type;
		return make_file_opt_token (
			http::file_opt_token<type,file_optype::multiple>(std::forward<Opt>(opt))
		);
	}
	else
		return opt.init(std::ios::out | std::ios::binary);
}

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_DETAIL_PARSER_H