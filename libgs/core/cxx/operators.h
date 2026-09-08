// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_OPERATOR_H
#define LIBGS_CORE_CXX_OPERATOR_H

#include <libgs/core/cxx/attributes.h>
#include <libgs/core/cxx/concepts.h>

namespace libgs
{

[[nodiscard]] constexpr bool equality (
	concepts::arithmetic_p auto a, concepts::arithmetic_p auto b
);

[[nodiscard]] constexpr bool nequality (
	concepts::arithmetic_p auto a, concepts::arithmetic_p auto b
);

[[nodiscard]] constexpr bool equal_greater (
	concepts::arithmetic_p auto a, concepts::arithmetic_p auto b
);

[[nodiscard]] constexpr bool equal_less (
	concepts::arithmetic_p auto a, concepts::arithmetic_p auto b
);

} //namespace libgs
#include <libgs/core/cxx/detail/operators.h>


#endif //LIBGS_CORE_CXX_OPERATOR_H
