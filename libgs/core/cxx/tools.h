// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_TOOLS_H
#define LIBGS_CORE_CXX_TOOLS_H

#include <libgs/core/cxx/aggregate_template.h>
#include <libgs/core/cxx/type_traits.h>
#include <libgs/core/cxx/attributes.h>
#include <typeinfo>

namespace libgs
{

using std_typeid_t = decltype(typeid(void).hash_code());

template <typename T>
[[nodiscard]] LIBGS_CORE_TAPI const char *type_name();

[[nodiscard]] LIBGS_CORE_API const char *type_name(const std::type_info &type);
[[nodiscard]] LIBGS_CORE_TAPI const char *type_name(auto &&t);

template <typename T>
[[nodiscard]] constexpr T &remove_const(const T &v);

template <typename T>
[[nodiscard]] constexpr T *remove_const(const T *v);

template <typename T>
[[nodiscard]] constexpr const T &as_const(const T &v);

template <typename T>
[[nodiscard]] constexpr const T &&as_const(const T &&v);

template <typename T>
[[nodiscard]] constexpr const T *as_const(const T *v);

[[nodiscard]] constexpr decltype(auto) return_reference(auto &&value);

template <typename...Args>
constexpr void ignore_unused(Args&&...) {}

template <typename T, typename U> requires std::is_class_v<T>
struct class_member
{
	using type = std::remove_reference_t <
		decltype(std::declval<T>().*std::declval<U>())
	>;
};

template <typename T, typename U> requires std::is_class_v<T>
using class_member_t = class_member<T,U>::type;

template <typename Derived, typename Base>
struct crtp_derived { using type = Derived; };

template <typename Base>
struct crtp_derived<void,Base> { using type = Base; };

template <typename Derived, typename Base>
using crtp_derived_t = crtp_derived<Derived, Base>::type;

} //namespace libgs
#include <libgs/core/cxx/detail/tools.h>


#endif //LIBGS_CORE_CXX_TOOLS_H
