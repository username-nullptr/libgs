
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

template <concepts::optional_value Error>
template <typename...Args>
unexpected<Error>::unexpected(Args&&...args) requires
	concepts::constructible<error_t,Args...> :
	m_error(std::forward<Args>(args)...)
{

}

template <concepts::optional_value Error>
const Error &unexpected<Error>::error() const & noexcept
{
	return m_error;
}

template <concepts::optional_value Error>
Error &&unexpected<Error>::error() const && noexcept
{
	return std::move(m_error);
}

template <concepts::optional_value Error>
Error &unexpected<Error>::error() & noexcept
{
	return m_error;
}

template <concepts::optional_value Error>
Error &&unexpected<Error>::error() && noexcept
{
	return std::move(m_error);
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(value_t value) :
	optional_base<value_t>(std::move(value))
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(unexpected<error_t> une) :
	unexpected<error_t>(std::move(une))
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(const expected &other) requires
	concepts::copy_constructible<value_t> and
	concepts::copy_constructible<error_t> :
	optional_base<value_t>(other),
	unexpected<error_t>(other)
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(const expected &other) requires
	concepts::copy_constructible<value_t> and
	concepts::copy_constructible<error_t>
{
	optional_base<value_t>::operator=(other);
	unexpected<error_t>::operator=(other);
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error>::expected(expected &&other) noexcept requires
	concepts::move_constructible<value_t> and
	concepts::move_constructible<error_t> :
	optional_base<Value>(std::move(other)),
	unexpected<error_t>(std::move(other))
{

}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(expected &&other) noexcept requires
	concepts::move_constructible<value_t> and
	concepts::move_constructible<error_t>
{
	if( this == &other )
		return *this;
	optional_base<Value>::operator=(std::move(other));
	unexpected<error_t>::operator=(std::move(other));
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
bool expected<Value,Error>::is_error() const noexcept
{
	return not this->has_value();
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename...Args>
expected<Value,Error> &expected<Value,Error>::emplace(Args&&...args) requires
	concepts::constructible<value_t,Args...>
{
	this->m_value = value_t(std::forward<Args>(args)...);
	this->m_error = error_t();
	this->m_has_value = true;
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename...Args>
expected<Value,Error> &expected<Value,Error>::despair(Args&&...args) requires
	concepts::constructible<error_t,Args...>
{
	this->m_error = error_t(std::forward<Args>(args)...);
	this->m_value = value_t();
	this->m_has_value = false;
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::despair(unexpected<error_t> une)
{
	this->m_error = une.error();
	this->m_value = value_t();
	this->m_has_value = false;
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(value_t value) noexcept
{
	return emplace(std::move(value));
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::operator=(unexpected<error_t> une) noexcept
{
	return despair(std::move(une));
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename Func>
auto expected<Value,Error>::transform(Func &&func) const requires transform_v<Func>
{
	using result_t = std::invoke_result_t<Func,value_t>;
	return this->has_value() ?
		expected<result_t,error_t>(func(this->value())) :
		expected<result_t,error_t>(this->error());
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename Func>
auto expected<Value,Error>::and_then(Func &&func) const requires and_then_v<Func>
{
	using result_t = std::invoke_result_t<Func,value_t>;
	return this->has_value() ? func(this->value()) : result_t(this->error());
}

template <concepts::optional_value Value, concepts::optional_value Error>
template <typename Func>
expected<Value,Error> expected<Value,Error>::or_else(Func &&func) const requires or_else_v<Func>
{
	if constexpr( or_else_0_v<Func> )
		return this->has_value() ? *this : func(this->error());

	else if constexpr( or_else_1_v<Func> )
		return this->has_value() ? *this : func();

	else if constexpr( or_else_2_v<Func> )
	{
		if( not this->has_value() )
			func(this->error());
		return *this;
	}
	else if constexpr( or_else_3_v<Func> )
	{
		if( not this->has_value() )
			func();
		return *this;
	}
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> expected<Value,Error>::or_else(value_t value) const
{
	return this->has_value() ?
		*this : expected(std::move(value));
}

template <concepts::optional_value Value, concepts::optional_value Error>
const expected<Value,Error> &expected<Value,Error>::exception(const std::string &what)
	const requires exception_v
{
	if( not this->has_value() )
		this->error().exception(what);
	return *this;
}

template <concepts::optional_value Value, concepts::optional_value Error>
expected<Value,Error> &expected<Value,Error>::exception(const std::string &what)
	requires exception_v
{
	if( not this->has_value() )
		this->error().exception(what);
	return *this;
}

template <concepts::optional_value Error>
expected<void,Error>::expected(unexpected<error_t> une) :
	unexpected<error_t>(std::move(une)),
	m_has_value(false)
{

}

template <concepts::optional_value Error>
expected<void,Error>::expected(const expected &other) requires
	concepts::copy_constructible<error_t> :
	unexpected<error_t>(other),
	m_has_value(other.m_has_value)
{

}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::operator=(const expected &other) requires
	concepts::copy_constructible<error_t>
{
	unexpected<error_t>::operator=(other);
	m_has_value = other.m_has_value;
	return *this;
}

template <concepts::optional_value Error>
expected<void,Error>::expected(expected &&other) noexcept requires
	concepts::move_constructible<error_t> :
	unexpected<error_t>(std::move(other)),
	m_has_value(other.m_has_value)
{
	other.m_has_value = false;
}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::operator=(expected &&other) noexcept requires
	concepts::move_constructible<error_t>
{
	if( this == &other )
		return *this;
	unexpected<error_t>::operator=(std::move(other));
	m_has_value = other.m_has_value;
	other.m_has_value = false;
	return *this;
}

template <concepts::optional_value Error>
bool expected<void,Error>::has_value() const noexcept
{
	return m_has_value;
}

template <concepts::optional_value Error>
bool expected<void,Error>::is_error() const noexcept
{
	return not m_has_value;
}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::emplace() noexcept
{
	this->m_has_value = true;
	return *this;
}

template <concepts::optional_value Error>
template <typename...Args>
expected<void,Error> &expected<void,Error>::despair(Args&&...args) requires
	concepts::constructible<error_t,Args...>
{
	this->m_error = error_t(std::forward<Args>(args)...);
	this->m_has_value = false;
	return *this;
}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::despair(unexpected<error_t> une)
{
	this->m_error = une.error();
	this->m_has_value = false;
	return *this;
}

template <concepts::optional_value Error>
auto expected<void,Error>::transform(concepts::callable auto &&func) const
{
	using Func = decltype(func);
	using result_t = std::invoke_result_t<Func>;

	if constexpr( std::is_void_v<result_t> )
	{
		if( has_value() )
			func();
		return *this;
	}
	else
	{
		return has_value() ?
			expected<result_t,error_t>(func()) :
			expected<result_t,error_t>();
	}
}

template <concepts::optional_value Error>
template <typename Func>
auto expected<void,Error>::and_then(Func &&func) const requires and_then_v<Func>
{
	using result_t = std::invoke_result_t<Func>;
	return has_value() ? func() : result_t();
}

template <concepts::optional_value Error>
template <typename Func>
expected<void,Error> expected<void,Error>::or_else(Func &&func) const requires or_else_v<Func>
{
	if constexpr( or_else_0_v<Func> )
		return has_value() ? *this : func(this->error());

	else if constexpr( or_else_1_v<Func> )
		return has_value() ? *this : func();

	else if constexpr( or_else_2_v<Func> )
	{
		if( not has_value() )
			func(this->error());
		return *this;
	}
	else if constexpr( or_else_3_v<Func> )
	{
		if( not has_value() )
			func();
		return *this;
	}
}

template <concepts::optional_value Error>
expected<void,Error> expected<void,Error>::or_else() const
{
	return has_value() ? *this : expected();
}

template <concepts::optional_value Error>
expected<void,Error>::operator bool() const noexcept
{
	return has_value();
}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::operator=(unexpected<error_t> une) noexcept
{
	return despair(std::move(une));
}

template <concepts::optional_value Error>
const expected<void,Error> &expected<void,Error>::exception(const std::string &what)
	const requires exception_v
{
	if( not has_value() )
		this->error().exception(what);
	return *this;
}

template <concepts::optional_value Error>
expected<void,Error> &expected<void,Error>::exception(const std::string &what)
	requires exception_v
{
	if( not has_value() )
		this->error().exception(what);
	return *this;
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_EXPECTED_H