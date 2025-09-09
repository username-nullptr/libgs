
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

namespace libgs { namespace concepts
{

template <typename Value>
concept expected_value = optional_value<Value> or std::is_void_v<Value>;

} //namespace libgs::concepts

template <concepts::optional_value Error>
class LIBGS_CORE_TAPI unexpected
{
public:
    using error_t = Error;

	template <typename...Args>
	unexpected(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

public:
	[[nodiscard]] const error_t &error() const & noexcept;
	[[nodiscard]] error_t &&error() const && noexcept;

	[[nodiscard]] error_t &error() & noexcept;
	[[nodiscard]] error_t &&error() && noexcept;

protected:
	error_t m_error;
};

template <typename Value, concepts::optional_value Error>
class expected;

template <concepts::optional_value Value, concepts::optional_value Error>
class LIBGS_CORE_TAPI expected<Value,Error> final : public optional_base<Value>, public unexpected<Error>
{
public:
	using value_t = Value;
    using error_t = Error;

	expected(value_t value);
	expected(unexpected<error_t> une);

	expected(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected &operator=(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected(expected &&other) noexcept requires
		concepts::move_constructible<value_t> and
		concepts::move_constructible<error_t>;

	expected &operator=(expected &&other) noexcept requires
		concepts::move_constructible<value_t> and
		concepts::move_constructible<error_t>;

public:
	[[nodiscard]] bool is_error() const noexcept;

	template <typename...Args>
	expected &emplace(Args&&...args) requires
		concepts::constructible<value_t,Args...>;

	template <typename...Args>
	expected &despair(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	expected &despair(unexpected<error_t> une);

	expected &operator=(value_t value) noexcept;
	expected &operator=(unexpected<error_t> une) noexcept;

public:
	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool transform_v = requires(Func func, value_t value) {
		{ func(value) } -> concepts::expected_value;
	};
	template <typename Func>
	auto transform(Func &&func) const requires transform_v<Func>;

	template <concepts::callable_novoid<value_t> Func>
	static constexpr bool and_then_v = requires(Func func, value_t value) {
		[]<typename U0, typename U1>(expected<U0,U1>) {} (func(value));
	};
	template <typename Func>
	auto and_then(Func &&func) const requires and_then_v<Func>;

	template <typename Func>
	static constexpr bool or_else_0_v = requires(Func func, error_t error) {
		[]<typename U0, typename U1>(expected<U0,U1>) {} (func(error));
	};

	template <typename Func>
	static constexpr bool or_else_1_v = requires(Func func) {
		[]<typename U0, typename U1>(expected<U0,U1>) {} (func());
	};

	template <typename Func>
	static constexpr bool or_else_2_v = requires(Func func, error_t error) {
		{ func(error) } -> std::same_as<void>;
	};

	template <typename Func>
	static constexpr bool or_else_3_v = requires(Func func) {
		{ func() } -> std::same_as<void>;
	};

	template <typename Func>
	static constexpr bool or_else_v =
		or_else_0_v<Func> or or_else_1_v<Func> or
		or_else_2_v<Func> or or_else_3_v<Func>;

	template <typename Func>
	expected or_else(Func &&func) const requires or_else_v<Func>;
	[[nodiscard]] expected or_else(value_t value = {}) const;

public:
	static constexpr bool exception_v = requires(error_t error) {
		error.exception(std::string());
	};
	const expected &exception(const std::string &what = "") const requires exception_v;
	expected &exception(const std::string &what = "") requires exception_v;
};

template <concepts::optional_value Error>
class LIBGS_CORE_TAPI expected<void,Error> final : public unexpected<Error>
{
public:
    using error_t = Error;

	expected() = default;
	expected(unexpected<error_t> une);

	expected(const expected &other) requires
		concepts::copy_constructible<error_t>;

	expected &operator=(const expected &other) requires
		concepts::copy_constructible<error_t>;

	expected(expected &&other) noexcept requires
		concepts::move_constructible<error_t>;

	expected &operator=(expected &&other) noexcept requires
		concepts::move_constructible<error_t>;

public:
	[[nodiscard]] bool has_value() const noexcept;
	[[nodiscard]] bool is_error() const noexcept;
	expected &emplace() noexcept;

	template <typename...Args>
	expected &despair(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	expected &despair(unexpected<error_t> une);

public:
	auto transform(concepts::callable auto &&func) const;

	template <concepts::callable_novoid Func>
	static constexpr bool and_then_v = requires(Func func) {
		[]<typename U>(expected<void,U>) {} (func());
	};
	template <typename Func>
	auto and_then(Func &&func) const requires and_then_v<Func>;

	template <typename Func>
	static constexpr bool or_else_0_v = requires(Func func, error_t error) {
		[]<typename U>(expected<void,U>) {} (func(error));
	};

	template <typename Func>
	static constexpr bool or_else_1_v = requires(Func func) {
		[]<typename U>(expected<void,U>) {} (func());
	};

	template <typename Func>
	static constexpr bool or_else_2_v = requires(Func func, error_t error) {
		{ func(error) } -> std::same_as<void>;
	};

	template <typename Func>
	static constexpr bool or_else_3_v = requires(Func func) {
		{ func() } -> std::same_as<void>;
	};

	template <typename Func>
	static constexpr bool or_else_v =
		or_else_0_v<Func> or or_else_1_v<Func> or
		or_else_2_v<Func> or or_else_3_v<Func>;

	template <typename Func>
	expected or_else(Func &&func) const requires or_else_v<Func>;
	[[nodiscard]] expected or_else() const;

public:
	[[nodiscard]] operator bool() const noexcept;
	expected &operator=(unexpected<error_t> une) noexcept;

public:
	static constexpr bool exception_v = requires(error_t error) {
		error.exception(std::string());
	};
	const expected &exception(const std::string &what = "") const requires exception_v;
	expected &exception(const std::string &what = "") requires exception_v;

private:
	bool m_has_value = true;
};

} //namespace libgs
#include <libgs/core/cxx/detail/expected.h>


#endif //LIBGS_CORE_CXX_EXPECTED_H