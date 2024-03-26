
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

#ifndef LIBGS_CORE_LOG_DETAIL_MQ_H
#define LIBGS_CORE_LOG_DETAIL_MQ_H

namespace libgs::log
{

inline basic_message_node<void>::basic_message_node(output_type type, context_ptr context) :
	type(type),
	context(std::move(context))
{

}

inline basic_message_node<char>::basic_message_node
(output_type type, context_ptr context, rt_context &&runtime_context, str_type &&msg) :
	basic_message_node<void>(type, std::move(context)),
	runtime_context(runtime_context),
	msg(msg)
{

}

inline basic_message_node<wchar_t>::basic_message_node
(output_type type, context_ptr context, rt_context &&runtime_context, str_type &&msg) :
	basic_message_node<void>(type, std::move(context)),
	runtime_context(runtime_context),
	msg(msg)
{

}

} //namespace libgs::log


#endif //LIBGS_CORE_LOG_DETAIL_MQ_H
