
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

#ifndef LIBGS_CORE_CXX_DETAIL_EXPECTED_H
#define LIBGS_CORE_CXX_DETAIL_EXPECTED_H

namespace libgs
{

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename...Args>
expected<Value,Error>::expected(Args&&...args) requires
	concepts::constructible<value_t,Args...> :
	optional_base<Value>(std::forward<Args>(args)...)
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename...Args>
expected<Value,Error>::expected(Args&&...args) requires
	concepts::constructible<error_t,Args...> :
	m_error(std::forward<Args>(args)...)
{
	this->m_has_value = false;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(const expected &other) requires
	concepts::copy_constructible<value_t> and
	concepts::copy_constructible<error_t> :
	optional_base<Value>(other),
	m_error(other.m_error)
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(const expected &other) requires
	concepts::copy_constructible<value_t> and
	concepts::copy_constructible<error_t>
{
	optional_base<Value>::operator=(other);
	m_error = other.m_error;
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(expected &&other) requires
	concepts::move_constructible<value_t> and
	concepts::move_constructible<error_t> :
	optional_base<Value>(std::move(other)),
	m_error(std::move(other.m_error))
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(expected &&other) requires
	concepts::move_constructible<value_t> and
	concepts::move_constructible<error_t>
{
	optional_base<Value>::operator=(std::move(other));
	m_error = std::move(other.m_error);
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename...Args>
void expected<Value,Error>::error(Args&&...args) requires
	concepts::constructible<error_t,Args...>
{
	m_error = value_t(std::forward<Args>(args)...);
	this->m_has_value = false;
}

template <concepts::optional_value Value, concepts::optional_value Error>
const Error &expected<Value,Error>::error() const & noexcept
{
	return m_error;
}

template <concepts::optional_value Value, concepts::optional_value Error>
Error &&expected<Value,Error>::error() const && noexcept
{
	return std::move(m_error);
}

template <concepts::optional_value Value, concepts::optional_value Error>
Error &expected<Value,Error>::error() & noexcept
{
	return m_error;
}

template <concepts::optional_value Value, concepts::optional_value Error>
Error &&expected<Value,Error>::error() && noexcept
{
	return std::move(m_error);
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename Func>
auto expected<Value,Error>::and_then(Func &&func) requires and_then_v<Func>
{
	using result_t = std::invoke_result_t<Func,value_t>;
	return this->has_value() ? func(this->value()) : result_t();
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename Token>
expected<Value,Error> expected<Value,Error>::or_else(Token &&token) requires or_else_v<Token>
{
	if constexpr( concepts::callable_ret<Token,expected,error_t> )
	{
		return this->has_value() ?
			*this : token(error());
	}
	else if constexpr( concepts::callable_ret<Token,expected,error_t> )
	{
		return this->has_value() ?
			*this : expected(token(error()));
	}
	else if constexpr( concepts::callable_ret<Token,expected> )
	{
		return this->has_value() ?
			*this : token();
	}
	else if constexpr( concepts::callable_ret<Token,error_t> )
	{
		return this->has_value() ?
			*this : expected(token());
	}
	else if constexpr( concepts::callable_void<Token> )
	{
		if( not this->has_value() )
			token();
		return *this;
	}
	else
	{
		return this->has_value() ?
			*this : expected(std::forward<Token>(token));
	}
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_EXPECTED_H