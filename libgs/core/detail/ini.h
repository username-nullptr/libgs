
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
#include <fstream>

namespace libgs { namespace detail
{

template <concepts::character CharT>
[[nodiscard]] LIBGS_CORE_TAPI std::basic_string<CharT>
ini_replace(const concepts::text_p<CharT> auto &text)
{
	return strtls::replace(strtls::to_string(text),
		static_cast<CharT>(' '), static_cast<CharT>('_')
	);
}

} //namespace detail

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
optional<basic_value<CharT>> basic_ini_keys<CharT,Map,MapArgs...>::read
(const concepts::text_p<char_t> auto &key) const noexcept
{
	auto it = m_keys.find(detail::ini_replace<char_t>(key));
	return it == m_keys.end() ? optional<value_t>() : libgs::make_optional(it->second);
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini_keys<CharT,Map,MapArgs...>::write
(const concepts::text_p<char_t> auto &key, concepts::value_set<char_t> auto &&value) noexcept
{
	m_keys[detail::ini_replace<char_t>(key)] = std::forward<decltype(value)>(value);
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
optional<basic_value<CharT>> basic_ini_keys<CharT,Map,MapArgs...>::operator[]
(const concepts::text_p<char_t> auto &key) const noexcept
{
	return read(key);
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_value<CharT> &basic_ini_keys<CharT,Map,MapArgs...>::operator[]
(const concepts::text_p<char_t> auto &key) noexcept
{
	return m_keys[detail::ini_replace<char_t>(key)];
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::iterator
basic_ini_keys<CharT,Map,MapArgs...>::begin() noexcept
{
	return m_keys.begin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_iterator
basic_ini_keys<CharT,Map,MapArgs...>::cbegin() const noexcept
{
	return m_keys.cbegin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_iterator
basic_ini_keys<CharT,Map,MapArgs...>::begin() const noexcept
{
	return m_keys.begin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::iterator
basic_ini_keys<CharT,Map,MapArgs...>::end() noexcept
{
	return m_keys.end();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_iterator
basic_ini_keys<CharT,Map,MapArgs...>::cend() const noexcept
{
	return m_keys.cend();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_iterator
basic_ini_keys<CharT,Map,MapArgs...>::end() const noexcept
{
	return m_keys.end();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::rbegin() noexcept
{
	return m_keys.rbegin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::crbegin() const noexcept
{
	return m_keys.crbegin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::rbegin() const noexcept
{
	return m_keys.rbegin();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::rend() noexcept
{
	return m_keys.rend();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::crend() const noexcept
{
	return m_keys.crend();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_reverse_iterator
basic_ini_keys<CharT,Map,MapArgs...>::rend() const noexcept
{
	return m_keys.rend();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::iterator
basic_ini_keys<CharT,Map,MapArgs...>::find(const concepts::text_p<char_t> auto &key) noexcept
{
	return m_keys.find(detail::ini_replace<char_t>(key));
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
basic_ini_keys<CharT,Map,MapArgs...>::const_iterator
basic_ini_keys<CharT,Map,MapArgs...>::find(const concepts::text_p<char_t> auto &key) const noexcept
{
	return m_keys.find(detail::ini_replace<char_t>(key));
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini_keys<CharT,Map,MapArgs...>::clear() noexcept
{
	m_keys.clear();
}

template <concepts::character CharT, template <typename,typename,typename...> class Map, typename...MapArgs>
size_t basic_ini_keys<CharT,Map,MapArgs...>::size() const noexcept
{
	return m_keys.size();
}

namespace detail
{

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

[[nodiscard]] LIBGS_CORE_API std::filesystem::path ini_tmp_file(const std::filesystem::path &file_name);
LIBGS_CORE_API void ini_commit_io_work(std::function<void()> work);

} //namespace detail

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
class LIBGS_CORE_TAPI basic_ini<CharT,Exec,Map,MapArgs...>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY(impl)
	friend class basic_ini;

public:
	impl(const auto &exec, const path_t &file_name) :
		m_exec(exec), m_timer(exec)
	{
		set_file_name(std::move(file_name));
	}

	impl(impl&&) = default;
	impl& operator=(impl&&) = default;

	template <typename Exec0>
	impl(typename basic_ini<char_t,Exec0,Map,MapArgs...>::impl &&other) :
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
	impl &operator=(typename basic_ini<char_t,Exec0,Map,MapArgs...>::impl &&other)
	{
		m_exec = std::move(other.m_exec);
		m_file_name = std::move(other.m_file_name);
		m_groups = std::move(other.m_groups);
		m_sync_on_delete = other.m_sync_on_delete;
		other.m_sync_on_delete = false;
		return *this;
	}

public:
	[[nodiscard]] static auto replace(const auto &text) {
		return detail::ini_replace<char_t>(text);
	}

	void set_file_name(const path_t &file_name)
	{
		if( not file_name.empty() )
			m_file_name = *app::absolute_path(file_name).or_else();
	}

public:
	// It may be executed within the thread, so the const modifier provides protection.
	[[nodiscard]] data_t load(auto &error, const std::function<bool()> &cancelled) const
	{
		data_t data;
		error = std::error_code();
		if( not exists(m_file_name) )
		{
			error = std::make_error_code(std::errc::no_such_file_or_directory);
			return data;
		}
		std::basic_ifstream<char_t> file;
		auto prev = file.exceptions();
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			file.open(m_file_name);
			unit_data_t *curr_group = nullptr;
			string_t buf;

			for(size_t line=1;; line++)
			{
				if( cancelled() )
				{
					error = make_error_code(asio::error::operation_aborted);
					return data;
				}
				else if( file.eof() or file.peek() == EOF )
					break;

				std::getline(file, buf);
				buf = strtls::trimmed(buf);

				if( buf.empty() or buf[0] == static_cast<char_t>('#') or buf[0] == static_cast<char_t>(';') )
					continue;

				auto list = string_vector_t::from_string(buf, static_cast<char_t>('#'));
				buf = strtls::trimmed(list[0]);

				list = string_vector_t::from_string(buf, static_cast<char_t>(';'));
				buf = strtls::trimmed(list[0]);

				if( buf.starts_with(static_cast<char_t>('[')) )
					curr_group = &data[parsing_group(buf, line)];
				else
				{
					auto [key, value] = parsing_key_value(buf, line);
					if( not curr_group )
					{
						throw system_error (
							std::error_code(static_cast<int>(line), detail::ini_no_group_specified()),
							"libgs::basic_ini"
						);
					}
					(*curr_group)[std::move(key)] = std::move(value);
				}
			}
		}
		catch(const std::system_error &ex)
		{
			error = ex.code();
			data.clear();
		}
		file.exceptions(prev);
		file.close();
		return data;
	}

	// It may be executed within the thread, so the const modifier provides protection.
	void sync(data_t data, error_code &error, const std::function<bool()> &cancelled) const
	{
		std::basic_ofstream<char_t> file;
		auto file_name = detail::ini_tmp_file(m_file_name);
		auto prev = file.exceptions();
		file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			file.open(file_name, std::ios_base::out | std::ios_base::trunc);
			for(auto &[group, values] : data)
			{
				if( cancelled() )
				{
					error = make_error_code(asio::error::operation_aborted);
					try {
						file.close();
						std::filesystem::remove(file_name);
					}
					catch(...) {}
					return ;
				}
				file << l_str(char_t,"[")
					 << (strtls::is_ascii(group) ? group : to_percent_encoding(group))
					 << l_str(char_t,"]")
					 << l_str(char_t,"\n");

				for(auto &[key, value] : values)
				{
					if( key.empty() or value->empty() )
						continue;

					file << (strtls::is_ascii(key) ? key : to_percent_encoding(key))
						 << l_str(char_t,"=");

					if( value.is_rlnum() )
					{
						if( value->front() == 0x2B/*+*/ )
							value = value->substr(1);
						file << value.to_string();
					}
					else
					{
						file << l_str(char_t,"\"")
						     << (value.is_ascii() ? value.to_string() : to_percent_encoding(value.to_string()))
						     << l_str(char_t,"\"");
					}
					file << l_str(char_t,"\n");
				}
				file << l_str(char_t,"\n");
			}
		}
		catch(const std::system_error &ex)
		{
			error = ex.code();
			return ;
		}
		file.exceptions(prev);
		file.close();
		std::filesystem::rename(file_name, m_file_name, error);
	}

public:
	[[nodiscard]] std::pair<string_t,string_t> from_path(std::basic_string_view<char_t> path, const char *func)
	{
		string_vector_t str_list;
		str_list = string_vector_t::from_string(path, static_cast<char_t>('/'));

		if( str_list.size() != 2 )
		{
			throw runtime_error("libgs::basic_ini: {}: The path '{}' is invalid.",
				func, strtls::detail::ascii_transition<char>(path)
			);
		}
		return std::make_pair(str_list[0], str_list[1]);
	}

	template <typename Rep, typename Period>
	void set_sync_period(const duration<Rep,Period> &period)
	{
		using namespace std::chrono;
		auto msec = std::chrono::duration_cast<milliseconds>(period);
		if( msec == m_sync_period )
			return ;

		m_timer.cancel();
		m_sync_period = std::move(msec);

		if( msec == 0ms )
			return ;

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
				detail::ini_commit_io_work([self, data = self->data()]
				{
					error_code error; LIBGS_UNUSED(error);
					self->sync(std::move(data), error, []{return false;});
				});
			}
			co_return ;
		});
	}

public:
	void set_data(data_t data)
	{
		for(auto &[group, values] : data)
		{
			for(auto &[key, value] : values)
				m_groups[std::move(group)][std::move(key)] = std::move(value);
		}
	}

	[[nodiscard]] data_t data() const
	{
		data_t data;
		for(auto &[group, values] : m_groups)
		{
			for(auto &[key, value] : values)
				data[group][key] = value;
		}
		return data;
	}

private:
	[[nodiscard]] string_t parsing_group(const string_t &str, size_t line) const
	{
		if( str.size() < 3 or not str.ends_with(static_cast<char_t>(']')) )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_group()),
				"libgs::basic_ini"
			);
		}
		auto group = from_percent_encoding (
			strtls::trimmed(string_t(str.c_str() + 1, str.size() - 2))
		);
		if( group.empty() )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_group()),
				"libgs::basic_ini"
			);
		}
		return detail::ini_replace<char_t>(group);
	}

	[[nodiscard]] std::pair<string_t,value_t> parsing_key_value(const string_t &str, size_t line) const
	{
		if( str.size() < 2 )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_invalid_key_value_line()),
				"libgs::basic_ini"
			);
		}
		auto pos = str.find(static_cast<char_t>('='));
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
		auto key = from_percent_encoding(strtls::trimmed(str.substr(0,pos)));
		if( key.empty() )
		{
			throw system_error (
				std::error_code(static_cast<int>(line), detail::ini_key_is_empty()),
				"libgs::basic_ini"
			);
		}
		auto value = strtls::trimmed(str.substr(pos+1));
		{
			int i = 0;
			for(; i<static_cast<int>(value.size()); i++)
			{
				if( value[i] != static_cast<char_t>('=') )
					break;
			}
			if( --i >= 0 )
				value = value.substr(0,i);
		}
		if( value.size() == 1 )
		{
			if( value[0] == static_cast<char_t>('=') or
			    value[0] == static_cast<char_t>('\'') or
			    value[0] == static_cast<char_t>('"') )
			{
				throw system_error (
					std::error_code(static_cast<int>(line), detail::ini_invalid_value()),
					"libgs::basic_ini"
				);
			}
		}
		else if( value[0] == static_cast<char_t>('\'') or value[0] == static_cast<char_t>('"') )
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
		return std::pair<string_t,value_t>(
			detail::ini_replace<char_t>(key), from_percent_encoding(value)
		);
	}

public:
	executor_t m_exec {};
	path_t m_file_name {};
	group_map_t m_groups {};

	asio::steady_timer m_timer;
	milliseconds m_sync_period {0};
	bool m_sync_on_delete = false;

	std::vector<std::shared_ptr<bool>> m_cancel_vector {};
};

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::group_key::group_key
(const concepts::text_p<char_t> auto &group, const concepts::text_p<char_t> auto &key) noexcept :
	group(impl::replace(group)), key(impl::replace(key))
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <concepts::text<CharT> Text0, concepts::text<CharT> Text1>
basic_ini<CharT,Exec,Map,MapArgs...>::group_key::group_key(const std::pair<Text0,Text1> &pair) noexcept :
	group(impl::replace(pair.first)), key(impl::replace(pair.second))
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <concepts::text<CharT> Text0, concepts::text<CharT> Text1>
basic_ini<CharT,Exec,Map,MapArgs...>::group_key::group_key(const std::tuple<Text0,Text1> &tuple) noexcept :
	group(impl::replace(std::get<0>(tuple))), key(impl::replace(std::get<1>(tuple)))
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini
(concepts::match_exec_context<executor_t> auto &exec, const path_t &file_name) :
	basic_ini(exec.get_executor(), file_name)
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini
(const concepts::match_exec<executor_t> auto &exec, const path_t &file_name)
{
	m_impl = std::make_shared<impl>(exec, file_name);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini(const path_t &file_name)
	requires concepts::match_def_exec<executor_t> :
	basic_ini(io_context(), file_name)
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini
(concepts::match_exec_context<executor_t> auto &exec, data_t data, const path_t &file_name) :
	basic_ini(exec.get_executor(), std::move(data), file_name)
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini
(const concepts::match_exec<executor_t> auto &exec, data_t data, const path_t &file_name)
{
	m_impl = std::make_shared<impl>(exec, file_name);
	set_data(std::move(data));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini(data_t data, const path_t &file_name)
	requires concepts::match_def_exec<executor_t> :
	basic_ini(io_context(), std::move(data), file_name)
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::~basic_ini()
{
	if( not sync_on_delete() )
		return ;

	error_code error; LIBGS_UNUSED(error);
	sync(error);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini(basic_ini &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = std::make_shared<impl>();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
basic_ini<CharT,Exec,Map,MapArgs...> &basic_ini<CharT,Exec,Map,MapArgs...>::operator=(basic_ini &&other) noexcept
{
	if( this == &other )
		return *this;
	m_impl = other.m_impl;
	other.m_impl = std::make_shared<impl>();
	return *this;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <typename Exec0>
basic_ini<CharT,Exec,Map,MapArgs...>::basic_ini(basic_ini<char_t,Exec0,map_temp,MapArgs...> &&other)
	requires concepts::match_exec<Exec0,executor_t> :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <typename Exec0>
basic_ini<CharT,Exec,Map,MapArgs...>&
basic_ini<CharT,Exec,Map,MapArgs...>::operator=(basic_ini<char_t,Exec0,map_temp,MapArgs...> &&other)
	requires concepts::match_exec<Exec0,executor_t>
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::set_file_name(const path_t &file_name)
{
	m_impl->set_file_name(file_name);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::path_t
basic_ini<CharT,Exec,Map,MapArgs...>::file_name() const noexcept
{
	return m_impl->m_file_name;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
optional<basic_value<CharT>> basic_ini<CharT,Exec,Map,MapArgs...>::read
(const group_key &gk) const noexcept
{
	auto it = m_impl->m_groups.find(gk.group);
	return it == m_impl->m_groups.end() ?
		optional<value_t>() : it->second.read(std::move(gk.key));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
optional<basic_value<CharT>> basic_ini<CharT,Exec,Map,MapArgs...>::read
(const concepts::string_p<char_t> auto &path) const noexcept
{
	return read(m_impl->from_path(path, "read"));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::write
(group_key gk, concepts::value_set<char_t> auto &&value) noexcept
{
	m_impl->m_groups[std::move(gk.group)].write (
		std::move(gk.key), std::forward<decltype(value)>(value)
	);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::write
(const concepts::string_p<char_t> auto &path, concepts::value_set<char_t> auto &&value) noexcept
{
	auto pair = m_impl->from_path(path, "write");
	write(std::move(pair), std::forward<decltype(value)>(value));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
const typename basic_ini<CharT,Exec,Map,MapArgs...>::ini_keys_t&
basic_ini<CharT,Exec,Map,MapArgs...>::group(const concepts::text_p<char_t> auto &group) const
{
	return remove_const(this)->group(group);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::ini_keys_t&
basic_ini<CharT,Exec,Map,MapArgs...>::group(const concepts::text_p<char_t> auto &group)
{
	auto it = m_impl->m_groups.find(impl::replace(group));
	if( it != m_impl->m_groups.end() )
	{
		throw runtime_error("basic_ini: group: The group '{}' is not exists.",
			strtls::detail::ascii_transition<char>(std::forward<decltype(group)>(group))
		);
	}
	return it->second;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
const typename basic_ini<CharT,Exec,Map,MapArgs...>::ini_keys_t&
basic_ini<CharT,Exec,Map,MapArgs...>::operator[](const concepts::text_p<char_t> auto &group) const
{
	return this->group(group);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::ini_keys_t&
basic_ini<CharT,Exec,Map,MapArgs...>::operator[](const concepts::text_p<char_t> auto &group) noexcept
{
	return m_impl->m_groups[impl::replace(group)];
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::value_t
basic_ini<CharT,Exec,Map,MapArgs...>::operator[](const group_key &gk) const
{
	return (*this)[gk.group][gk.key];
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::value_t&
basic_ini<CharT,Exec,Map,MapArgs...>::operator[](group_key gk) noexcept
{
	return (*this)[std::move(gk.group)][std::move(gk.key)];
}

#if LIBGS_CPLUSPLUS >= 202100L

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::value_t basic_ini<CharT,Exec,Map,MapArgs...>::operator[]
(const concepts::text_p<char_t> auto &group, const concepts::text_p<char_t> auto &key) const
{
	return (*this)[group][key];
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::value_t &basic_ini<CharT,Exec,Map,MapArgs...>::operator[]
(concepts::text_p<char_t> auto &&group, concepts::text_p<char_t> auto &&key) noexcept
{
	return (*this)[std::forward<decltype(group)>(group)][std::forward<decltype(key)>(key)];
}

#endif //LIBGS_CPLUSPLUS

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::iterator
basic_ini<CharT,Exec,Map,MapArgs...>::begin() noexcept
{
	return m_impl->m_groups.begin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::cbegin() const noexcept
{
	return m_impl->m_groups.cbegin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::begin() const noexcept
{
	return m_impl->m_groups.begin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::iterator
basic_ini<CharT,Exec,Map,MapArgs...>::end() noexcept
{
	return m_impl->m_groups.end();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::cend() const noexcept
{
	return m_impl->m_groups.cend();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::end() const noexcept
{
	return m_impl->m_groups.end();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::rbegin() noexcept
{
	return m_impl->m_groups.rbegin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::crbegin() const noexcept
{
	return m_impl->m_groups.crbegin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::rbegin() const noexcept
{
	return m_impl->m_groups.rbegin();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::rend() noexcept
{
	return m_impl->m_groups.rend();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::crend() const noexcept
{
	return m_impl->m_groups.crend();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_reverse_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::rend() const noexcept
{
	return m_impl->m_groups.rend();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <concepts::opt_token<error_code> Token>
auto basic_ini<CharT,Exec,Map,MapArgs...>::load(Token &&token)
{
	std::function<bool()> cancelled = []{
		return false;
	};
	if constexpr( is_error_code_token_v<Token> )
		set_data(m_impl->load(token, std::move(cancelled)));

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		load(error);
		if( error )
			throw system_error(error, "libgs::basic_ini::load");
	}
	else
	{
		auto cflag = std::make_shared<bool>(false);
		m_impl->m_cancel_vector.emplace_back(cflag);

		auto slot = asio::get_associated_cancellation_slot(token);
		cancelled = [cflag = std::move(cflag), state = asio::cancellation_state(slot)]{
			return *cflag or state.cancelled() != asio::cancellation_type::none;
		};
		return async_work<error_code>::handle(get_executor(),
		[impl = m_impl, cancelled = std::move(cancelled)](auto handle, auto exec) mutable
		{
			using handle_t = std::remove_cvref_t<decltype(handle)>;
			detail::ini_commit_io_work([
				impl, cancelled = std::move(cancelled),
				handle = std::make_shared<handle_t>(std::move(handle)), exec
			]() mutable
			{
				error_code error;
				auto data = impl->load(error, cancelled); // !!! thread
				dispatch(exec, [impl, data = std::move(data), handle = std::move(handle), error]() mutable
				{
					impl->set_data(std::move(data));
					std::move(*handle)(error);
				});
			});
		},
		std::forward<Token>(token));
	}
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <concepts::opt_token<error_code> Token>
auto basic_ini<CharT,Exec,Map,MapArgs...>::load_or(Token &&token)
{
	if( exists(file_name()) )
		return load(std::forward<Token>(token));

	if constexpr( is_async_opt_token_v<Token> )
	{
		return async_work<error_code>::handle(get_executor(), [this](auto handle, auto exec) mutable
		{
			using handle_t = std::remove_cvref_t<decltype(handle)>;
			dispatch(exec, [handle = std::make_shared<handle_t>(std::move(handle))]() mutable {
				std::move(*handle)(std::error_code());
			});
		},
		std::forward<Token>(token));
	}
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <concepts::opt_token<error_code> Token>
auto basic_ini<CharT,Exec,Map,MapArgs...>::sync(Token &&token)
{
	std::function<bool()> cancelled = []{
		return false;
	};
	if constexpr( std::is_same_v<Token,error_code&> )
		m_impl->sync(data(), token, std::move(cancelled));

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		sync(error);
		if( error )
			throw system_error(error, "libgs::basic_ini::sync");
	}
	else
	{
		auto cflag = std::make_shared<bool>(false);
		m_impl->m_cancel_vector.emplace_back(cflag);

		auto slot = asio::get_associated_cancellation_slot(token);
		cancelled = [cflag = std::move(cflag), state = asio::cancellation_state(slot)]{
			return *cflag or state.cancelled() != asio::cancellation_type::none;
		};
		return async_work<error_code>::handle(get_executor(),
		[impl = m_impl, cancelled = std::move(cancelled)](auto handle, auto exec) mutable
		{
			using handle_t = std::remove_cvref_t<decltype(handle)>;
			detail::ini_commit_io_work([
				impl, data = impl->data(), cancelled = std::move(cancelled),
				handle = std::make_shared<handle_t>(std::move(handle)), exec
			]() mutable
			{
				error_code error;
				impl->sync(std::move(data), error, cancelled);
				dispatch(exec, [handle = std::move(handle), error]() mutable {
					std::move(*handle)(error);
				});
			});
		},
		std::forward<Token>(token));
	}
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
template <typename Rep, typename Period>
void basic_ini<CharT,Exec,Map,MapArgs...>::set_sync_period(const duration<Rep,Period> &period)
{
	m_impl->set_sync_period(period);
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::set_sync_on_delete(bool enable) noexcept
{
	m_impl->m_sync_on_delete = enable;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
milliseconds basic_ini<CharT,Exec,Map,MapArgs...>::sync_period() const noexcept
{
	return m_impl->m_sync_period;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
bool basic_ini<CharT,Exec,Map,MapArgs...>::sync_on_delete() const noexcept
{
	return m_impl->m_sync_on_delete;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::cancel()
{
	auto flags = std::move(m_impl->m_cancel_vector);
	for(auto flag : flags)
		*flag = true;
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::iterator
basic_ini<CharT,Exec,Map,MapArgs...>::find(const concepts::text_p<char_t> auto &group) noexcept
{
	return m_impl->m_groups.find(impl::replace(group));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::const_iterator
basic_ini<CharT,Exec,Map,MapArgs...>::find(const concepts::text_p<char_t> auto &group) const noexcept
{
	return m_impl->m_groups.find(impl::replace(group));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::clear() noexcept
{
	m_impl->m_groups.clear();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
size_t basic_ini<CharT,Exec,Map,MapArgs...>::size() const noexcept
{
	return m_impl->m_groups.size();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
void basic_ini<CharT,Exec,Map,MapArgs...>::set_data(data_t data)
{
	m_impl->set_data(std::move(data));
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::data_t basic_ini<CharT,Exec,Map,MapArgs...>::data() const
{
	return m_impl->data();
}

template <concepts::character CharT, concepts::exec Exec,
		  template<typename,typename,typename...> class Map, typename...MapArgs>
typename basic_ini<CharT,Exec,Map,MapArgs...>::executor_t
basic_ini<CharT,Exec,Map,MapArgs...>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs


#endif //LIBGS_CORE_DETAIL_INI_H
