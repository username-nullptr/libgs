
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

#ifndef LIBGS_CORE_ALGORITHM_DETAIL_BASE_H
#define LIBGS_CORE_ALGORITHM_DETAIL_BASE_H

namespace libgs { namespace detail
{

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI auto _sto_float(auto &&func, std::basic_string_view<CharT> str)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index);
	if( index < str.size() )
		throw runtime_error("Cannot convert string to arithmetic.");
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI auto _sto_int(auto &&func, std::basic_string_view<CharT> str, size_t base)
{
	size_t index = 0;
	auto res = func({str.data(), str.size()}, &index, static_cast<int>(base));
	if( index < str.size() )
	{
		res = static_cast<decltype(res)>(
			_sto_float(static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str)
		);
	}
	return res;
}

template <concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI int _stob(std::basic_string_view<CharT> str)
{
	if constexpr( is_char_v<CharT> )
	{
#ifdef WIN32
		if( _stricmp(str.data(), "true") == 0 )
			return 1;
		else if( _stricmp(str.data(), "false") == 0 )
			return 0;
#else
		if( str.size() == 4 and strncasecmp(str.data(), "true", 4) == 0 )
			return 1;
		else if( str.size() == 5 and strncasecmp(str.data(), "false", 5) == 0 )
			return 0;
#endif
		return -1;
	}
	else
		return _stob<char>(wcstombs(str));
}

template <concepts::char_type CharT, typename T>
[[nodiscard]] LIBGS_CORE_TAPI T try_stobtot(std::basic_string_view<CharT> str, const std::optional<T> &odv)
{
	int res = _stob<CharT>(str);
	if( res < 0 )
	{
		if( odv )
			return *odv;
		throw runtime_error("Cannot convert string to arithmetic.");
	}
	return static_cast<T>(!!res);
}

template <concepts::integral_type T, concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::basic_string_view<CharT> str, size_t base, std::optional<T> odv = {})
{
	if constexpr( std::is_same_v<T, bool> )
		return stob(str, base);
	else
	{
		try {
			if constexpr( std::is_same_v<T, char> )
			{
				return static_cast<char>(_sto_int(
					static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned char> )
			{
				return static_cast<unsigned char>(_sto_int(
					static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, short> )
			{
				return static_cast<short>(_sto_int(
					static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned short> )
			{
				return static_cast<unsigned short>(_sto_int(
					static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, int> )
			{
				return static_cast<int>(_sto_int(
					static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned int> )
			{
				return static_cast<unsigned int>(_sto_int(
					static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, long> )
			{
				return static_cast<long>(_sto_int(
					static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long> )
			{
				return static_cast<unsigned long>(_sto_int(
					static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base
				));
			}
			else if constexpr( std::is_same_v<T, long long> )
			{
				return static_cast<long long>(_sto_int(
					static_cast<long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoll), str, base
				));
			}
			else if constexpr( std::is_same_v<T, unsigned long long> )
			{
				return static_cast<unsigned long long>(_sto_int(
					static_cast<unsigned long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoull), str, base
				));
			}
		}
		catch(std::exception&) {}
		return try_stobtot(str, odv);
	}
}

template <concepts::float_type T, concepts::char_type CharT>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::basic_string_view<CharT> str, std::optional<T> odv = {})
{
	try {
		if constexpr( std::is_same_v<T, float> )
		{
			return _sto_float(
				static_cast<float(*)(const std::basic_string<CharT>&,size_t*)>(std::stof), str
			);
		}
		else if constexpr( std::is_same_v<T, double> )
		{
			return _sto_float(
				static_cast<double(*)(const std::basic_string<CharT>&,size_t*)>(std::stod), str
			);
		}
		else if constexpr( std::is_same_v<T, long double> )
		{
			return _sto_float(
				static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str
			);
		}
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

} //namespace detail

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::string_view str, size_t base)
{
	return detail::ston<T,char>(str, base);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::wstring_view str, size_t base)
{
	return detail::ston<T,wchar_t>(str, base);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::string_view str)
{
	return detail::ston<T,char>(str);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston(std::wstring_view str)
{
	return detail::ston<T,wchar_t>(str);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::string_view str, size_t base, T default_value) noexcept
{
	return detail::ston<T,char>(str, base, default_value);
}

template <concepts::integral_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::wstring_view str, size_t base, T default_value) noexcept
{
	return detail::ston<T,wchar_t>(str, base, default_value);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::string_view str, T default_value)
{
	return detail::ston<T,char>(str, default_value);
}

template <concepts::float_type T>
[[nodiscard]] LIBGS_CORE_TAPI T ston_or(std::wstring_view str, T default_value)
{
	return detail::ston<T,wchar_t>(str, default_value);
}

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_DETAIL_BASE_H