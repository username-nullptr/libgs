
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
#include <libgs/core/shared_mutex.h>
#include <libgs/core/cxx/exception.h>
#include <map>

namespace libgs
{

class ini_exception : public exception {
public: using exception::exception;
};

template <typename CharT, typename T>
concept ini_read_type =
	basic_value<CharT>::template is_string_v<T> or 
	std::is_arithmetic_v<T> or
	std::is_enum_v<T>;

template <concept_char_type CharT>
class basic_ini_keys
{
	LIBGS_DISABLE_COPY_MOVE(basic_ini_keys)

public:
	using str_type = std::basic_string<CharT>;
	using value_type = basic_value<CharT>;
	using key_map = std::map<str_type, value_type>;

public:
	basic_ini_keys() = default;
	virtual ~basic_ini_keys() = default;

public:
	template <ini_read_type<CharT> T = value_type>
	auto read_or(const str_type &key, T default_value = T()) const noexcept;

	template <ini_read_type<CharT> T = value_type>
	auto read(const str_type &key) const noexcept(false);

	template <typename T>
	basic_ini_keys &write(const str_type &key, T &&value) noexcept;

public:
	value_type operator[](const str_type &key) const noexcept(false);
	value_type &operator[](const str_type &key) noexcept;
	value_type &operator[](str_type &&key) noexcept;

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...
	template <ini_read_type<CharT> T>
	value_type operator[](const str_type &key, T default_value) const noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](const str_type &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](str_type &&key, T default_value) noexcept;
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

protected:
	key_map m_keys;
};

template <concept_char_type CharT>
class basic_ini
{
	LIBGS_DISABLE_COPY(basic_ini)

public:
	using str_type = std::basic_string<CharT>;
	using value_type = basic_value<CharT>;
	using str_list_type = basic_string_list<CharT>;

	using ini_keys = basic_ini_keys<CharT>;
	using group_map = std::map<str_type, ini_keys>;

	static constexpr bool is_char_v = libgs::is_char_v<CharT>;

public:
	explicit basic_ini(std::string file_name = {});
	virtual ~basic_ini();

public:
	basic_ini(basic_ini &&other) noexcept;
	basic_ini &operator=(basic_ini &&other) noexcept;

public:
	basic_ini &set_file_nmae(const std::string &file_name);
	std::string file_name() const noexcept;

public:
	template <ini_read_type<CharT> T = value_type>
	auto read_or(const str_type &group, const str_type &key, T default_value = T()) const noexcept;

	template <ini_read_type<CharT> T = value_type>
	auto read(const str_type &group, const str_type &key) const noexcept(false);

	template <typename T>
	basic_ini &write(const str_type &group, const str_type &key, T &&value) noexcept;

public:
	const ini_keys &group(const str_type &group) const noexcept(false);
	ini_keys &group(const str_type &group) noexcept(false);

public:
	const ini_keys &operator[](const str_type &group) const noexcept(false);
	ini_keys &operator[](const str_type &group) noexcept;
	ini_keys &operator[](str_type &&group) noexcept;

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...
	value_type operator[](const str_type &group, const str_type &key) const noexcept(false);
	value_type &operator[](const str_type &group, const str_type &key) noexcept;
	value_type &operator[](const str_type &group, str_type &&key) noexcept;
	value_type &operator[](str_type &&group, const str_type &key) noexcept;
	value_type &operator[](str_type &&group, str_type &&key) noexcept;

	template <ini_read_type<CharT> T>
	value_type operator[](const str_type &group, const str_type &key, T default_value) const noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](const str_type &group, const str_type &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](const str_type &group, str_type &&key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](str_type &&group, const str_type &key, T default_value) noexcept;

	template <ini_read_type<CharT> T>
	value_type &operator[](str_type &&group, str_type &&key, T default_value) noexcept;

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
	basic_ini &load();
	basic_ini &sync();
	basic_ini &set_sync_on_delete(bool enable = true) noexcept;
	[[nodiscard]] bool sync_on_delete() const noexcept;

protected:
	str_type parsing_group(const str_type &str, size_t line);
	bool parsing_key_value(const str_type &curr_group, const str_type &str, size_t line);
	
protected:
	struct data
	{
		std::string file_name;
		group_map groups;
		bool m_sync_on_delete = false;
	}
	*m_data;
	inline static shared_mutex m_rw_lock;
};

using ini = basic_ini<char>;

using wini = basic_ini<wchar_t>;

LIBGS_CORE_API ini &ini_instance();

LIBGS_CORE_API wini &wini_instance();

} //namespace libgs
#include <libgs/core/detail/ini.h>


#endif //LIBGS_CORE_INI_H
