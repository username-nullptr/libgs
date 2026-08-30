
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_UTILS_DETAIL_ASIO_TOOLS_H
#define LIBGS_CORE_UTILS_DETAIL_ASIO_TOOLS_H

namespace libgs
{

inline const_buffer::const_buffer(const asio::const_buffer &buf) :
	asio::const_buffer(buf.data(), buf.size())
{

}

inline const_buffer::const_buffer(const mutable_buffer &buf) :
	asio::const_buffer(buf.data(), buf.size())
{

}

inline const_buffer::const_buffer(const char *buf) :
	asio::const_buffer(buf, strlen(buf))
{

}

inline const_buffer::const_buffer(const std::string &buf) :
	asio::const_buffer(buf.c_str(), buf.size())
{

}

inline const_buffer::const_buffer(std::string_view buf) :
	asio::const_buffer(buf.data(), buf.size())
{

}

inline const_buffer &const_buffer::operator=(const mutable_buffer &buf)
{
	operator=(const_buffer(buf.data(), buf.size()));
	return *this;
}

template <typename...Args>
auto buffer(Args&&...args) requires (sizeof...(Args) > 0)
{
	using tuple_t = std::tuple<Args...>;
	using buf_t = std::tuple_element_t<0,tuple_t>;

	if constexpr( std::is_same_v<std::remove_cvref_t<buf_t>, std::nullptr_t> )
		return asio::buffer("",0);
	else
		return asio::buffer(std::forward<Args>(args)...);
}

template <typename Token>
decltype(auto) unbound_token(Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( is_redirect_time_v<token_t> )
		return unbound_token(token.token);
	else if constexpr( is_redirect_error_v<token_t> )
		return return_reference(token.token_);
	else if constexpr( is_cancellation_slot_binder_v<token_t> )
		return token.get();
	else
		return std::forward<Token>(token);
}

decltype(auto) get_executor_helper(concepts::sched auto &&exec)
{
	using Exec = decltype(exec);
	using exec_t = std::remove_cvref_t<Exec>;

	if constexpr( is_exec_v<exec_t> )
		return std::forward<Exec>(exec);
	else
		return exec.get_executor();
}

} //namespace libgs


#endif //LIBGS_CORE_UTILS_DETAIL_ASIO_TOOLS_H
