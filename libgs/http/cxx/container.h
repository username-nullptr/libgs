// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_CXX_CONTAINER_H
#define LIBGS_HTTP_CXX_CONTAINER_H

#include <libgs/http/cxx/attributes.h>
#include <libgs/http/cxx/concepts.h>
#include <libgs/core/container.h>
#include <map>
#include <set>

namespace libgs::http
{

using key_t = std::string;

struct LIBGS_HTTP_API less_case_insensitive {
	[[nodiscard]] bool operator()(const key_t &v1, const key_t &v2) const;
};

template <typename Value>
using map = std::map<key_t, Value, less_case_insensitive>;

template <typename Value>
using set = std::set<Value, less_case_insensitive>;

using value_map = map<value>;
using value_set = set<value>;

[[nodiscard]] LIBGS_HTTP_TAPI optional<value> value_map_get (
	const value_map &map, const core_concepts::text_p<char> auto &key
) noexcept;

[[nodiscard]] LIBGS_HTTP_VAPI optional<value> value_set_get (
	const value_set &set, const value &node
) noexcept;

} //namespace libgs::http
#include <libgs/http/cxx/detail/container.h>


#endif //LIBGS_HTTP_CXX_CONTAINER_H
