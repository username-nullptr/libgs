
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
#include <libgs/core/log.h>

#include <filesystem>
#include <fstream>

namespace libgs
{

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini_keys<CharT>::read_or(const str_type &key, T default_value) const noexcept
{
	if constexpr( is_dsame_v<T, value_type> )
	{
		auto it = m_keys.find(key);
		if( it == m_keys.end() )
			return default_value;
		return value_type(it->second);
	}
	else
		return read<value_type>(key).template get<T>();
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini_keys<CharT>::read(const str_type &key) const noexcept(false)
{
	if constexpr( is_dsame_v<T, value_type> )
	{
		auto it = m_keys.find(key);
		if( it != m_keys.end() )
			return value_type(it->second);

		if constexpr( is_char_v<CharT> )
			throw ini_exception("basic_ini_keys: read: The key '{}' is not exists.", key);
		else
			throw ini_exception("basic_ini_keys: read: The key '{}' is not exists.", libgs::wcstombs(key));
	}
	else
		return read<value_type>(key).template get<T>();
}

template <concept_char_type CharT>
template <typename T>
basic_ini_keys<CharT> &basic_ini_keys<CharT>::write(const str_type &key, T &&value) noexcept
{
	m_keys[key] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini_keys<CharT> &basic_ini_keys<CharT>::write(str_type &&key, T &&value) noexcept
{
	m_keys[std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
basic_value<CharT> basic_ini_keys<CharT>::operator[](const str_type &key) const noexcept(false)
{
	return read<value_type>(key);
}

template <concept_char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](const str_type &key) noexcept
{
	return m_keys[key];
}

template <concept_char_type CharT>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](str_type &&key) noexcept
{
	return m_keys[std::move(key)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302 // TODO ...

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> basic_ini_keys<CharT>::operator[](const str_type &key, T default_value) const noexcept
{
	return read_or<value_type>(key, default_value);
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](const str_type &key, T default_value) noexcept
{
	return *m_keys.emplace(std::make_pair(key, default_value)).first;
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_value<CharT> &basic_ini_keys<CharT>::operator[](str_type &&key, T default_value) noexcept
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
basic_ini<CharT>::basic_ini(std::string file_name) :
	m_data(new data())
{
	if( file_name.empty() )
		return ;

	m_data->file_name = std::move(file_name);
	load();
}

template <concept_char_type CharT>
basic_ini<CharT>::~basic_ini()
{
	if( sync_on_delete() )
		sync();
}

template <concept_char_type CharT>
basic_ini<CharT>::basic_ini(basic_ini &&other) noexcept :
	m_data(other.m_data)
{
	other.m_data = new data();
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator=(basic_ini &&other) noexcept
{
	m_data = other.m_data;
	other.m_data = new data();
	return *this;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::global_instance()
{
	if constexpr( is_char_v )
		return ini_global_instance();
	else
		return wini_global_instance();
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::set_file_nmae(const std::string &file_name)
{
	auto _name = app::absolute_path(file_name);
	if( m_data->file_name == _name )
		return *this;

	m_data->file_name = _name;
	return load();
}

template <concept_char_type CharT>
std::string basic_ini<CharT>::file_name() const noexcept
{
	return m_data->file_name;
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini<CharT>::read_or(const str_type &group, const str_type &key, T default_value) const noexcept
{
	if constexpr( is_dsame_v<T, value_type> )
	{
		auto it = m_data->groups.find(group);
		if( it == m_data->groups.end() )
			return default_value;
		return it->second.template read_or<T>(key, default_value);
	}
	else
		return read<value_type>(group, key, default_value).template get<T>();
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
auto basic_ini<CharT>::read(const str_type &group, const str_type &key) const noexcept(false)
{
	if constexpr( is_dsame_v<T, value_type> )
	{
		auto it = m_data->groups.find(group);
		if( it != m_data->groups.end() )
			return it->second.template read<T>(key);

		if constexpr( is_char_v )
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", group);
		else
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", libgs::wcstombs(group));
	}
	else
		return read<value_type>(group, key).template get<T>();
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(const str_type &group, const str_type &key, T &&value) noexcept
{
	m_data->groups[group][key] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(const str_type &group, str_type &&key, T &&value) noexcept
{
	m_data->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(str_type &&group, const str_type &key, T &&value) noexcept
{
	m_data->groups[group][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
template <typename T>
basic_ini<CharT> &basic_ini<CharT>::write(str_type &&group, str_type &&key, T &&value) noexcept
{
	m_data->groups[std::move(group)][std::move(key)] = std::forward<T>(value);
	return *this;
}

template <concept_char_type CharT>
const basic_ini_keys<CharT> &basic_ini<CharT>::group(const str_type &group) const noexcept(false)
{
	auto it = m_data->groups.find(group);
	if( it != m_data->groups.end() )
		return it->second;

	if constexpr( is_char_v )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::group(const str_type &group) noexcept(false)
{
	auto it = m_data->groups.find(group);
	if( it != m_data->groups.end() )
		return it->second;

	if constexpr( is_char_v )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concept_char_type CharT>
const basic_ini_keys<CharT> &basic_ini<CharT>::operator[](const str_type &group) const noexcept(false)
{
	return this->group(group);
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::operator[](const str_type &group) noexcept
{
	return m_data->groups[group];
}

template <concept_char_type CharT>
basic_ini_keys<CharT> &basic_ini<CharT>::operator[](str_type &&group) noexcept
{
	return m_data->groups[std::move(group)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...

template <concept_char_type CharT>
basic_ini<CharT> basic_ini<CharT>::operator[](const str_type &group, const str_type &key) const noexcept(false)
{
	return (*this)[group][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](const str_type &group, const str_type &key) noexcept
{
	return (*this)[group][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](const str_type &group, str_type &&key) noexcept
{
	return (*this)[group][std::move(key)];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](str_type &&group, const str_type &key) noexcept
{
	return (*this)[std::move(group)][key];
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::operator[](str_type &&group, str_type &&key) noexcept
{
	return (*this)[std::move(group)][std::move(key)];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> basic_ini<CharT>::operator[](const str_type &group, const str_type &key, T default_value) const noexcept
{
	return read_or<value_type>(group, key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](const str_type &group, const str_type &key, T default_value) noexcept
{
	return (*this)[group][key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](const str_type &group, str_type &&key, T default_value) noexcept
{

	return (*this)[group][std::move(key), default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](str_type &&group, const str_type &key, T default_value) noexcept
{
	return (*this)[std::move(group)][key, default_value];
}

template <concept_char_type CharT>
template <ini_read_type<CharT> T>
basic_ini<CharT> &basic_ini<CharT>::operator[](str_type &&group, str_type &&key, T default_value) noexcept
{
	return (*this)[std::move(group)][std::move(key), default_value];
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concept_char_type CharT>
basic_ini<CharT>::iterator basic_ini<CharT>::begin() noexcept
{
	return m_data->groups.begin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::cbegin() const noexcept
{
	return m_data->groups.cbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::begin() const noexcept
{
	return m_data->groups.begin();
}

template <concept_char_type CharT>
basic_ini<CharT>::iterator basic_ini<CharT>::end() noexcept
{
	return m_data->groups.end();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::cend() const noexcept
{
	return m_data->groups.cend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_iterator basic_ini<CharT>::end() const noexcept
{
	return m_data->groups.end();
}

template <concept_char_type CharT>
basic_ini<CharT>::reverse_iterator basic_ini<CharT>::rbegin() noexcept
{
	return m_data->groups.rbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::crbegin() const noexcept
{
	return m_data->groups.crbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::rbegin() const noexcept
{
	return m_data->groups.rbegin();
}

template <concept_char_type CharT>
basic_ini<CharT>::reverse_iterator basic_ini<CharT>::rend() noexcept
{
	return m_data->groups.rend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::crend() const noexcept
{
	return m_data->groups.crend();
}

template <concept_char_type CharT>
basic_ini<CharT>::const_reverse_iterator basic_ini<CharT>::rend() const noexcept
{
	return m_data->groups.rend();
}

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

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::load()
{
	if( not std::filesystem::exists(m_data->file_name) )
		return *this;

	std::shared_lock locker(m_rw_lock);
	LIBGS_UNUSED(locker);

	std::basic_ifstream<CharT> file(m_data->file_name);
	if( not file.is_open() )
	{
		libgs_log_error("file '{}' open failed.", m_data->file_name);
		return *this;
	}
	str_type buf;
	str_type curr_group;

	for(size_t line=1; std::getline(file, buf); line++)
	{
		buf = str_trimmed(buf);
		if( buf.empty() or
		    buf[0] == ini_keyword_char<CharT>::sharp or
		    buf[0] == ini_keyword_char<CharT>::semicolon )
			continue;
		
		auto list = str_list_type::from_string(buf, ini_keyword_char<CharT>::sharp);
		buf = str_trimmed(list[0]);

		list = str_list_type::from_string(buf, ini_keyword_char<CharT>::semicolon);
		buf = str_trimmed(list[0]);

		if( buf.starts_with(ini_keyword_char<CharT>::left_bracket) )
		{
			curr_group = parsing_group(buf, line);	
			if( curr_group.empty() )
				return *this;
		}
		else if( not parsing_key_value(curr_group, buf, line) )
			return *this;
	}
	return *this;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::sync()
{
	if( m_data->file_name.empty() )
		return *this;

	std::unique_lock locker(m_rw_lock);
	std::basic_ofstream<CharT> file(m_data->file_name, std::ios_base::out | std::ios_base::trunc);

	if( not file.is_open() )
	{
		libgs_log_error("file '{}' open failed.", m_data->file_name);
		return *this;
	}
	for(auto &[group, keys] : *this)
	{
		file << ini_keyword_char<CharT>::left_bracket
			 << to_percent_encoding(group)
			 << ini_keyword_char<CharT>::right_bracket 
			 << ini_keyword_char<CharT>::line_break;

		for(auto &[key, value] : keys)
		{
			file << to_percent_encoding(key)
				 << ini_keyword_char<CharT>::assigning
				 << to_percent_encoding(value.to_string())
				 << ini_keyword_char<CharT>::line_break;
		}
	}
	file.close();
	return *this;
}

template <concept_char_type CharT>
basic_ini<CharT> &basic_ini<CharT>::set_sync_on_delete(bool enable) noexcept
{
	m_data->m_sync_on_delete = enable;
}

template <concept_char_type CharT>
bool basic_ini<CharT>::sync_on_delete() const noexcept
{
	return m_data->m_sync_on_delete;
}

template <concept_char_type CharT>
std::basic_string<CharT> basic_ini<CharT>::parsing_group(const str_type &str, size_t line)
{
	if( str.size() < 3 or not str.ends_with(ini_keyword_char<CharT>::right_bracket) )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
		return {};
	}
	auto group = from_percent_encoding(str_trimmed(str_type(str.c_str() + 1, str.size() - 2)));
	if( group.empty() )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid group.", line);
		return {};
	}
	m_data->groups[group];
	return group;
}

template <concept_char_type CharT>
bool basic_ini<CharT>::parsing_key_value(const str_type &curr_group, const str_type &str, size_t line)
{
	if( curr_group.empty() )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: No group specified.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: No group specified.", line);
		return false;
	}
	if( str.size() < 2 )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
		return false;
	}
	auto pos = str.find(ini_keyword_char<CharT>::assigning);
	if( pos == 0 )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
		return false;
	}
	else if( pos == str_type::npos )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid key-value line.", line);
		return false;
	}
	auto key = from_percent_encoding(str_trimmed(str.substr(0,pos)));
	if( key.empty() )
	{
		if constexpr( is_char_v )
			libgs_log_error("Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
		else
			libgs_log_werror(L"Ini file parsing: syntax error: [line:{}]: Invalid key-value line: Key is empty.", line);
		return false;
	}
	m_data->groups[curr_group][key] = from_percent_encoding(str_trimmed(str.substr(pos+1)));
	return true;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_INI_H
