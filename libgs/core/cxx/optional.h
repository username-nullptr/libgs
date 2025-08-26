
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

#ifndef LIBGS_CORE_CXX_OPTIONAL_H
#define LIBGS_CORE_CXX_OPTIONAL_H

#include <optional>
#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/concepts.h>

namespace libgs
{

template <concepts::optional_value Value>
class LIBGS_CORE_TAPI optional_base
{
public:
	using value_t = Value;
	optional_base() = default;
	optional_base(value_t value);

	optional_base(const optional_base &other) requires
		concepts::copy_constructible<value_t>;

	optional_base &operator=(const optional_base &other) requires
		concepts::copy_constructible<value_t>;

	optional_base(optional_base &&other) requires
		concepts::move_constructible<value_t>;

	optional_base &operator=(optional_base &&other) requires
		concepts::move_constructible<value_t>;

public:
	template <typename...Args>
	void emplace(Args&&...args) requires
		concepts::constructible<value_t,Args...>;

	[[nodiscard]] bool has_value() const noexcept;

	[[nodiscard]] const value_t &value() const &;
	[[nodiscard]] value_t &&value() const &&;

	[[nodiscard]] value_t &value() &;
	[[nodiscard]] value_t &&value() &&;

	[[nodiscard]] value_t value_or(value_t default_value = {}) const & noexcept;
	[[nodiscard]] value_t value_or(value_t default_value = {}) const && noexcept;

public:
	optional_base &operator=(value_t value) noexcept;
	[[nodiscard]] operator bool() const noexcept;

	[[nodiscard]] const value_t &operator*() const &;
	[[nodiscard]] value_t &&operator*() const &&;

	[[nodiscard]] value_t &operator*() &;
	[[nodiscard]] value_t &&operator*() &&;

	[[nodiscard]] const value_t *operator->() const;
	[[nodiscard]] value_t *operator->();

	[[nodiscard]] bool operator==(const optional_base &other) const
		requires std::equality_comparable<value_t>;

protected:
	value_t m_value {};
	bool m_has_value = false;
};

template <concepts::optional_value Value>
class LIBGS_CORE_TAPI optional : public optional_base<Value>
{
public:
	using value_t = Value;
	using optional_base<Value>::optional_base;
	void reset() noexcept;

public:
	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool transform_v =
		concepts::optional_value<std::invoke_result_t<Func,value_t>>;

	template <typename Func>
	[[nodiscard]] auto transform(Func &&func) requires transform_v<Func>;

	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool and_then_v = requires(Func func, value_t value) {
		[]<typename U>(optional<U>) {} (func(value));
	};

	template <typename Func>
	[[nodiscard]] auto and_then(Func &&func) requires and_then_v<Func>;

	template <typename Func>
	static constexpr bool or_else_v =
		concepts::callable_ret<Func,optional> or
		concepts::callable_void<Func>;

	template <typename Func>
	[[nodiscard]] optional or_else(Func &&func) requires or_else_v<Func>;
	[[nodiscard]] optional or_else(value_t value);
};

template <concepts::optional_value_p Value>
[[nodiscard]] constexpr auto make_optional(Value &&value);

template <concepts::optional_value Value, typename...Args>
[[nodiscard]] LIBGS_CORE_TAPI optional<Value> make_optional(Args&&...args)
	requires concepts::constructible<Value,Args...>;

} //namespace libgs
#include <libgs/core/cxx/detail/optional.h>


#endif //LIBGS_CORE_CXX_OPTIONAL_H