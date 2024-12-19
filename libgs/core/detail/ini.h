
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

#ifndef LIBGS_CORE_DETAIL_INI_H
#define LIBGS_CORE_DETAIL_INI_H

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/app_utls.h>
#include <filesystem>
#include <fstream>

namespace libgs
{

template <concepts::char_type CharT>
template <concepts::ini_read<CharT> T>
auto basic_ini_keys<CharT>::read_or(const string_t &key, T default_value) const noexcept
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_keys.find(key);
		if( it == m_keys.end() )
			return default_value;
		return value_t(it->second);
	}
	else
		return read<value_t>(key).template get<T>();
}

template <concepts::char_type CharT>
template <concepts::ini_read<CharT> T>
auto basic_ini_keys<CharT>::read(const string_t &key) const
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_keys.find(key);
		if( it != m_keys.end() )
			return value_t(it->second);

		if constexpr( is_char_v<CharT> )
			throw ini_exception("basic_ini_keys: read: The key '{}' is not exists.", key);
		else
			throw ini_exception("basic_ini_keys: read: The key '{}' is not exists.", libgs::wcstombs(key));
	}
	else
		return read<value_t>(key).template get<T>();
}

template <concepts::char_type CharT>
basic_value<CharT> basic_ini_keys<CharT>::read(const string_t &key) const
{
	return read<value_t>(key);
}

template <concepts::char_type CharT>
template <typename T>
void basic_ini_keys<CharT>::write(const string_t &key, T &&value) noexcept
{
	m_keys[key] = std::forward<T>(value);
}

template <concepts::char_type CharT>
template <typename T>
void basic_ini_keys<CharT>::write(string_t &&key, T &&value) noexcept
{
	m_keys[std::move(key)] = std::forward<T>(value);
}

template <concepts::char_type CharT>
basic_value<CharT> basic_ini_keys<CharT>::operator[](const string_t &key) const
{
	return read<value_t>(key);
}

template <concepts::char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](const string_t &key) noexcept
{
	return m_keys[key];
}

template <concepts::char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](string_t &&key) noexcept
{
	return m_keys[std::move(key)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302 // TODO ...

template <concepts::char_type CharT>
template <concepts::ini_read<CharT> T>
typename basic_ini_keys<CharT>::value_t
basic_ini_keys<CharT>::operator[](const string_t &key, T default_value) const noexcept
{
	return read_or<value_t>(key, default_value);
}

template <concepts::char_type CharT>
template <concepts::ini_read<CharT> T>
typename basic_ini_keys<CharT>::value_t&
basic_ini_keys<CharT>::operator[](const string_t &key, T default_value) noexcept
{
	return *m_keys.emplace(std::make_pair(key, default_value)).first;
}

template <concepts::char_type CharT>
template <concepts::ini_read<CharT> T>
typename basic_ini_keys<CharT>::value_t&
basic_ini_keys<CharT>::operator[](string_t &&key, T default_value) noexcept
{
	return *m_keys.emplace(std::make_pair(std::move(key), default_value)).first;
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::begin() noexcept
{
	return m_keys.begin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::cbegin() const noexcept
{
	return m_keys.cbegin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::begin() const noexcept
{
	return m_keys.begin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::end() noexcept
{
	return m_keys.end();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::cend() const noexcept
{
	return m_keys.cend();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::end() const noexcept
{
	return m_keys.end();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::reverse_iterator basic_ini_keys<CharT>::rbegin() noexcept
{
	return m_keys.rbegin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::crbegin() const noexcept
{
	return m_keys.crbegin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::rbegin() const noexcept
{
	return m_keys.rbegin();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::reverse_iterator basic_ini_keys<CharT>::rend() noexcept
{
	return m_keys.rend();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::crend() const noexcept
{
	return m_keys.crend();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::rend() const noexcept
{
	return m_keys.rend();
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::find(const string_t &key) noexcept
{
	return m_keys.find(key);
}

template <concepts::char_type CharT>
typename basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::find(const string_t &key) const noexcept
{
	return m_keys.find(key);
}

template <concepts::char_type CharT>
void basic_ini_keys<CharT>::clear() noexcept
{
	m_keys.clear();
}

template <concepts::char_type CharT>
size_t basic_ini_keys<CharT>::size() const noexcept
{
	return m_keys.size();
}

namespace detail
{

template <typename T>
struct ini_keyword_char {};

template <>
struct ini_keyword_char<char>
{
	static constexpr char left_bracket = '[';
	static constexpr char right_bracket = ']';
	static constexpr char assigning = '=';
	static constexpr char single_quotes = '\'';
	static constexpr char double_quotes = '"';
	static constexpr char sharp = '#';
	static constexpr char semicolon = ';';
	static constexpr char line_break = '\n';
};

template <>
struct ini_keyword_char<wchar_t>
{
	static constexpr wchar_t left_bracket = L'[';
	static constexpr wchar_t right_bracket = L']';
	static constexpr wchar_t assigning = L'=';
	static constexpr wchar_t single_quotes = L'\'';
	static constexpr wchar_t double_quotes = L'"';
	static constexpr wchar_t sharp = L'#';
	static constexpr wchar_t semicolon = L';';
	static constexpr wchar_t line_break = L'\n';
};

} //namespace detail

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
class LIBGS_CORE_TAPI basic_ini<CharT,IniKeys>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	inline static class error_category final : public std::error_category
	{
		LIBGS_DISABLE_COPY_MOVE(error_category)

	protected:
		constexpr explicit error_category(const char *name, const char *desc) :
			m_name(name), m_desc(desc) {}

	public:
		[[nodiscard]] const char *name() const noexcept override {
			return m_name;
		}
		[[nodiscard]] std::string message(int line) const override {
			return std::format("Ini file parsing: syntax error: [line:{}]: {}.", line, m_desc);
		}

	private:
		const char *m_name;
		const char *m_desc;
	}
	s_invalid_group          { "invalid_group"         , "Invalid group"                         },
	s_no_group_specified     { "no_group_specified"    , "No group specified"                    },
	s_invalid_key_value_line { "invalid_key_value_line", "Invalid key-value line"                },
	s_key_is_empty           { "invalid_key_value_line", "Invalid key-value line: Key is empty"  },
	s_invalid_value          { "invalid_key_value_line", "Invalid key-value line: Invalid value" };

public:
	impl() = default;

	[[nodiscard]] string_t parsing_group
	(const string_t &str, size_t line, error_code &error) noexcept
	{
		if( str.size() < 3 or not str.ends_with(detail::ini_keyword_char<CharT>::right_bracket) )
		{
			error = std::error_code(line, s_invalid_group);
			return {};
		}
		auto group = from_percent_encoding(str_trimmed(string_t(str.c_str() + 1, str.size() - 2)));
		if( group.empty() )
		{
			error = std::error_code(line, s_invalid_group);
			return {};
		}
		groups[group];
		return group;
	}

	[[nodiscard]] bool parsing_key_value
	(const string_t &curr_group, const string_t &str, size_t line, error_code &error) noexcept
	{
		using keyword_char = detail::ini_keyword_char<CharT>;
		if( curr_group.empty() )
		{
			error = std::error_code(line, s_no_group_specified);
			return false;
		}
		else if( str.size() < 2 )
		{
			error = std::error_code(line, s_invalid_key_value_line);
			return false;
		}
		auto pos = str.find(keyword_char::assigning);
		if( pos == 0 )
		{
			error = std::error_code(line, s_key_is_empty);
			return false;
		}
		else if( pos == string_t::npos )
		{
			error = std::error_code(line, s_invalid_key_value_line);
			return false;
		}
		auto key = from_percent_encoding(str_trimmed(str.substr(0,pos)));
		if( key.empty() )
		{
			error = std::error_code(line, s_key_is_empty);
			return false;
		}
		auto value = str_trimmed(str.substr(pos+1));
		{
			int i = 0;
			for(; i<static_cast<int>(value.size()); i++)
			{
				if( value[i] != keyword_char::assigning )
					break;
			}
			if( --i >= 0 )
				value = value.substr(0,i);
		}
		auto set_invalid_value = [&error, line]
		{
			error = std::error_code(line, s_invalid_value);
			return false;
		};
		if( value.size() == 1 )
		{
			if( value[0] == keyword_char::assigning or
			    value[0] == keyword_char::single_quotes or
			    value[0] == keyword_char::double_quotes )
				return set_invalid_value();
			return true;
		}
		else if( value[0] == keyword_char::single_quotes or value[0] == keyword_char::double_quotes )
		{
			if( value.back() != value[0] )
				return set_invalid_value();
			value = value.substr(1, value.size() - 2);
		}
		groups[curr_group][key] = from_percent_encoding(value);
		return true;
	}

public:
	std::string file_name;
	group_map groups {};
	bool m_sync_on_delete = false;
	friend class basic_ini;
};

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
basic_ini<CharT,IniKeys>::basic_ini(std::string_view file_name) :
	m_impl(new impl())
{
	set_file_name(file_name);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
basic_ini<CharT,IniKeys>::~basic_ini()
{
	if( not sync_on_delete() )
		return ;

	std::string errmsg;
	sync(errmsg);
	ignore_unused(errmsg);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
basic_ini<CharT,IniKeys>::basic_ini(basic_ini &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
basic_ini<CharT,IniKeys> &basic_ini<CharT,IniKeys>::operator=(basic_ini &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
void basic_ini<CharT,IniKeys>::set_file_name(std::string_view file_name)
{
	auto _name = app::absolute_path(file_name);
	if( m_impl->file_name == _name )
		return *this;

	m_impl->file_name = std::move(_name);
	std::string error;

	load(error);
	ignore_unused(error);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
std::string_view basic_ini<CharT,IniKeys>::file_name() const noexcept
{
	return m_impl->file_name;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
auto basic_ini<CharT,IniKeys>::read_or(const string_t &group, const string_t &key, T default_value) const noexcept
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_impl->groups.find(group);
		if( it == m_impl->groups.end() )
			return default_value;
		return it->second.template read_or<T>(key, default_value);
	}
	else
		return read<value_t>(group, key, default_value).template get<T>();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
auto basic_ini<CharT,IniKeys>::read(const string_t &group, const string_t &key) const noexcept(false)
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_impl->groups.find(group);
		if( it != m_impl->groups.end() )
			return it->second.template read<T>(key);

		if constexpr( is_char_v<CharT> )
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", group);
		else
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", libgs::wcstombs(group));
	}
	else
		return read<value_t>(group, key).template get<T>();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t
basic_ini<CharT,IniKeys>::read(const string_t &group, const string_t &key) const
{
	return read<value_t>(group, key);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <typename T>
void basic_ini<CharT,IniKeys>::write(const string_t &group, const string_t &key, T &&value) noexcept
{
	m_impl->groups[group][key] = std::forward<T>(value);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <typename T>
void basic_ini<CharT,IniKeys>::write(const string_t &group, string_t &&key, T &&value) noexcept
{
	m_impl->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <typename T>
void basic_ini<CharT,IniKeys>::write(string_t &&group, const string_t &key, T &&value) noexcept
{
	m_impl->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <typename T>
void basic_ini<CharT,IniKeys>::write(string_t &&group, string_t &&key, T &&value) noexcept
{
	m_impl->groups[std::move(group)][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
const typename basic_ini<CharT,IniKeys>::ini_keys_t &basic_ini<CharT,IniKeys>::group(const string_t &group) const
{
	auto it = m_impl->groups.find(group);
	if( it != m_impl->groups.end() )
		return it->second;

	if constexpr( is_char_v<CharT> )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::ini_keys_t &basic_ini<CharT,IniKeys>::group(const string_t &group)
{
	auto it = m_impl->groups.find(group);
	if( it != m_impl->groups.end() )
		return it->second;

	if constexpr( is_char_v<CharT> )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
const typename basic_ini<CharT,IniKeys>::ini_keys_t &basic_ini<CharT,IniKeys>::operator[](const string_t &group) const
{
	return this->group(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::ini_keys_t &basic_ini<CharT,IniKeys>::operator[](const string_t &group) noexcept
{
	return m_impl->groups[group];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::ini_keys_t &basic_ini<CharT,IniKeys>::operator[](string_t &&group) noexcept
{
	return m_impl->groups[std::move(group)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t basic_ini<CharT,IniKeys>::operator[]
(const string_t &group, const string_t &key) const noexcept(false)
{
	return (*this)[group][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key) noexcept
{
	return (*this)[group][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, string_t &&key) noexcept
{
	return (*this)[group][std::move(key)];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, const string_t &key) noexcept
{
	return (*this)[std::move(group)][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, string_t &&key) noexcept
{
	return (*this)[std::move(group)][std::move(key)];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys>::value_t basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key, T default_value) const noexcept
{
	return read_or<value_t>(group, key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key, T default_value) noexcept
{
	return (*this)[group][key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, string_t &&key, T default_value) noexcept
{

	return (*this)[group][std::move(key), default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, const string_t &key, T default_value) noexcept
{
	return (*this)[std::move(group)][key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, string_t &&key, T default_value) noexcept
{
	return (*this)[std::move(group)][std::move(key), default_value];
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::iterator basic_ini<CharT,IniKeys>::begin() noexcept
{
	return m_impl->groups.begin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_iterator basic_ini<CharT,IniKeys>::cbegin() const noexcept
{
	return m_impl->groups.cbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_iterator basic_ini<CharT,IniKeys>::begin() const noexcept
{
	return m_impl->groups.begin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::iterator basic_ini<CharT,IniKeys>::end() noexcept
{
	return m_impl->groups.end();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_iterator basic_ini<CharT,IniKeys>::cend() const noexcept
{
	return m_impl->groups.cend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_iterator basic_ini<CharT,IniKeys>::end() const noexcept
{
	return m_impl->groups.end();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::reverse_iterator basic_ini<CharT,IniKeys>::rbegin() noexcept
{
	return m_impl->groups.rbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_reverse_iterator basic_ini<CharT,IniKeys>::crbegin() const noexcept
{
	return m_impl->groups.crbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_reverse_iterator basic_ini<CharT,IniKeys>::rbegin() const noexcept
{
	return m_impl->groups.rbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::reverse_iterator basic_ini<CharT,IniKeys>::rend() noexcept
{
	return m_impl->groups.rend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_reverse_iterator basic_ini<CharT,IniKeys>::crend() const noexcept
{
	return m_impl->groups.crend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_reverse_iterator basic_ini<CharT,IniKeys>::rend() const noexcept
{
	return m_impl->groups.rend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
bool basic_ini<CharT,IniKeys>::load(std::string &errmsg) noexcept
{

}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
void basic_ini<CharT,IniKeys>::load()
{
	std::string errmsg;
	bool res = load(errmsg);
	if( not res )
		throw ini_exception(errmsg);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
template <concepts::opt_token<std::error_code> Token>
auto basic_ini<CharT,IniKeys>::load(Token &&token)
{
	if constexpr( std::is_same_v<Token,error_code&> )
	{
		token = error_code();
		if( not std::filesystem::exists(m_impl->file_name) )
		{
			token = std::make_error_code(std::errc::no_such_file_or_directory);
			return ;
		}
		std::basic_ifstream<CharT> file;
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			file.open(m_impl->file_name);
		}
		catch(const std::system_error &ex) {
			token = ex.code();
		}
		string_t buf;
		string_t curr_group;

		for(size_t line=1; std::getline(file, buf); line++)
		{
			buf = str_trimmed(buf);
			if( buf.empty() or
			    buf[0] == detail::ini_keyword_char<CharT>::sharp or
			    buf[0] == detail::ini_keyword_char<CharT>::semicolon )
				continue;

			auto list = string_list_t::from_string(buf, detail::ini_keyword_char<CharT>::sharp);
			buf = str_trimmed(list[0]);

			list = string_list_t::from_string(buf, detail::ini_keyword_char<CharT>::semicolon);
			buf = str_trimmed(list[0]);

			if( buf.starts_with(detail::ini_keyword_char<CharT>::left_bracket) )
			{
				curr_group = m_impl->parsing_group(buf, line, token);
				if( curr_group.empty() )
					return ;
			}
			else if( not m_impl->parsing_key_value(curr_group, buf, line, token) )
				return ;
		}
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		load(error);
		if( error )
			throw system_error(error, "libgs::basic_ini::load");
	}
	else if constexpr( is_function_v<Token> )
	{

	}
}


template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
bool basic_ini<CharT,IniKeys>::sync(std::string &errmsg) noexcept
{
	using keyword_char = detail::ini_keyword_char<CharT>;
	if( m_impl->file_name.empty() )
	{
		errmsg = std::format("File name is empty.");
		return false;
	}
	std::basic_ofstream<CharT> file(m_impl->file_name, std::ios_base::out | std::ios_base::trunc);
	if( not file.is_open() )
	{
		errmsg = std::format("file '{}' open failed.", m_impl->file_name);
		return false;
	}
	for(auto &[group, keys] : *this)
	{
		file << keyword_char::left_bracket
			 << to_percent_encoding(group)
			 << keyword_char::right_bracket
			 << keyword_char::line_break;

		for(auto &[key, value] : keys)
		{
			file << to_percent_encoding(key) << keyword_char::assigning;
			if( value.is_digit() )
				file << value.to_string();
			else
			{
				file << keyword_char::double_quotes
				     << to_percent_encoding(value.to_string())
				     << keyword_char::double_quotes;
			}
			file << keyword_char::line_break;
		}
		file << keyword_char::line_break;
	}
	file.close();
	return true;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
void basic_ini<CharT,IniKeys>::sync()
{
	std::string errmsg;
	bool res = sync(errmsg);
	if( not res )
		throw ini_exception(errmsg);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
void basic_ini<CharT,IniKeys>::set_sync_on_delete(bool enable) noexcept
{
	m_impl->m_sync_on_delete = enable;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
bool basic_ini<CharT,IniKeys>::sync_on_delete() const noexcept
{
	return m_impl->m_sync_on_delete;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::iterator basic_ini<CharT,IniKeys>::find(const string_t &group) noexcept
{
	return m_impl->groups.find(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
typename basic_ini<CharT,IniKeys>::const_iterator basic_ini<CharT,IniKeys>::find(const string_t &group) const noexcept
{
	return m_impl->groups.find(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
void basic_ini<CharT,IniKeys>::clear() noexcept
{
	m_impl->groups.clear();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys>
size_t basic_ini<CharT,IniKeys>::size() const noexcept
{
	return m_impl->groups.size();
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_INI_H
