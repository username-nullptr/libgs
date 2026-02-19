
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_CXX_CONTAINER_H
#define LIBGS_HTTP_NT_CXX_CONTAINER_H

#include <libgs/http_nt/cxx/attributes.h>
#include <libgs/http_nt/cxx/concepts.h>
#include <libgs/core/value.h>
#include <map>
#include <set>

namespace libgs::http_nt
{

using key_t = std::string;

struct LIBGS_HTTP_NT_API less_case_insensitive {
	[[nodiscard]] bool operator()(const key_t &v1, const key_t &v2) const;
};

template <typename Value>
using map = std::map<key_t, Value, less_case_insensitive>;

template <typename Value>
using set = std::set<Value, less_case_insensitive>;

using value_map = map<value>;
using value_set = set<value>;

[[nodiscard]] LIBGS_HTTP_NT_TAPI optional<value> value_map_get (
	const value_map &map, const core_concepts::text_p<char> auto &key
) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_VAPI optional<value> value_set_get (
	const value_set &set, const value &node
) noexcept;

using kv_vector = std::vector<std::pair<std::string,value>>;

class LIBGS_HTTP_NT_API parameter_map : public kv_vector
{
public:
	using kv_vector::kv_vector;
	using kv_vector::operator[];

	[[nodiscard]] iterator find(std::string_view key);
	[[nodiscard]] const_iterator find(std::string_view key) const;

	value &operator[](std::string_view key);
	value &operator[](std::string &&key);
	[[nodiscard]] const value &operator[](std::string_view key) const;
};

} //namespace libgs::http_nt
#include <libgs/http_nt/cxx/detail/container.h>


#endif //LIBGS_HTTP_NT_CXX_CONTAINER_H
