
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_AOP_H
#define LIBGS_HTTP_SERVER_DETAIL_AOP_H

namespace libgs::http
{

template <typename Stream, concept_char_type CharT>
basic_aop<Stream,CharT>::~basic_aop() = default;

template <typename Stream, concept_char_type CharT>
awaitable<bool> basic_aop<Stream,CharT>::before(context_t &context)
{
	ignore_unused(context);
	co_return false;
}

template <typename Stream, concept_char_type CharT>
awaitable<bool> basic_aop<Stream,CharT>::after(context_t &context)
{
	ignore_unused(context);
	co_return false;
}

template <typename Stream, concept_char_type CharT>
bool basic_aop<Stream,CharT>::exception(context_t &context, std::exception &ex)
{
	ignore_unused(context, ex);
	return false;
}

namespace detail
{

template <typename Stream, typename CharT, typename...Args>
concept concept_aop_ptr_list = requires(Args&&...args) {
	std::vector<basic_aop_ptr<Stream,CharT>> { basic_aop_ptr<Stream,CharT>(std::forward<Args>(args))... };
};

template <typename Stream, typename CharT, typename...Args>
concept concept_ctrlr_aop_ptr_list = requires(Args&&...args) {
	std::vector<basic_ctrlr_aop_ptr<Stream,CharT>> { basic_ctrlr_aop_ptr<Stream,CharT>(std::forward<Args>(args))... };
};

template <typename Func, typename Stream, typename CharT>
concept concept_request_handler = requires(Func &&func, basic_service_context<Stream,CharT> &context) {
	std::is_same_v<awaitable_return_type_t<decltype(func(context))>,void>;
};

}} //namespace libgs::http::detail


#endif //LIBGS_HTTP_SERVER_DETAIL_AOP_H
