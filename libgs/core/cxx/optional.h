
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

#ifndef LIBGS_CORE_CXX_OPTIONAL_H
#define LIBGS_CORE_CXX_OPTIONAL_H

#include <libgs/core/cxx/exception.h>
#include <libgs/core/cxx/concepts.h>
#include <optional>

namespace libgs
{

template <concepts::optional_value Value>
class LIBGS_CORE_TAPI optional_base
{
public:
	using value_t = Value;
	using storage_t = std::aligned_storage_t <
		sizeof(value_t), alignof(value_t)
	>;

public:
	~optional_base();
	optional_base(const optional_base &other) requires
		concepts::copy_constructible<value_t>;

	optional_base &operator=(const optional_base &other) requires
		concepts::copy_constructible<value_t>;

	optional_base(optional_base &&other) requires
		concepts::move_constructible<value_t>;

	optional_base &operator=(optional_base &&other) requires
		concepts::move_constructible<value_t>;

public:
	[[nodiscard]] bool has_value() const noexcept;

	[[nodiscard]] const value_t &value() const &;
	[[nodiscard]] value_t &&value() const &&;

	[[nodiscard]] value_t &value() &;
	[[nodiscard]] value_t &&value() &&;

	[[nodiscard]] value_t value_or(value_t default_value) const & noexcept;
	[[nodiscard]] value_t value_or(value_t default_value) const && noexcept;

	[[nodiscard]] value_t value_or() const & noexcept
		requires concepts::constructible<value_t>;

	[[nodiscard]] value_t value_or() const && noexcept
		requires concepts::constructible<value_t>;

public:
	[[nodiscard]] explicit operator bool() const noexcept;

	[[nodiscard]] const value_t &operator*() const &;
	[[nodiscard]] value_t &&operator*() const &&;

	[[nodiscard]] value_t &operator*() &;
	[[nodiscard]] value_t &&operator*() &&;

	[[nodiscard]] const value_t *operator->() const;
	[[nodiscard]] value_t *operator->();

	[[nodiscard]] bool operator==(const optional_base &other) const
		requires std::equality_comparable<value_t>;

public:
	void operator+(const optional_base&) = delete;
	void operator-(const optional_base&) = delete;
	void operator/(const optional_base&) = delete;
	void operator%(const optional_base&) = delete;

protected:
	optional_base(value_t value);
	optional_base() = default;

	template <typename...Args>
	void _emplace(Args&&...args) requires
		concepts::constructible<value_t,Args...>;

	const value_t *_data() const noexcept;
	value_t *_data() noexcept;
	void _reset() noexcept;

protected:
	storage_t m_storage;
	bool m_has_value = false;
};

#define LIBGS_OPTIONAL_ERROR_IF(opt, fmt, ...) if( not opt ) \
	throw libgs::runtime_error(libgs::with_location(std::format(fmt,__VA_ARGS__)))

template <concepts::optional_value Value>
class LIBGS_CORE_TAPI optional final : public optional_base<Value>
{
public:
	using value_t = Value;

	optional(value_t value);
	optional() = default;

	template <typename...Args>
	optional &emplace(Args&&...args) requires
		concepts::constructible<value_t,Args...>;

	optional &operator=(value_t value) noexcept;
	optional &reset() noexcept;

public:
	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool transform_v = requires(Func func, value_t value) {
		{ func(value) } -> concepts::optional_value;
	};
	template <typename Func>
	auto transform(Func &&func) const requires transform_v<Func>;

	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool and_then_v = requires(Func func, value_t value) {
		[]<typename U>(optional<U>) {} (func(value));
	};
	template <typename Func>
	auto and_then(Func &&func) const requires and_then_v<Func>;

	template <typename Func>
	static constexpr bool or_else_v =
		concepts::callable_ret<Func,optional> or
		concepts::callable_void<Func>;

	template <typename Func>
	optional or_else(Func &&func) const requires or_else_v<Func>;
	[[nodiscard]] optional or_else(value_t value = {}) const;
};

template <concepts::optional_value_p Value>
[[nodiscard]] constexpr auto make_optional(Value &&value);

template <concepts::optional_value Value, typename...Args>
[[nodiscard]] LIBGS_CORE_TAPI optional<Value> make_optional(Args&&...args)
	requires concepts::constructible<Value,Args...>;

} //namespace libgs
#include <libgs/core/cxx/detail/optional.h>


#endif //LIBGS_CORE_CXX_OPTIONAL_H