
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

#ifndef LIBGS_CORE_CXX_DETIAL_OPTIONAL_H
#define LIBGS_CORE_CXX_DETIAL_OPTIONAL_H

#include <libgs/core/cxx/tools.h>

namespace libgs { namespace detail
{

template <concepts::optional_value Value>
void check_optional_has_value(const optional_base<Value> &opt)
{
	if( not opt.has_value() )
	{
		runtime_error::loc_throw(std::format (
			"libgs::optional_base<{}> has no value",
			type_name<Value>()
		));
	}
}

} //namespace detail

template <concepts::optional_value Value>
optional_base<Value>::optional_base(value_t value) :
	m_has_value(true)
{
	_emplace(std::move(value));
}

template <concepts::optional_value Value>
optional_base<Value>::~optional_base()
{
	if( has_value() )
		_reset();
}

template <concepts::optional_value Value>
optional_base<Value>::optional_base(const optional_base &other) requires
	concepts::copy_constructible<value_t> :
	m_has_value(other.m_has_value)
{
	if( other.m_has_value )
		_emplace(*other._data());
}

template <concepts::optional_value Value>
optional_base<Value> &optional_base<Value>::operator=(const optional_base &other) requires
	concepts::copy_constructible<value_t>
{
	m_has_value = other.m_has_value;
	if( other.m_has_value )
		_emplace(*other._data());
	return *this;
}

template <concepts::optional_value Value>
optional_base<Value>::optional_base(optional_base &&other) requires
	concepts::move_constructible<value_t> :
	m_has_value(other.m_has_value)
{
	if( other.m_has_value )
	{
		_emplace(std::move(*other._data()));
		other._reset();
	}
}

template <concepts::optional_value Value>
optional_base<Value> &optional_base<Value>::operator=(optional_base &&other) requires
	concepts::move_constructible<value_t>
{
	if( this == &other )
		return *this;

	m_has_value = other.m_has_value;
	if( other.m_has_value )
	{
		_emplace(std::move(*other._data()));
		other._reset();
	}
	return *this;
}

template <concepts::optional_value Value>
bool optional_base<Value>::has_value() const noexcept
{
	return m_has_value;
}

template <concepts::optional_value Value>
const Value &optional_base<Value>::value() const &
{
	detail::check_optional_has_value(*this);
	return *_data();
}

template <concepts::optional_value Value>
Value &&optional_base<Value>::value() const &&
{
	detail::check_optional_has_value(*this);
	return std::move(*_data());
}

template <concepts::optional_value Value>
Value &optional_base<Value>::value() &
{
	detail::check_optional_has_value(*this);
	return *_data();
}

template <concepts::optional_value Value>
Value &&optional_base<Value>::value() &&
{
	detail::check_optional_has_value(*this);
	return std::move(*_data());
}

template <concepts::optional_value Value>
Value optional_base<Value>::value_or(value_t default_value) const & noexcept
{
	return has_value() ? *_data() : std::move(default_value);
}

template <concepts::optional_value Value>
Value optional_base<Value>::value_or(value_t default_value) const && noexcept
{
	return has_value() ? std::move(*_data()) : std::move(default_value);
}

template <concepts::optional_value Value>
Value optional_base<Value>::value_or() const & noexcept
	requires concepts::constructible<value_t>
{
	return has_value() ? *_data() : value_t();
}

template <concepts::optional_value Value>
Value optional_base<Value>::value_or() const && noexcept
	requires concepts::constructible<value_t>
{
	return has_value() ? std::move(*_data()) : value_t();
}

template <concepts::optional_value Value>
optional_base<Value>::operator bool() const noexcept
{
	return has_value();
}

template <concepts::optional_value Value>
const Value &optional_base<Value>::operator*() const &
{
	return value();
}

template <concepts::optional_value Value>
Value &&optional_base<Value>::operator*() const &&
{
	return std::move(value());
}

template <concepts::optional_value Value>
Value &optional_base<Value>::operator*() &
{
	return value();
}

template <concepts::optional_value Value>
Value &&optional_base<Value>::operator*() &&
{
	return std::move(value());
}

template <concepts::optional_value Value>
const Value *optional_base<Value>::operator->() const
{
	return &value();
}

template <concepts::optional_value Value>
Value *optional_base<Value>::operator->()
{
	return &value();
}

template <concepts::optional_value Value>
bool optional_base<Value>::operator==(const optional_base &other) const
	requires std::equality_comparable<value_t>
{
	return has_value() and other.has_value() and value() == other.value();
}

template <concepts::optional_value Value>
template <typename...Args>
void optional_base<Value>::_emplace(Args&&...args) requires
	concepts::constructible<value_t,Args...>
{
	new (&m_storage) value_t(std::forward<Args>(args)...);
	this->m_has_value = true;
}

template <concepts::optional_value Value>
const Value *optional_base<Value>::_data() const noexcept
{
	return std::launder(reinterpret_cast<const value_t*>(&m_storage));
}

template <concepts::optional_value Value>
Value *optional_base<Value>::_data() noexcept
{
	return std::launder(reinterpret_cast<value_t*>(&m_storage));
}

template <concepts::optional_value Value>
void optional_base<Value>::_reset() noexcept
{
	std::destroy_at(_data());
	m_has_value = false;
}

template <concepts::optional_value Value>
optional<Value>::optional(value_t value) :
	optional_base<Value>(std::move(value))
{

}

template <concepts::optional_value Value>
optional<Value>::optional(nullopt_t)
{

}

template <concepts::optional_value Value>
template <typename...Args>
optional<Value> &optional<Value>::emplace(Args&&...args) requires
	concepts::constructible<value_t,Args...>
{
	this->_emplace(std::forward<Args>(args)...);
	return *this;
}

template <concepts::optional_value Value>
optional<Value> &optional<Value>::operator=(value_t value) noexcept
{
	this->_emplace(std::move(value));
	return *this;
}

template <concepts::optional_value Value>
optional<Value> &optional<Value>::reset() noexcept
{
	if( this->has_value() )
		this->_reset();
	return *this;
}

template <concepts::optional_value Value>
template <typename Func>
auto optional<Value>::transform(Func &&func) const requires transform_v<Func>
{
	using result_t = std::invoke_result_t<Func,value_t>;
	return this->has_value() ?
		libgs::make_optional(func(this->value())) : optional<result_t>();
}

template <concepts::optional_value Value>
template <typename Func>
auto optional<Value>::and_then(Func &&func) const requires and_then_v<Func>
{
	using result_t = std::invoke_result_t<Func,value_t>;
	return this->has_value() ? func(this->value()) : result_t();
}

template <concepts::optional_value Value>
template <typename Func>
optional<Value> optional<Value>::or_else(Func &&func) const requires or_else_v<Func>
{
	if constexpr( concepts::callable_ret<Func,optional> )
	{
		return this->has_value() ?
			*this : func();
	}
	else
	// else if constexpr( concepts::callable_void<Func> )
	{
		if( not this->has_value() )
			func();
		return *this;
	}
}

template <concepts::optional_value Value>
optional<Value> optional<Value>::or_else(value_t value) const
{
	return this->has_value() ?
		*this : optional(std::move(value));
}

template <concepts::optional_value_p Value>
constexpr auto make_optional(Value &&value)
{
	return optional<std::remove_cvref_t<Value>>(std::forward<Value>(value));
}

template <concepts::optional_value Value, typename...Args>
optional<Value> make_optional(Args&&...args)
	requires concepts::constructible<Value,Args...>
{
	return optional<Value>(std::forward<Args>(args)...);
}

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETIAL_OPTIONAL_H