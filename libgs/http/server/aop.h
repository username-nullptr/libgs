
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

#ifndef LIBGS_HTTP_SERVER_AOP_H
#define LIBGS_HTTP_SERVER_AOP_H

#include <libgs/http/server/context.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class basic_aop
{
public:
	using context_type = basic_service_context<CharT,Exec>;
	virtual ~basic_aop() = 0;
	virtual awaitable<bool> before(context_type &context);
	virtual awaitable<bool> after(context_type &context);
	virtual bool exception(context_type &context, std::exception &ex);
};

using aop = basic_aop<char>;
using waop = basic_aop<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
using basic_aop_ptr = std::shared_ptr<basic_aop<CharT,Exec>>;

using aop_ptr = basic_aop_ptr<char>;
using waop_ptr = basic_aop_ptr<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class basic_ctrlr_aop : public basic_aop<CharT, Exec>
{
public:
	using context_type = basic_service_context<CharT,Exec>;
	virtual awaitable<void> service(context_type &context) = 0;
};

using ctrlr_aop = basic_ctrlr_aop<char>;
using wctrlr_aop = basic_ctrlr_aop<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
using basic_ctrlr_aop_ptr = std::shared_ptr<basic_ctrlr_aop<CharT,Exec>>;

using ctrlr_aop_ptr = basic_ctrlr_aop_ptr<char>;
using wctrlr_aop_ptr = basic_ctrlr_aop_ptr<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/aop.h>


#endif //LIBGS_HTTP_SERVER_AOP_H
