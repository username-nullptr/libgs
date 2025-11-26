
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_TYPES_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_TYPES_H

#include <libgs/http/protocol/types.h>

namespace libgs::http::protocol
{

template <model>
class generator {};

enum class generator_state {
	header, content_length, chunk, finish
};

template <model>
class parser {};

#define LIBGS_HTTP_PARSER_ERRNO \
X_MACRO( RLTL  , 10000 , "Request line too long."      ) \
X_MACRO( HLTL  , 10001 , "Header line too long."       ) \
X_MACRO( IREQL , 10002 , "Invalid request line."       ) \
X_MACRO( IRPYL , 10003 , "Invalid reply line."         ) \
X_MACRO( IHM   , 10004 , "Invalid http method."        ) \
X_MACRO( IHP   , 10005 , "Invalid http path."          ) \
X_MACRO( IHSC  , 10006 , "Invalid http status code."   ) \
X_MACRO( IHL   , 10007 , "Invalid header line."        ) \
X_MACRO( ICL   , 10008 , "Invalid cookie line."        ) \
X_MACRO( IDE   , 10009 , "The inserted data is empty." ) \
X_MACRO( SFE   , 10010 , "Size format error."          ) \
X_MACRO( RE    , 10011 , "This request is ended."      )

enum class parse_errno
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
};

enum class stage {
	header, body, finished
};

} //namespace libgs::http::protocol


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_TYPES_H