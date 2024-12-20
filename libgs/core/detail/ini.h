
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
#include <libgs/core/execution.h>
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

class ini_error_category final : public std::error_category
{
	LIBGS_DISABLE_COPY_MOVE(ini_error_category)

public:
	constexpr explicit ini_error_category(const char *name, const char *desc) :
		m_name(name), m_desc(desc) {}

	[[nodiscard]] const char *name() const noexcept override {
		return m_name;
	}
	[[nodiscard]] std::string message(int line) const override {
		return std::format("Ini file parsing: syntax error: [line:{}]: {}.", line, m_desc);
	}

private:
	const char *m_name;
	const char *m_desc;
};

[[nodiscard]] LIBGS_CORE_API ini_error_category &ini_invalid_group() noexcept;
[[nodiscard]] LIBGS_CORE_API ini_error_category &ini_no_group_specified() noexcept;
[[nodiscard]] LIBGS_CORE_API ini_error_category &ini_invalid_key_value_line() noexcept;
[[nodiscard]] LIBGS_CORE_API ini_error_category &ini_key_is_empty() noexcept;
[[nodiscard]] LIBGS_CORE_API ini_error_category &ini_invalid_value() noexcept;

LIBGS_CORE_API void ini_commit_io_work(std::function<void()> work);

} //namespace detail

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
class LIBGS_CORE_TAPI basic_ini<CharT,IniKeys,Exec>::impl : public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)
	friend class basic_ini;

public:
	impl(const executor_t &exec, std::string_view file_name) :
		m_exec(exec), m_timer(exec)
	{
		set_file_name(file_name);
	}

	impl(impl&&) = default;
	impl& operator=(impl&&) = default;

	template <typename Exec0>
	impl(typename basic_ini<CharT,IniKeys,Exec0>::impl &&other) :
		m_exec(std::move(other.m_exec)),
		m_file_name(std::move(other.m_file_name)),
		m_groups(std::move(other.m_groups)),
		m_timer(std::move(other.m_timer)),
		m_sync_period(other.m_sync_period),
		m_sync_on_delete(other.m_sync_on_delete)
	{
		other.m_sync_period = milliseconds(0);
		other.m_sync_on_delete = false;
	}

	template <typename Exec0>
	impl &operator=(typename basic_ini<CharT,IniKeys,Exec0>::impl &&other)
	{
		m_exec = std::move(other.m_exec);
		m_file_name = std::move(other.m_file_name);
		m_groups = std::move(other.m_groups);
		m_sync_on_delete = other.m_sync_on_delete;
		other.m_sync_on_delete = false;
		return *this;
	}

public:
	void set_file_name(std::string_view file_name)
	{
		m_file_name = app::absolute_path(file_name);
	}

	void load(error_code &error)
	{
		error = error_code();
		if( not std::filesystem::exists(m_file_name) )
		{
			error = std::make_error_code(std::errc::no_such_file_or_directory);
			return ;
		}
		std::basic_ifstream<CharT> file;
		auto prev = file.exceptions();
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			file.open(m_file_name);
			string_t buf;
			string_t curr_group;

			for(size_t line=1;; line++)
			{
				if( file.peek() == EOF )
					break;

				std::getline(file, buf);
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
					curr_group = parsing_group(buf, line);
				else
					parsing_key_value(curr_group, buf, line);
			}
		}
		catch(const std::system_error &ex)
		{
			error = ex.code();
			return ;
		}
		file.exceptions(prev);
		file.close();
	}

	void sync(error_code &error)
	{
		std::basic_ofstream<CharT> file;
		auto prev = file.exceptions();
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			file.open(m_file_name, std::ios_base::out | std::ios_base::trunc);
			using keyword_char_t = detail::ini_keyword_char<CharT>;

			for(auto &[group, keys] : m_groups)
			{
				file << keyword_char_t::left_bracket
					 << to_percent_encoding(group)
					 << keyword_char_t::right_bracket
					 << keyword_char_t::line_break;

				for(auto &[key, value] : keys)
				{
					file << to_percent_encoding(key) << keyword_char_t::assigning;
					if( value.is_digit() )
						file << value.to_string();
					else
					{
						file << keyword_char_t::double_quotes
						     << to_percent_encoding(value.to_string())
						     << keyword_char_t::double_quotes;
					}
					file << keyword_char_t::line_break;
				}
				file << keyword_char_t::line_break;
			}
		}
		catch(const std::system_error &ex)
		{
			error = ex.code();
			return ;
		}
		file.exceptions(prev);
		file.close();
	}

	template <typename Rep, typename Period>
	void set_sync_period(const duration<Rep,Period> &period)
	{
		using namespace std::chrono;
		auto msec = std::chrono::duration_cast<milliseconds>(period);

		if( m_sync_period != 0ms and msec == 0ms )
			m_timer.cancel();
		else if( m_sync_period == 0ms and msec != 0ms )
		{
			dispatch([self = this->shared_from_this()]() -> awaitable<void>
			{
				error_code error;
				while( not error )
				{
					self->m_timer.expires_after(self->m_sync_period);
					using namespace operators;

					co_await self->m_timer.async_wait(use_awaitable | error);
					if( error )
						break;
					detail::ini_commit_io_work([self]
					{
						error_code error; LIBGS_UNUSED(error);
						self->sync(error);
					});
				}
				co_return ;
			});
		}
		m_sync_period = std::move(msec);
	}

private:
	[[nodiscard]] string_t parsing_group(const string_t &str, size_t line)
	{
		if( str.size() < 3 or not str.ends_with(detail::ini_keyword_char<CharT>::right_bracket) )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_group()),
				"libgs::basic_ini"
			);
		}
		auto group = from_percent_encoding(str_trimmed(string_t(str.c_str() + 1, str.size() - 2)));
		if( group.empty() )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_group()),
				"libgs::basic_ini"
			);
		}
		m_groups[group];
		return group;
	}

	void parsing_key_value(const string_t &curr_group, const string_t &str, size_t line)
	{
		using keyword_char = detail::ini_keyword_char<CharT>;
		if( curr_group.empty() )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_no_group_specified()),
				"libgs::basic_ini"
			);
		}
		else if( str.size() < 2 )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_key_value_line()),
				"libgs::basic_ini"
			);
		}
		auto pos = str.find(keyword_char::assigning);
		if( pos == 0 )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_key_is_empty()),
				"libgs::basic_ini"
			);
		}
		else if( pos == string_t::npos )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_key_value_line()),
				"libgs::basic_ini"
			);
		}
		auto key = from_percent_encoding(str_trimmed(str.substr(0,pos)));
		if( key.empty() )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_key_is_empty()),
				"libgs::basic_ini"
			);
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
		if( value.size() == 1 )
		{
			if( value[0] == keyword_char::assigning or
			    value[0] == keyword_char::single_quotes or
			    value[0] == keyword_char::double_quotes )
			{
				throw system_error (
					std::error_code(static_cast<int>(line), detail::ini_invalid_value()),
					"libgs::basic_ini"
				);
			}
			return ;
		}
		else if( value[0] == keyword_char::single_quotes or value[0] == keyword_char::double_quotes )
		{
			if( value.back() != value[0] )
			{
				throw system_error (
					std::error_code(static_cast<int>(line), detail::ini_invalid_value()),
					"libgs::basic_ini"
				);
			}
			value = value.substr(1, value.size() - 2);
		}
		m_groups[curr_group][key] = from_percent_encoding(value);
	}

public:
	executor_t m_exec;
	std::string m_file_name;
	group_map m_groups {};

	asio::steady_timer m_timer;
	milliseconds m_sync_period {0};
	bool m_sync_on_delete = false;
};

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec>::basic_ini
(concepts::match_execution_context<executor_t> auto &exec, std::string_view file_name) :
	basic_ini(exec.get_executor(), file_name)
{

}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec>::basic_ini(const executor_t &exec, std::string_view file_name) :
	m_impl(new impl(exec, file_name))
{

}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec>::basic_ini(std::string_view file_name) :
	basic_ini(execution::context(), file_name)
{

}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec>::~basic_ini()
{
	if( not sync_on_delete() )
		return ;

	error_code error; LIBGS_UNUSED(error);
	sync(error);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec>::basic_ini(basic_ini &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
basic_ini<CharT,IniKeys,Exec> &basic_ini<CharT,IniKeys,Exec>::operator=(basic_ini &&other) noexcept
{
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename Exec0>
basic_ini<CharT,IniKeys,Exec>::basic_ini(basic_ini<char_t,IniKeys,Exec0> &&other)
	requires concepts::match_execution<Exec0,executor_t> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename Exec0>
basic_ini<CharT,IniKeys,Exec> &basic_ini<CharT,IniKeys,Exec>::operator=(basic_ini<char_t,IniKeys,Exec0> &&other)
	requires concepts::match_execution<Exec0,executor_t>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
void basic_ini<CharT,IniKeys,Exec>::set_file_name(std::string_view file_name)
{
	m_impl->set_file_name(file_name);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
std::string_view basic_ini<CharT,IniKeys,Exec>::file_name() const noexcept
{
	return m_impl->m_file_name;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
auto basic_ini<CharT,IniKeys,Exec>::read_or(const string_t &group, const string_t &key, T default_value) const noexcept
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_impl->m_groups.find(group);
		if( it == m_impl->m_groups.end() )
			return default_value;
		return it->second.template read_or<T>(key, default_value);
	}
	else
		return read<value_t>(group, key, default_value).template get<T>();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
auto basic_ini<CharT,IniKeys,Exec>::read(const string_t &group, const string_t &key) const noexcept(false)
{
	if constexpr( is_dsame_v<T, value_t> )
	{
		auto it = m_impl->m_groups.find(group);
		if( it != m_impl->m_groups.end() )
			return it->second.template read<T>(key);

		if constexpr( is_char_v<CharT> )
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", group);
		else
			throw ini_exception("basic_ini: read: The group '{}' is not exists.", libgs::wcstombs(group));
	}
	else
		return read<value_t>(group, key).template get<T>();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t
basic_ini<CharT,IniKeys,Exec>::read(const string_t &group, const string_t &key) const
{
	return read<value_t>(group, key);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename T>
void basic_ini<CharT,IniKeys,Exec>::write(const string_t &group, const string_t &key, T &&value) noexcept
{
	m_impl->m_groups[group][key] = std::forward<T>(value);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename T>
void basic_ini<CharT,IniKeys,Exec>::write(const string_t &group, string_t &&key, T &&value) noexcept
{
	m_impl->m_groups[group][std::move(key)] = std::forward<T>(value);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename T>
void basic_ini<CharT,IniKeys,Exec>::write(string_t &&group, const string_t &key, T &&value) noexcept
{
	m_impl->m_groups[group][std::move(key)] = std::forward<T>(value);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename T>
void basic_ini<CharT,IniKeys,Exec>::write(string_t &&group, string_t &&key, T &&value) noexcept
{
	m_impl->m_groups[std::move(group)][std::move(key)] = std::forward<T>(value);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
const typename basic_ini<CharT,IniKeys,Exec>::ini_keys_t&
basic_ini<CharT,IniKeys,Exec>::group(const string_t &group) const
{
	auto it = m_impl->m_groups.find(group);
	if( it != m_impl->m_groups.end() )
		return it->second;

	if constexpr( is_char_v<CharT> )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::ini_keys_t&
basic_ini<CharT,IniKeys,Exec>::group(const string_t &group)
{
	auto it = m_impl->m_groups.find(group);
	if( it != m_impl->m_groups.end() )
		return it->second;

	if constexpr( is_char_v<CharT> )
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", group);
	else
		throw ini_exception("basic_ini: group: The group '{}' is not exists.", libgs::wcstombs(group));
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
const typename basic_ini<CharT,IniKeys,Exec>::ini_keys_t&
basic_ini<CharT,IniKeys,Exec>::operator[](const string_t &group) const
{
	return this->group(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::ini_keys_t&
basic_ini<CharT,IniKeys,Exec>::operator[](const string_t &group) noexcept
{
	return m_impl->m_groups[group];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::ini_keys_t&
basic_ini<CharT,IniKeys,Exec>::operator[](string_t &&group) noexcept
{
	return m_impl->m_groups[std::move(group)];
}

#if LIBGS_CORE_CPLUSPLUS >= 202302L // TODO ...

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t basic_ini<CharT,IniKeys,Exec>::operator[]
(const string_t &group, const string_t &key) const noexcept(false)
{
	return (*this)[group][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key) noexcept
{
	return (*this)[group][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, string_t &&key) noexcept
{
	return (*this)[group][std::move(key)];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, const string_t &key) noexcept
{
	return (*this)[std::move(group)][key];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, string_t &&key) noexcept
{
	return (*this)[std::move(group)][std::move(key)];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys,Exec>::value_t basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key, T default_value) const noexcept
{
	return read_or<value_t>(group, key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, const string_t &key, T default_value) noexcept
{
	return (*this)[group][key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(const string_t &group, string_t &&key, T default_value) noexcept
{

	return (*this)[group][std::move(key), default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, const string_t &key, T default_value) noexcept
{
	return (*this)[std::move(group)][key, default_value];
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::ini_read<CharT> T>
typename basic_ini<CharT,IniKeys,Exec>::value_t &basic_ini<CharT>::operator[]
(string_t &&group, string_t &&key, T default_value) noexcept
{
	return (*this)[std::move(group)][std::move(key), default_value];
}

#endif // LIBGS_CORE_CPLUSPLUS >= 202302L

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::iterator basic_ini<CharT,IniKeys,Exec>::begin() noexcept
{
	return m_impl->m_groups.begin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_iterator basic_ini<CharT,IniKeys,Exec>::cbegin() const noexcept
{
	return m_impl->m_groups.cbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_iterator basic_ini<CharT,IniKeys,Exec>::begin() const noexcept
{
	return m_impl->m_groups.begin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::iterator basic_ini<CharT,IniKeys,Exec>::end() noexcept
{
	return m_impl->m_groups.end();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_iterator basic_ini<CharT,IniKeys,Exec>::cend() const noexcept
{
	return m_impl->m_groups.cend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_iterator basic_ini<CharT,IniKeys,Exec>::end() const noexcept
{
	return m_impl->m_groups.end();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::reverse_iterator basic_ini<CharT,IniKeys,Exec>::rbegin() noexcept
{
	return m_impl->m_groups.rbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_reverse_iterator basic_ini<CharT,IniKeys,Exec>::crbegin() const noexcept
{
	return m_impl->m_groups.crbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_reverse_iterator basic_ini<CharT,IniKeys,Exec>::rbegin() const noexcept
{
	return m_impl->m_groups.rbegin();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::reverse_iterator basic_ini<CharT,IniKeys,Exec>::rend() noexcept
{
	return m_impl->m_groups.rend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_reverse_iterator basic_ini<CharT,IniKeys,Exec>::crend() const noexcept
{
	return m_impl->m_groups.crend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_reverse_iterator basic_ini<CharT,IniKeys,Exec>::rend() const noexcept
{
	return m_impl->m_groups.rend();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::opt_token<error_code> Token>
auto basic_ini<CharT,IniKeys,Exec>::load(Token &&token)
{
	if constexpr( std::is_same_v<Token,error_code&> )
		m_impl->load(token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		load(error);
		if( error )
			throw system_error(error, "libgs::basic_ini::load");
	}
	else
	{
		return async<error_code>(get_executor(), [this](auto handle, auto exec) mutable
		{
			using handle_t = std::remove_cvref_t<decltype(handle)>;
			detail::ini_commit_io_work(
			[this, impl = m_impl, handle = std::make_shared<handle_t>(std::move(handle)), exec]() mutable
			{
				LIBGS_UNUSED(impl);
				error_code error;
				load(error);
				dispatch(exec, [handle = std::move(handle), error]() mutable {
					std::move(*handle)(error);
				});
			});
		},
		std::forward<Token>(token));
	}
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <concepts::opt_token<error_code> Token>
auto basic_ini<CharT,IniKeys,Exec>::sync(Token &&token)
{
	if constexpr( std::is_same_v<Token,error_code&> )
		m_impl->sync(token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		sync(error);
		if( error )
			throw system_error(error, "libgs::basic_ini::sync");
	}
	else
	{
		return async<error_code>(get_executor(), [this](auto handle, auto exec) mutable
		{
			using handle_t = std::remove_cvref_t<decltype(handle)>;
			detail::ini_commit_io_work(
			[this, impl = m_impl, handle = std::make_shared<handle_t>(std::move(handle)), exec]() mutable
			{
				LIBGS_UNUSED(impl);
				error_code error;
				sync(error);
				dispatch(exec, [handle = std::move(handle), error]() mutable {
					std::move(*handle)(error);
				});
			});
		},
		std::forward<Token>(token));
	}
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
template <typename Rep, typename Period>
void basic_ini<CharT,IniKeys,Exec>::set_sync_period(const duration<Rep,Period> &period)
{
	m_impl->set_sync_period(period);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
milliseconds basic_ini<CharT,IniKeys,Exec>::sync_period() const noexcept
{
	return m_impl->m_sync_period;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
void basic_ini<CharT,IniKeys,Exec>::set_sync_on_delete(bool enable) noexcept
{
	m_impl->m_sync_on_delete = enable;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
bool basic_ini<CharT,IniKeys,Exec>::sync_on_delete() const noexcept
{
	return m_impl->m_sync_on_delete;
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::iterator basic_ini<CharT,IniKeys,Exec>::find(const string_t &group) noexcept
{
	return m_impl->m_groups.find(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::const_iterator basic_ini<CharT,IniKeys,Exec>::find(const string_t &group) const noexcept
{
	return m_impl->m_groups.find(group);
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
void basic_ini<CharT,IniKeys,Exec>::clear() noexcept
{
	m_impl->m_groups.clear();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
size_t basic_ini<CharT,IniKeys,Exec>::size() const noexcept
{
	return m_impl->m_groups.size();
}

template <concepts::char_type CharT, concepts::base_of_basic_ini_keys<CharT> IniKeys, concepts::execution Exec>
typename basic_ini<CharT,IniKeys,Exec>::executor_t basic_ini<CharT,IniKeys,Exec>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_INI_H
