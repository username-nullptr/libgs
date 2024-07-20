
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

#include <libgs/core/app_utls.h>
#include <filesystem>
#include <fstream>

namespace libgs
{

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
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

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
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

template <concept_char_type CharT>
basic_value<CharT> basic_ini_keys<CharT>::read(const string_t &key) const
{
	return read<value_t>(key);
}

template <concept_char_type CharT>
template <typename T>
basic_ini_keys<CharT> &basic_ini_keys<CharT>::write(const string_t &key, T &&value) noexcept
{
	m_keys[key] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini_keys<CharT> &basic_ini_keys<CharT>::write(string_t &&key, T &&value) noexcept
{
	m_keys[std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> basic_ini_keys<CharT>::operator[](const string_t &key) const
{
	return read<value_t>(key);
}

template <concept_char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](const string_t &key) noexcept
{
	return m_keys[key];
}

template <concept_char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](string_t &&key) noexcept
{
	return m_keys[std::move(key)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302 // TODO ...

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> basic_ini_keys<CharT>::operator[](const string_t &key, T default_value) const noexcept
{
	return read_or<value_t>(key, default_value);
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](const string_t &key, T default_value) noexcept
{
	return *m_keys.emplace(std::make_pair(key, default_value)).first;
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](string_t &&key, T default_value) noexcept
{
	return *m_keys.emplace(std::make_pair(std::move(key), default_value)).first;
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concept_char_type CharT>
basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::begin() noexcept
{
	return m_keys.begin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::cbegin() const noexcept
{
	return m_keys.cbegin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::begin() const noexcept
{
	return m_keys.begin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::end() noexcept
{
	return m_keys.end();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::cend() const noexcept
{
	return m_keys.cend();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::end() const noexcept
{
	return m_keys.end();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::reverse_iterator basic_ini_keys<CharT>::rbegin() noexcept
{
	return m_keys.rbegin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::crbegin() const noexcept
{
	return m_keys.crbegin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::rbegin() const noexcept
{
	return m_keys.rbegin();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::reverse_iterator basic_ini_keys<CharT>::rend() noexcept
{
	return m_keys.rend();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::crend() const noexcept
{
	return m_keys.crend();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_reverse_iterator basic_ini_keys<CharT>::rend() const noexcept
{
	return m_keys.rend();
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::iterator basic_ini_keys<CharT>::find(const string_t &key) noexcept
{
	return m_keys.find(key);
}

template <concept_char_type CharT>
basic_ini_keys<CharT>::const_iterator basic_ini_keys<CharT>::find(const string_t &key) const noexcept
{
	return m_keys.find(key);
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini_keys<CharT>::clear() noexcept
{
	m_keys.clear();
	return *this;
}

template <concept_char_type CharT>
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
	static constexpr char sharp = '#';
	static constexpr char semicolon = ';';
	static constexpr char line_break = '\n';
};

template <>
struct ini_keyword_char<wchar_t>
{
	static constexpr char left_bracket = L'[';
	static constexpr char right_bracket = L']';
	static constexpr char assigning = L'=';
	static constexpr char sharp = L'#';
	static constexpr char semicolon = L';';
	static constexpr char line_break = L'\n';
};

} //namespace detail

template <concept_char_type CharT>
class basic_ini<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	impl() = default;

public:
	[[nodiscard]] string_t parsing_group
	(const string_t &str, size_t line, std::string &errmsg) noexcept
	{
		if( str.size() < 3 or not str.ends_with(detail::ini_keyword_char<CharT>::right_bracket) )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
			return {};
		}
		auto group = from_percent_encoding(str_trimmed(string_t(str.c_str() + 1, str.size() - 2)));
		if( group.empty() )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
			return {};
		}
		groups[group];
		return group;
	}

	[[nodiscard]] bool parsing_key_value
	(const string_t &curr_group, const string_t &str, size_t line, std::string &errmsg) noexcept
	{
		if( curr_group.empty() )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: No group specified.", line);
			return false;
		}
		else if( str.size() < 2 )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
			return false;
		}
		auto pos = str.find(detail::ini_keyword_char<CharT>::assigning);
		if( pos == 0 )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
			return false;
		}
		else if( pos == string_t::npos )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
			return false;
		}
		auto key = from_percent_encoding(str_trimmed(str.substr(0,pos)));
		if( key.empty() )
		{
			errmsg = std::format("Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
			return false;
		}
		groups[curr_group][key] = from_percent_encoding(str_trimmed(str.substr(pos+1)));
		return true;
	}

public:
	std::string file_name;
	group_map groups {};
	bool m_sync_on_delete = false;
	friend class basic_ini;
};

template <concept_char_type CharT>
basic_ini<CharT>::basic_ini(std::string_view file_name) :
	m_impl(new impl())
{
	set_file_nmae(file_name);
}

template <concept_char_type CharT>
basic_ini<CharT>::~basic_ini()
{
	if( not sync_on_delete() )
		return ;

	std::string errmsg;
	sync(errmsg);
	ignore_unused(errmsg);
}

template <concept_char_type CharT>
basic_ini<CharT>::basic_ini(basic_ini &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator=(basic_ini &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::set_file_name(std::string_view file_name)
{
	auto _name = app::absolute_path(file_name);
	if( m_impl->file_name == _name )
		return *this;

	m_impl->file_name = std::move(_name);
	return *this;
}

template <concept_char_type CharT>
std::string basic_ini<CharT>::file_name() const noexcept
{
	return m_impl->file_name;
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini<CharT>::read_or(const string_t &group, const string_t &key, T default_value) const noexcept
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

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini<CharT>::read(const string_t &group, const string_t &key) const noexcept(false)
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_impl->groups.find(group);
		if( it != m_impl->groups.end() )
			return it->second.template read<T>(key);

		if constexpr( is_char_v )
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", group);
		else
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", libgs::wcstombs(group));
	}
	else
		return read<value_t>(group, key).template get<T>();
}

template <concept_char_type CharT>
basic_value<CharT> basic_ini<CharT>::read(const string_t &group, const string_t &key) const
{
	return read<value_t>(group, key);
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(const string_t &group, const string_t &key, T &&value) noexcept
{
	m_impl->groups[group][key] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(const string_t &group, string_t &&key, T &&value) noexcept
{
	m_impl->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(string_t &&group, const string_t &key, T &&value) noexcept
{
	m_impl->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(string_t &&group, string_t &&key, T &&value) noexcept
{
	m_impl->groups[std::move(group)][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
const basic_ini_keys<CharT> &basic_ini<CharT>::group(const string_t &group) const
{
	auto it = m_impl->groups.find(group);
	if( it != m_impl->groups.end() )
		return it->second;

	if constexpr( is_char_v )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::group(const string_t &group)
{
	auto it = m_impl->groups.find(group);
	if( it != m_impl->groups.end() )
		return it->second;

	if constexpr( is_char_v )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concept_char_type CharT>
const basic_ini_keys<CharT> &basic_ini<CharT>::operator[](const string_t &group) const
{
	return this->group(group);
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::operator[](const string_t &group) noexcept
{
	return m_impl->groups[group];
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::operator[](string_t &&group) noexcept
{
	return m_impl->groups[std::move(group)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...

template <concept_char_type CharT>
basic_ini<CharT> basic_ini<CharT>::operator[](const string_t &group, const string_t &key) const noexcept(false)
{
	return (*this)[group][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](const string_t &group, const string_t &key) noexcept
{
	return (*this)[group][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](const string_t &group, string_t &&key) noexcept
{
	return (*this)[group][std::move(key)];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](string_t &&group, const string_t &key) noexcept
{
	return (*this)[std::move(group)][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](string_t &&group, string_t &&key) noexcept
{
	return (*this)[std::move(group)][std::move(key)];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> basic_ini<CharT>::operator[](const string_t &group, const string_t &key, T default_value) const noexcept
{
	return read_or<value_t>(group, key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](const string_t &group, const string_t &key, T default_value) noexcept
{
	return (*this)[group][key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](const string_t &group, string_t &&key, T default_value) noexcept
{

	return (*this)[group][std::move(key), default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](string_t &&group, const string_t &key, T default_value) noexcept
{
	return (*this)[std::move(group)][key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](string_t &&group, string_t &&key, T default_value) noexcept
{
	return (*this)[std::move(group)][std::move(key), default_value];
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concept_char_type CharT>
basic_ini<CharT>::iterator basic_ini<CharT>::begin() noexcept
{
	return m_impl->groups.begin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::cbegin() const noexcept
{
	return m_impl->groups.cbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::begin() const noexcept
{
	return m_impl->groups.begin();
}

template <concept_char_type CharT>
basic_ini<CharT>::iterator basic_ini<CharT>::end() noexcept
{
	return m_impl->groups.end();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::cend() const noexcept
{
	return m_impl->groups.cend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::end() const noexcept
{
	return m_impl->groups.end();
}

template <concept_char_type CharT>
basic_ini<CharT>::reverse_iterator basic_ini<CharT>::rbegin() noexcept
{
	return m_impl->groups.rbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::crbegin() const noexcept
{
	return m_impl->groups.crbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::rbegin() const noexcept
{
	return m_impl->groups.rbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::reverse_iterator basic_ini<CharT>::rend() noexcept
{
	return m_impl->groups.rend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::crend() const noexcept
{
	return m_impl->groups.crend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::rend() const noexcept
{
	return m_impl->groups.rend();
}

template <concept_char_type CharT>
bool basic_ini<CharT>::load(std::string &errmsg) noexcept
{
	if( not std::filesystem::exists(m_impl->file_name) )
	{
		errmsg = std::format("No such file: '{}'", m_impl->file_name);
		return false;
	}
	std::shared_lock locker(m_impl->m_rw_lock);
	LIBGS_UNUSED(locker);

	std::basic_ifstream<CharT> file(m_impl->file_name);
	if( not file.is_open() )
	{
		errmsg = std::format("file '{}' open failed.", m_impl->file_name);
		return false;
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
			curr_group = m_impl->parsing_group(buf, line, errmsg);
			if( curr_group.empty() )
				return false;
		}
		else if( not m_impl->parsing_key_value(curr_group, buf, line, errmsg) )
			return false;
	}
	return true;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::load()
{
	std::string errmsg;
	bool res = load(errmsg);
	if( not res )
		throw ini_exception(errmsg);
	return *this;
}

template <concept_char_type CharT>
bool basic_ini<CharT>::sync(std::string &errmsg) noexcept
{
	if( m_impl->file_name.empty() )
	{
		errmsg = std::format("File name is empty.");
		return false;
	}
	std::unique_lock locker(m_impl->m_rw_lock);
	std::basic_ofstream<CharT> file(m_impl->file_name, std::ios_base::out | std::ios_base::trunc);

	if( not file.is_open() )
	{
		errmsg = std::format("file '{}' open failed.", m_impl->file_name);
		return false;
	}
	for(auto &[group, keys] : *this)
	{
		file << detail::ini_keyword_char<CharT>::left_bracket
			 << to_percent_encoding(group)
			 << detail::ini_keyword_char<CharT>::right_bracket
			 << detail::ini_keyword_char<CharT>::line_break;

		for(auto &[key, value] : keys)
		{
			file << to_percent_encoding(key)
				 << detail::ini_keyword_char<CharT>::assigning
				 << to_percent_encoding(value.to_string())
				 << detail::ini_keyword_char<CharT>::line_break;
		}
	}
	file.close();
	return true;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::sync()
{
	std::string errmsg;
	bool res = sync(errmsg);
	if( not res )
		throw ini_exception(errmsg);
	return *this;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::set_sync_on_delete(bool enable) noexcept
{
	m_impl->m_sync_on_delete = enable;
}

template <concept_char_type CharT>
bool basic_ini<CharT>::sync_on_delete() const noexcept
{
	return m_impl->m_sync_on_delete;
}

template <concept_char_type CharT>
basic_ini<CharT>::iterator basic_ini<CharT>::find(const string_t &group) noexcept
{
	return m_impl->groups.find(group);
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::find(const string_t &group) const noexcept
{
	return m_impl->groups.find(group);
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::clear() noexcept
{
	m_impl->groups.clear();
	return *this;
}

template <concept_char_type CharT>
size_t basic_ini<CharT>::size() const noexcept
{
	return m_impl->groups.size();
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_INI_H
