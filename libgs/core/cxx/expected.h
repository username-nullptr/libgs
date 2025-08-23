
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

#ifndef LIBGS_CORE_CXX_EXPECTED_H
#define LIBGS_CORE_CXX_EXPECTED_H

#include <libgs/core/cxx/optional.h>

namespace libgs
{

template <concepts::optional_value Value, concepts::optional_value Error>
class LIBGS_CORE_TAPI expected : public optional_base<Value>
{
public:
	using value_t = Value;
    using error_t = Error;

	template <typename...Args>
	expected(Args&&...args) requires
		concepts::constructible<value_t,Args...>;

	template <typename...Args>
	expected(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	expected(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected &operator=(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected(expected &&other) requires
		concepts::move_constructible<value_t> and
		concepts::move_constructible<error_t>;

	expected &operator=(expected &&other) requires
		concepts::move_constructible<value_t> and
		concepts::move_constructible<error_t>;

public:
	[[nodiscard]] const error_t &error() const & noexcept;
	[[nodiscard]] error_t &&error() const && noexcept;

	[[nodiscard]] error_t &error() & noexcept;
	[[nodiscard]] error_t &&error() && noexcept;

public:
	template <concepts::optional_value Value0, concepts::optional_value Error0>
	[[nodiscard]] auto and_then(concepts::callable_ret<expected<Value0,Error0>,value_t> auto &&func)
		requires concepts::constructible<Error0,error_t>;

	[[nodiscard]] expected or_else(concepts::callable_ret<expected,error_t> auto &&func);
	[[nodiscard]] expected or_else(value_t value);

private:
	error_t m_error;
};

} //namespace libgs
#include <libgs/core/cxx/detail/expected.h>


#endif //LIBGS_CORE_CXX_EXPECTED_H