
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

#ifndef LIBGS_HTTP_HEADER_H
#define LIBGS_HTTP_HEADER_H

#include <libgs/http/global.h>
#include <libgs/core/value.h>

namespace libgs::http
{

struct header
{
static constexpr const char
	* accept_language   = "Accept-Language"  ,
	* accept_encoding   = "Accept-Encoding"  ,
	* accept_ranges     = "Accept-Ranges"    ,
	* accept            = "Accept"           ,
	* age               = "Age"              ,
	* content_encoding  = "Content-Encoding" ,
	* content_length    = "Content-Length"   ,
	* cache_control     = "Cache-Control"    ,
	* content_range     = "Content-Range"    ,
	* content_type      = "Content-Type"     ,
	* connection        = "Connection"       ,
	* expires           = "Expires"          ,
	* host              = "Host"             ,
	* last_modified     = "Last-Modified"    ,
	* location          = "Location"         ,
	* origin            = "Origin"           ,
	* referer           = "Referer"          ,
	* range             = "Range"            ,
	* transfer_encoding = "Transfer-Encoding",
	* user_agent        = "User-Agent"       ,
	* upgrade           = "Upgrade"          ;
};

using headers = map<value>;

} //namespace libgs::http


#endif //LIBGS_HTTP_HEADER_H