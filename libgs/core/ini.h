
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

#ifndef LIBGS_CORE_INI_H
#define LIBGS_CORE_INI_H

#include <libgs/core/value.h>
#include <libgs/core/string_list.h>
#include <map>

namespace libgs
{

class ini_exception : public runtime_error {
public: using runtime_error::runtime_error;
};

template <typename CharT, typename T>
concept ini_read_type =
	basic_value<CharT>::template is_string_v<T> or 
	std::is_arithmetic_v<T> or
	std::is_enum_v<T>;

template <concept_char_type CharT>
class LIBGS_CORE_TAPI basic_ini_keys
{
	LIBGS_DISABLE_COPY_MOVE(basic_ini_keys)

public:
	using string_t = std::basic_string<CharT>;
	using value_t = basic_value<CharT>;
	using key_map = std::map<string_t, value_t>;

public:
	basic_ini_keys() = default;
	virtual ~basic_ini_keys() = default;

public:
	template <ini_read_type<CharT> T = value_t>
	[[nodiscard]] auto read_or(const string_t &key, T default_value = T()) const noexcept;

	template <ini_read_type<CharT> T = value_t>
	[[nodiscard]] auto read(const string_t &key) const;

	[[nodiscard]] value_t read(const string_t &key) const;

	template <typename T>
	basic_ini_keys &write(const string_t &key, T &&value) noexcept;

	template <typename T>
	basic_ini_keys &write(string_t &&key, T &&value) noexcept;

public:
	[[nodiscard]] value_t operator[](const string_t &key) const;
	[[nodiscard]] value_t &operator[](const string_t &key) noexcept;
	[[nodiscard]] value_t &operator[](string_t &&key) noexcept;

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...
	template <ini_read_type<CharT> T>
	value_t operator[](const string_t &key, T default_value) const noexcept;

	template <ini_read_type<CharT> T>
	value_t &operator[](const string_t &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	value_t &operator[](string_t &&key, T default_value) noexcept;
#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

public:
	using iterator = key_map::iterator;
	using const_iterator = key_map::const_iterator;
	using reverse_iterator = key_map::reverse_iterator;
	using const_reverse_iterator = key_map::const_reverse_iterator;

public:
	[[nodiscard]] iterator begin() noexcept;
	[[nodiscard]] const_iterator cbegin() const noexcept;
	[[nodiscard]] const_iterator begin() const noexcept;

	[[nodiscard]] iterator end() noexcept;
	[[nodiscard]] const_iterator cend() const noexcept;
	[[nodiscard]] const_iterator end() const noexcept;

	[[nodiscard]] reverse_iterator rbegin() noexcept;
	[[nodiscard]] const_reverse_iterator crbegin() const noexcept;
	[[nodiscard]] const_reverse_iterator rbegin() const noexcept;

	[[nodiscard]] reverse_iterator rend() noexcept;
	[[nodiscard]] const_reverse_iterator crend() const noexcept;
	[[nodiscard]] const_reverse_iterator rend() const noexcept;

public:
	[[nodiscard]] iterator find(const string_t &key) noexcept;
	[[nodiscard]] const_iterator find(const string_t &key) const noexcept;

	basic_ini_keys &clear() noexcept;
	[[nodiscard]] size_t size() const noexcept;

protected:
	key_map m_keys;
};

template <concept_char_type CharT>
class LIBGS_CORE_TAPI basic_ini
{
	LIBGS_DISABLE_COPY(basic_ini)

public:
	using string_t = std::basic_string<CharT>;
	using value_t = basic_value<CharT>;
	using string_list_t = basic_string_list<CharT>;

	using ini_keys_t = basic_ini_keys<CharT>;
	using group_map = std::map<string_t, ini_keys_t>;

	static constexpr bool is_char_v = libgs::is_char_v<CharT>;

public:
	explicit basic_ini(std::string_view file_name = {});
	virtual ~basic_ini();

	basic_ini(basic_ini &&other) noexcept;
	basic_ini &operator=(basic_ini &&other) noexcept;

public:
	basic_ini &set_file_name(std::string_view file_name);
	[[nodiscard]] std::string file_name() const noexcept;

public:
	template <ini_read_type<CharT> T = value_t>
	[[nodiscard]] auto read_or(const string_t &group, const string_t &key, T default_value = T()) const noexcept;

	template <ini_read_type<CharT> T = value_t>
	[[nodiscard]] auto read(const string_t &group, const string_t &key) const;

	[[nodiscard]] value_t read(const string_t &group, const string_t &key) const;

	template <typename T>
	basic_ini &write(const string_t &group, const string_t &key, T &&value) noexcept;

	template <typename T>
	basic_ini &write(const string_t &group, string_t &&key, T &&value) noexcept;

	template <typename T>
	basic_ini &write(string_t &&group, const string_t &key, T &&value) noexcept;

	template <typename T>
	basic_ini &write(string_t &&group, string_t &&key, T &&value) noexcept;

public:
	[[nodiscard]] const ini_keys_t &group(const string_t &group) const;
	[[nodiscard]] ini_keys_t &group(const string_t &group) noexcept(false);

public:
	[[nodiscard]] const ini_keys_t &operator[](const string_t &group) const;
	[[nodiscard]] ini_keys_t &operator[](const string_t &group) noexcept;
	[[nodiscard]] ini_keys_t &operator[](string_t &&group) noexcept;

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...
	[[nodiscard]] value_t operator[](const string_t &group, const string_t &key) const noexcept(false);
	[[nodiscard]] value_t &operator[](const string_t &group, const string_t &key) noexcept;
	[[nodiscard]] value_t &operator[](const string_t &group, string_t &&key) noexcept;
	[[nodiscard]] value_t &operator[](string_t &&group, const string_t &key) noexcept;
	[[nodiscard]] value_t &operator[](string_t &&group, string_t &&key) noexcept;

	template <ini_read_type<CharT> T>
	[[nodiscard]] value_t operator[](const string_t &group, const string_t &key, T default_value) const noexcept;

	template <ini_read_type<CharT> T>
	[[nodiscard]] value_t &operator[](const string_t &group, const string_t &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	[[nodiscard]] value_t &operator[](const string_t &group, string_t &&key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	[[nodiscard]] value_t &operator[](string_t &&group, const string_t &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	[[nodiscard]] value_t &operator[](string_t &&group, string_t &&key, T default_value) noexcept;
#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

public:
	using iterator = group_map::iterator;
	using const_iterator = group_map::const_iterator;
	using reverse_iterator = group_map::reverse_iterator;
	using const_reverse_iterator = group_map::const_reverse_iterator;

public:
	[[nodiscard]] iterator begin() noexcept;
	[[nodiscard]] const_iterator cbegin() const noexcept;
	[[nodiscard]] const_iterator begin() const noexcept;

	[[nodiscard]] iterator end() noexcept;
	[[nodiscard]] const_iterator cend() const noexcept;
	[[nodiscard]] const_iterator end() const noexcept;

	[[nodiscard]] reverse_iterator rbegin() noexcept;
	[[nodiscard]] const_reverse_iterator crbegin() const noexcept;
	[[nodiscard]] const_reverse_iterator rbegin() const noexcept;

	[[nodiscard]] reverse_iterator rend() noexcept;
	[[nodiscard]] const_reverse_iterator crend() const noexcept;
	[[nodiscard]] const_reverse_iterator rend() const noexcept;

public:
	bool load(std::string &errmsg) noexcept;
	basic_ini &load();

	bool sync(std::string &errmsg) noexcept;
	basic_ini &sync();

	basic_ini &set_sync_on_delete(bool enable = true) noexcept;
	[[nodiscard]] bool sync_on_delete() const noexcept;

public:
	[[nodiscard]] iterator find(const string_t &group) noexcept;
	[[nodiscard]] const_iterator find(const string_t &group) const noexcept;

	basic_ini &clear() noexcept;
	[[nodiscard]] size_t size() const noexcept;

protected:
	class impl;
	impl *m_impl;
};

using ini = basic_ini<char>;
using wini = basic_ini<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/ini.h>


#endif //LIBGS_CORE_INI_H
