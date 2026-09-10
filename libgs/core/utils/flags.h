// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_FLAGS_H
#define LIBGS_CORE_UTILS_FLAGS_H

#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>
#include <initializer_list>

namespace libgs { namespace concepts
{

template <typename T>
concept flag_template = std::is_enum_v<T> and sizeof(T) <= sizeof(uint32_t);

template <typename T>
concept flag_number = std::is_integral_v<T> and sizeof(T) <= sizeof(uint32_t);

} //namespace concepts

template <concepts::flag_template Enum>
class LIBGS_CORE_TAPI flags
{
public:
	using enum_t = Enum;
	constexpr flags() noexcept = default;
	constexpr flags(enum_t f) noexcept;
	constexpr flags(const flags &other) = default;
	constexpr flags(std::initializer_list<enum_t> flag_values) noexcept;
	flags &operator=(const flags &other) = default;

public:
	template <concepts::flag_number Int>
	flags &operator&=(Int mask) noexcept;

	flags &operator&=(enum_t mask) noexcept;
	flags &operator|=(flags f) noexcept;
	flags &operator|=(enum_t f) noexcept;
	flags &operator^=(flags f) noexcept;
	flags &operator^=(enum_t f) noexcept;

public:
	template <concepts::flag_number Int>
	[[nodiscard]] constexpr flags operator&(Int mask) const noexcept;

	[[nodiscard]] constexpr flags operator&(enum_t f) const noexcept;
	[[nodiscard]] constexpr flags operator|(flags f) const noexcept;
	[[nodiscard]] constexpr flags operator|(enum_t f) const noexcept;
	[[nodiscard]] constexpr flags operator^(flags f) const noexcept;
	[[nodiscard]] constexpr flags operator^(enum_t f) const noexcept;
	[[nodiscard]] constexpr flags operator~() const noexcept;
	[[nodiscard]] constexpr bool operator!() const noexcept;

public:
	[[nodiscard]] constexpr auto operator<=>(const flags&) const noexcept = default;
	[[nodiscard]] constexpr bool operator==(const flags&) const noexcept = default;

	[[nodiscard]] constexpr auto operator<=>(enum_t f) const noexcept;
	[[nodiscard]] constexpr bool operator==(enum_t f) const noexcept;

public:
	template <concepts::arithmetic T>
	[[nodiscard]] constexpr operator T() const noexcept;
	[[nodiscard]] constexpr operator enum_t() const noexcept;
	[[nodiscard]] constexpr uint32_t operator*() const noexcept;

	template <concepts::arithmetic T = uint32_t>
	[[nodiscard]] constexpr T value() const noexcept;

	[[nodiscard]] constexpr bool test_flag(enum_t f) const noexcept;
	constexpr flags &set_flag(enum_t f, bool on = true) noexcept;

private:
	using iterator = std::initializer_list<enum_t>::const_iterator;
	[[nodiscard]] constexpr static int initializer_list_helper(iterator it, iterator end) noexcept;
	uint32_t m_value = 0;
};

} //namespace libgs
#include <libgs/core/utils/detail/flags.h>

#define LIBGS_DECLARE_FLAGS(v_flags, v_enum)  using v_flags = libgs::flags<v_enum>

#define LIBGS_DECLARE_OPERATORS_FOR_FLAGS(_flags) \
	constexpr inline libgs::flags<_flags::enum_t> operator| \
	(_flags::enum_t f1, _flags::enum_t f2) noexcept \
	{ return libgs::flags<_flags::enum_t>(f1) | f2; } \
	constexpr inline libgs::flags<_flags::enum_t> operator| \
	(_flags::enum_t f1, libgs::flags<_flags::enum_t> f2) noexcept \
	{ return f2 | f1; }


#endif //LIBGS_CORE_UTILS_FLAGS_H
