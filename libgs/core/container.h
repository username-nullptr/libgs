
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_CONTAINER_H
#define LIBGS_CORE_CONTAINER_H

#include <libgs/core/value.h>

namespace libgs
{

using key_t = std::string;

using kv_vector = std::vector<std::pair<std::string,value>>;

class LIBGS_CORE_API parameter_map : public kv_vector
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

template <typename Derived>
class LIBGS_CORE_TAPI const_parameters
{
public:
	using derived_t = crtp_derived_t<Derived,const_parameters>;
	using parameters_t = parameter_map;
	using value_t = value;

public:
	explicit const_parameters(const parameters_t *parameters);
	virtual ~const_parameters() = default;

	[[nodiscard]] optional<value_t> parameter (
		const concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] bool contains_parameter (
		const concepts::text_p<char> auto &key,
		const value_t &value
	) const noexcept;

	[[nodiscard]] bool contains_parameter (
		const concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] optional<value_t> parameter(size_t index) const;
	[[nodiscard]] bool contains_parameter(size_t index) const noexcept;

	[[nodiscard]] const parameters_t &parameters() const noexcept;

protected:
	const parameters_t *m_parameters = nullptr;
};

template <typename Derived>
class LIBGS_CORE_TAPI mutable_parameters : public const_parameters<Derived>
{
	using base_t = const_parameters<Derived>;

public:
	template <concepts::text_p<char> T>
	base_t::derived_t &set_parameter(T &&key, base_t::value_t value) noexcept;

	template <concepts::text_p<char> T>
	base_t::derived_t &unset_parameter(const T &key) noexcept;

	[[nodiscard]] base_t::parameters_t &parameters() noexcept;
	using base_t::parameters;
	using base_t::base_t;
};

} //namespace libgs
#include <libgs/core/detail/container.h>


#endif //LIBGS_CORE_CONTAINER_H
