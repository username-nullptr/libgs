
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
	template <typename...Args>
	void error(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	[[nodiscard]] const error_t &error() const & noexcept;
	[[nodiscard]] error_t &&error() const && noexcept;

	[[nodiscard]] error_t &error() & noexcept;
	[[nodiscard]] error_t &&error() && noexcept;

public:
	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool and_then_v = requires(Func func, value_t value) {
		[]<typename U0, typename U1>(expected<U0,U1>) {} (func(value));
	};

	template <typename Func>
	[[nodiscard]] auto and_then(Func &&func) requires and_then_v<Func>;

	template <typename Token>
	static constexpr bool or_else_v =
		concepts::callable_ret<Token,expected,error_t> or
		concepts::callable_ret<Token,error_t,error_t> or
		concepts::callable_ret<Token,expected> or
		concepts::callable_ret<Token,error_t> or
		std::same_as<std::remove_cvref_t<Token>,value_t>;

	template <typename Token>
	[[nodiscard]] expected or_else(Token &&token) requires or_else_v<Token>;

private:
	error_t m_error;
};

} //namespace libgs
#include <libgs/core/cxx/detail/expected.h>


#endif //LIBGS_CORE_CXX_EXPECTED_H