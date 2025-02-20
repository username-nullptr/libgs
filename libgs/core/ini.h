
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

#include <libgs/core/execution.h>
#include <libgs/core/string_list.h>
#include <libgs/core/value.h>
#include <map>

namespace libgs
{

template <concepts::char_type CharT,
		  typename KeyMap = std::map<std::basic_string<CharT>,basic_value<CharT>>>
class LIBGS_CORE_TAPI basic_ini_keys
{
	LIBGS_DISABLE_COPY_MOVE(basic_ini_keys)

public:
	using char_t = CharT;
	using string_t = std::basic_string<char_t>;
	using value_t = basic_value<char_t>;
	using key_map_t = KeyMap;

	template <typename...Args>
	using format_string = typename value_t::template format_string<Args...>;

public:
	basic_ini_keys() = default;
	virtual ~basic_ini_keys() = default;

public:
	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) read_or (
		concepts::basic_string_type<char_t> auto &&key, T &&def_value = T()
	) const noexcept;

	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] auto read(concepts::basic_string_type<char_t> auto &&key) const;

	void write (
		concepts::basic_string_type<char_t> auto &&key,
		concepts::basic_value_arg<char_t> auto &&value
	) noexcept;

public:
	[[nodiscard]] value_t operator[](concepts::basic_string_type<char_t> auto &&key) const;
	[[nodiscard]] value_t &operator[](concepts::basic_string_type<char_t> auto &&key) noexcept;

#if LIBGS_CPLUSPLUS >= 202100L
	decltype(auto) operator[] (
		concepts::basic_string_type<char_t> auto &&key,
		concepts::basic_value_arg<char_t> auto &&def_value
	) const noexcept;

	decltype(auto) operator[] (
		concepts::basic_string_type<char_t> auto &&key,
		concepts::basic_value_arg<char_t> auto &&def_value
	) noexcept;
#endif //LIBGS_CPLUSPLUS

public:
	using iterator = typename key_map_t::iterator;
	using const_iterator = typename key_map_t::const_iterator;
	using reverse_iterator = typename key_map_t::reverse_iterator;
	using const_reverse_iterator = typename key_map_t::const_reverse_iterator;

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
	[[nodiscard]] iterator find(concepts::basic_string_type<char_t> auto &&key) noexcept;
	[[nodiscard]] const_iterator find(concepts::basic_string_type<char_t> auto &&key) const noexcept;

	void clear() noexcept;
	[[nodiscard]] size_t size() const noexcept;

protected:
	key_map_t m_keys;
};

namespace concepts
{

template <typename T, typename CharT>
concept base_of_basic_ini_keys = std::is_base_of_v<basic_ini_keys<CharT, typename T::key_map_t>, T>;

template <typename T>
concept basic_of_char_ini_keys = base_of_basic_ini_keys<char,T>;

template <typename T>
concept basic_of_wchar_ini_keys = base_of_basic_ini_keys<wchar_t,T>;

template <typename T>
concept basic_of_ini_keys = basic_of_char_ini_keys<T> or basic_of_wchar_ini_keys<T>;

} //namespace concepts

template <concepts::char_type CharT,
		  concepts::base_of_basic_ini_keys<CharT> IniKeys = basic_ini_keys<CharT>,
		  concepts::execution Exec = asio::any_io_executor,
		  typename GroupMap = std::map<std::basic_string<CharT>,IniKeys>>
class LIBGS_CORE_TAPI basic_ini
{
	LIBGS_DISABLE_COPY(basic_ini)

public:
	using char_t = CharT;
	using ini_keys_t = IniKeys;
	using executor_t = Exec;

	using path_t = std::filesystem::path;
	using string_t = std::basic_string<char_t>;
	using value_t = basic_value<char_t>;

	using string_list_t = basic_string_list<char_t>;
	using group_map_t = GroupMap;

	struct group_key
	{
		string_t group;
		string_t key;

		group_key (
			concepts::basic_string_type<char_t> auto &&group,
			concepts::basic_string_type<char_t> auto &&key
		) noexcept;

		template <concepts::basic_string_type<CharT> Str>
		group_key(const std::pair<Str,Str> &pair) noexcept;

		template <concepts::basic_string_type<CharT> Str>
		group_key(std::pair<Str,Str> &&pair) noexcept;

		template <concepts::basic_string_type<CharT> Str>
		group_key(const std::tuple<Str,Str> &tuple) noexcept;

		template <concepts::basic_string_type<CharT> Str>
		group_key(std::tuple<Str,Str> &&tuple) noexcept;
	};

public:
	explicit basic_ini (
		concepts::match_execution_context<executor_t> auto &exec,
		const path_t &file_name = {}
	);
	explicit basic_ini (
		const concepts::match_execution<executor_t> auto &exec,
		const path_t &file_name = {}
	);
	explicit basic_ini(const path_t &file_name = {}) requires
		concepts::match_default_execution<executor_t>;

	virtual ~basic_ini();
	basic_ini(basic_ini &&other) noexcept;
	basic_ini &operator=(basic_ini &&other) noexcept;

	template <typename Exec0>
	explicit basic_ini(basic_ini<char_t,IniKeys,Exec0,GroupMap> &&other)
		requires concepts::match_execution<Exec0,executor_t>;

	template <typename Exec0>
	basic_ini &operator=(basic_ini<char_t,IniKeys,Exec0,GroupMap> &&other)
		requires concepts::match_execution<Exec0,executor_t>;

public:
	virtual void set_file_name(const path_t &file_name);
	[[nodiscard]] virtual path_t file_name() const noexcept;

public:
	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) read_or(group_key gk, T &&def_value = T()) const noexcept;

	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) read_or (
		concepts::basic_string_type<char_t> auto &&path, T &&def_value = T()
	) const;

	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] auto read(group_key gk) const;

	template <concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] auto read (
		concepts::basic_string_type<char_t> auto &&path
	) const;

public:
	void write (
		group_key gk, concepts::basic_value_arg<char_t> auto &&value
	) noexcept;

	void write (
		concepts::basic_string_type<char_t> auto &&path,
		concepts::basic_value_arg<char_t> auto &&value
	) noexcept;

public:
	[[nodiscard]] const ini_keys_t &group(concepts::basic_string_type<char_t> auto &&group) const;
	[[nodiscard]] ini_keys_t &group(concepts::basic_string_type<char_t> auto &&group);

	[[nodiscard]] const ini_keys_t &operator[](concepts::basic_string_type<char_t> auto &&group) const;
	[[nodiscard]] ini_keys_t &operator[](concepts::basic_string_type<char_t> auto &&group) noexcept;

	[[nodiscard]] value_t operator[](group_key gk) const;
	[[nodiscard]] value_t &operator[](group_key gk) noexcept;

#if LIBGS_CPLUSPLUS >= 202100L
	[[nodiscard]] value_t operator[] (
		concepts::basic_string_type<char_t> auto &&group,
		concepts::basic_string_type<char_t> auto &&key
	) const;

	[[nodiscard]] value_t &operator[] (
		concepts::basic_string_type<char_t> auto &&group,
		concepts::basic_string_type<char_t> auto &&key
	) noexcept;

	[[nodiscard]] decltype(auto) operator[] (
		concepts::basic_string_type<char_t> auto &&group,
		concepts::basic_string_type<char_t> auto &&key,
		concepts::basic_value_arg<char_t> auto &&def_value
	) const noexcept;

	[[nodiscard]] decltype(auto) operator[] (
		concepts::basic_string_type<char_t> auto &&group,
		concepts::basic_string_type<char_t> auto &&key,
		concepts::basic_value_arg<char_t> auto &&def_value
	) noexcept;
#endif //LIBGS_CPLUSPLUS

public:
	using iterator = typename group_map_t::iterator;
	using const_iterator = typename group_map_t::const_iterator;
	using reverse_iterator = typename group_map_t::reverse_iterator;
	using const_reverse_iterator = typename group_map_t::const_reverse_iterator;

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
	template <concepts::opt_token<error_code> Token = use_sync_t>
	auto load(Token &&token = {});

	template <concepts::opt_token<error_code> Token = use_sync_t>
	auto load_or(Token &&token = {});

	template <concepts::opt_token<error_code> Token = use_sync_t>
	auto sync(Token &&token = {});

	template <typename Rep, typename Period>
	void set_sync_period(const duration<Rep,Period> &period = {});
	void set_sync_on_delete(bool enable = true) noexcept;

	[[nodiscard]] milliseconds sync_period() const noexcept;
	[[nodiscard]] bool sync_on_delete() const noexcept;
	void cancel();

public:
	[[nodiscard]] iterator find(concepts::basic_string_type<char_t> auto &&group) noexcept;
	[[nodiscard]] const_iterator find(concepts::basic_string_type<char_t> auto &&group) const noexcept;

	void clear() noexcept;
	[[nodiscard]] size_t size() const noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;

protected:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using ini = basic_ini<char>;
using wini = basic_ini<wchar_t>;

} //namespace libgs
#include <libgs/core/detail/ini.h>


#endif //LIBGS_CORE_INI_H
