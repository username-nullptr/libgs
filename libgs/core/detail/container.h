
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_DETAIL_CONTAINER_H
#define LIBGS_CORE_DETAIL_CONTAINER_H

namespace libgs
{

template <typename Derived>
const_parameters<Derived>::const_parameters(const parameters_t *parameters) :
	m_parameters(parameters)
{

}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(const concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it == parameters().end() )
		return nullopt;
	return it->second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const concepts::text_p<char> auto &key, const value_t &value) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it != parameters().end() )
		return it->second == value;
	return false;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter
(const concepts::text_p<char> auto &key) const noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	return it != parameters().end();
}

template <typename Derived>
optional<typename const_parameters<Derived>::value_t>
const_parameters<Derived>::parameter(size_t index) const
{
	if( not contains_parameter(index) )
		runtime_error::loc_throw("index out of range.");
	return parameters()[index].second;
}

template <typename Derived>
bool const_parameters<Derived>::contains_parameter(size_t index) const noexcept
{
	return index < parameters().size();
}

template <typename Derived>
const const_parameters<Derived>::parameters_t&
const_parameters<Derived>::parameters() const noexcept
{
	return *m_parameters;
}

template <typename Derived>
template <concepts::text_p<char> T>
mutable_parameters<Derived>::base_t::derived_t &mutable_parameters<Derived>::set_parameter
(T &&key, typename base_t::value_t value) noexcept
{
	parameters()[strtls::to_string(std::forward<T>(key))] = std::move(value);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
template <concepts::text_p<char> T>
mutable_parameters<Derived>::base_t::derived_t&
mutable_parameters<Derived>::unset_parameter(const T &key) noexcept
{
	auto it = parameters().find(strtls::to_string(key));
	if( it != parameters().end() )
		parameters().erase(it);
	return static_cast<base_t::derived_t&>(*this);
}

template <typename Derived>
mutable_parameters<Derived>::base_t::parameters_t&
mutable_parameters<Derived>::parameters() noexcept
{
	return remove_const(*this->m_parameters);
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_CONTAINER_H
