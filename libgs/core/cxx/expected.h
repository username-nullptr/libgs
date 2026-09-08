// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_EXPECTED_H
#define LIBGS_CORE_CXX_EXPECTED_H

#include <libgs/core/cxx/optional.h>

namespace libgs { namespace concepts
{

template <typename Value>
concept expected_value = optional_value<Value> or std::is_void_v<Value>;

} //namespace libgs::concepts

template <concepts::optional_value Error, typename Derived>
class LIBGS_CORE_TAPI expected_base
{
public:
	using error_t = Error;
	using derived_t = Derived;
	using error_storage_t = std::byte[sizeof(error_t)];

public:
	[[nodiscard]] const error_t &error() const & noexcept;
	[[nodiscard]] error_t &&error() const && noexcept;

	[[nodiscard]] error_t &error() & noexcept;
	[[nodiscard]] error_t &&error() && noexcept;

protected:
	expected_base(error_t error);
	expected_base();

	template <typename...Args>
	void _despair(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	void _swap(expected_base &other)
		noexcept(std::is_nothrow_swappable_v<error_t>);

	void _reset_error() noexcept;

	alignas(error_t) error_storage_t m_error_storage;
	error_t *m_error_ptr = nullptr;
};

template <concepts::optional_value Error>
class LIBGS_CORE_TAPI unexpected final
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

private:
	error_t m_error;
};

// template <typename Value, concepts::optional_value Error>
template <typename Value, typename Error>
class expected
{
	static_assert(concepts::optional_value<Value>,
		"libgs::expected<Value,Error>: Value must be optional_value."
	);
};

template <concepts::optional_value Value, concepts::optional_value Error>
class LIBGS_CORE_TAPI expected<Value,Error> final :
	public optional_base<Value>, public expected_base<Error,expected<Value,Error>>
{
public:
	using value_t = Value;
	using error_t = Error;

	using error_storage_t = std::aligned_storage_t <
		sizeof(error_t), alignof(error_t)
	>;

public:
	expected() requires
		concepts::constructible<value_t>;

	expected(value_t value);
	expected(unexpected<error_t> une);

	expected(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected &operator=(const expected &other) requires
		concepts::copy_constructible<value_t> and
		concepts::copy_constructible<error_t>;

	expected(expected &&other) noexcept (
		std::is_nothrow_move_constructible_v<value_t> and
		std::is_nothrow_move_constructible_v<error_t>
	) requires
		concepts::move_constructible<value_t> and
		concepts::move_constructible<error_t>;

	expected &operator=(expected &&other) noexcept (
		std::is_nothrow_move_constructible_v<value_t> and
		std::is_nothrow_move_constructible_v<error_t>
	) requires
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

	expected &swap(expected &other) noexcept (
		std::is_nothrow_swappable_v<value_t> and
		std::is_nothrow_swappable_v<error_t>
	);

public:
	template <concepts::callable<value_t> Func>
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
};

template <concepts::optional_value Error, concepts::optional_value_p Value>
[[nodiscard]] LIBGS_CORE_TAPI auto make_expected(Value &&value);

template <concepts::optional_value Error>
class LIBGS_CORE_TAPI expected<void,Error> final :
	public expected_base<Error,expected<void,Error>>
{
public:
	using error_t = Error;

	expected() = default;
	expected(unexpected<error_t> une);

	expected(const expected &other) requires
		concepts::copy_constructible<error_t>;

	expected &operator=(const expected &other) requires
		concepts::copy_constructible<error_t>;

	expected(expected &&other)
		noexcept(std::is_nothrow_move_constructible_v<error_t>)
		requires concepts::move_constructible<error_t>;

	expected &operator=(expected &&other)
		noexcept(std::is_nothrow_move_constructible_v<error_t>)
		requires concepts::move_constructible<error_t>;

public:
	[[nodiscard]] bool has_value() const noexcept;
	[[nodiscard]] bool is_error() const noexcept;
	expected &emplace() noexcept;

	template <typename...Args>
	expected &despair(Args&&...args) requires
		concepts::constructible<error_t,Args...>;

	expected &despair(unexpected<error_t> une);

	expected &swap(expected &other) noexcept (
		std::is_nothrow_swappable_v<error_t>
	);

public:
	auto transform(concepts::callable auto &&func) const;

	template <concepts::callable_novoid Func>
	static constexpr bool and_then_v = requires(Func func) {
		[]<typename U0, typename U1>(expected<U0,U1>) {} (func());
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
	[[nodiscard]] explicit operator bool() const noexcept;
	expected &operator=(unexpected<error_t> une) noexcept;

private:
	bool m_has_value = true;
};

template <concepts::optional_value Error>
[[nodiscard]] LIBGS_CORE_TAPI expected<void,Error> make_expected();

} //namespace libgs
#include <libgs/core/cxx/detail/expected.h>


#endif //LIBGS_CORE_CXX_EXPECTED_H
