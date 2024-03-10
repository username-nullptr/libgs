
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

#include <libgs/core/global.h>
#include <libgs/core/cxx/exception.h>

namespace libgs::algorithm_base
{

template <concept_char_type CharT>
[[nodiscard]] int _stob(const std::basic_string<CharT> &str)
{
	if constexpr( is_char_v<CharT> )
	{
#ifdef WIN32
		if( _stricmp(str.c_str(), "true") == 0 )
			return 1;
		else if( _stricmp(str.c_str(), "false") == 0 )
			return 0;
#else
		if( str.size() == 4 and strncasecmp(str.c_str(), "true", 4) == 0 )
			return 1;
		else if( str.size() == 5 and strncasecmp(str.c_str(), "false", 5) == 0 )
			return 0;
#endif
	}
	else
		return _stob<char>(wcstombs(str).c_str());
	return -1;
}

template <concept_char_type CharT, typename T>
static T try_stobtot(const std::basic_string<CharT> &str, const std::optional<T> &odv)
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

template <concept_char_type CharT>
inline auto _sto_float(auto &&func, const std::basic_string<CharT> &str)
{  
	size_t index = 0; 
	auto res = func(str, &index); 
	if( index < str.size() ) 
		throw runtime_error("Cannot convert string to arithmetic."); 
	return res; 
}

template <concept_char_type CharT>
inline auto _sto_int(auto &&func, const std::basic_string<CharT> &str, size_t base)
{
	size_t index = 0; 
	auto res = func(str, &index, static_cast<int>(base)); 
	if( index < str.size() ) 
		res = static_cast<decltype(res)>(_sto_float(static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str)); 
	return res;
}

template <concept_char_type CharT>
[[nodiscard]] int8_t stoi8(const std::basic_string<CharT> &str, size_t base, std::optional<int8_t> odv = {})
{
	try {
		return static_cast<int8_t>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] uint8_t stoui8(const std::basic_string<CharT> &str, size_t base, std::optional<uint8_t> odv = {})
{
	try {
		return static_cast<uint8_t>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] int16_t stoi16(const std::basic_string<CharT> &str, size_t base, std::optional<int16_t> odv = {})
{
	try {
		return static_cast<int16_t>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] uint16_t stoui16(const std::basic_string<CharT> &str, size_t base, std::optional<uint16_t> odv = {})
{
	try {
		return static_cast<uint16_t>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] int32_t stoi32(const std::basic_string<CharT> &str, size_t base, std::optional<int32_t> odv = {})
{
	try {
		return static_cast<int32_t>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] uint32_t stoui32(const std::basic_string<CharT> &str, size_t base, std::optional<uint32_t> odv = {})
{
	try {
		return static_cast<uint32_t>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] int64_t stoi64(const std::basic_string<CharT> &str, size_t base, std::optional<int64_t> odv = {})
{
	try {
		return static_cast<int64_t>(_sto_int(static_cast<long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoll), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] uint64_t stoui64(const std::basic_string<CharT> &str, size_t base, std::optional<uint64_t> odv = {})
{
	try {
		return static_cast<uint64_t>(_sto_int(static_cast<unsigned long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoull), str, base));
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] float stof(const std::basic_string<CharT> &str, std::optional<float> odv = {})
{
	try {
		return _sto_float(static_cast<float(*)(const std::basic_string<CharT>&,size_t*)>(std::stof), str);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] double stod(const std::basic_string<CharT> &str, std::optional<double> odv = {})
{
	try {
		return _sto_float(static_cast<double(*)(const std::basic_string<CharT>&,size_t*)>(std::stod), str);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] double stold(const std::basic_string<CharT> &str, std::optional<double> odv = {})
{
	try {
		return _sto_float(static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] bool stob(const std::basic_string<CharT> &str, size_t base, std::optional<bool> odv = {})
{
	int res = _stob<CharT>(str);
	if( res < 0 )
	{
		try {
			return !!_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base);
		}
		catch(...)
		{
			if( odv.has_value() )
				return *odv;
			throw runtime_error("Cannot convert string to arithmetic.");
		}
	}
	return res > 0;
}

template <concept_integral_type T, concept_char_type CharT>
[[nodiscard]] T ston(const std::basic_string<CharT> &str, size_t base, std::optional<T> odv = {})
{
	if constexpr( std::is_same_v<T, bool> )
		return stob(str, base);
	else
	{
		try {
			if constexpr( std::is_same_v<T, char> )
				return static_cast<char>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
			else if constexpr( std::is_same_v<T, unsigned char> )
				return static_cast<unsigned char>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
			else if constexpr( std::is_same_v<T, short> )
				return static_cast<short>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
			else if constexpr( std::is_same_v<T, unsigned short> )
				return static_cast<unsigned short>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
			else if constexpr( std::is_same_v<T, int> )
				return static_cast<int>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
			else if constexpr( std::is_same_v<T, unsigned int> )
				return static_cast<unsigned int>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
			else if constexpr( std::is_same_v<T, long> )
				return static_cast<long>(_sto_int(static_cast<long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stol), str, base));
			else if constexpr( std::is_same_v<T, unsigned long> )
				return static_cast<unsigned long>(_sto_int(static_cast<unsigned long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoul), str, base));
			else if constexpr( std::is_same_v<T, long long> )
				return static_cast<long long>(_sto_int(static_cast<long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoll), str, base));
			else if constexpr( std::is_same_v<T, unsigned long long> )
				return static_cast<unsigned long long>(_sto_int(static_cast<unsigned long long(*)(const std::basic_string<CharT>&,size_t*,int)>(std::stoull), str, base));
		}
		catch(std::exception&) {}
		return try_stobtot(str, odv);
	}
}

template <concept_float_type T, concept_char_type CharT>
[[nodiscard]] T ston(const std::basic_string<CharT> &str, std::optional<T> odv = {})
{
	try {
		if constexpr( std::is_same_v<T, float> )
			return _sto_float(static_cast<float(*)(const std::basic_string<CharT>&,size_t*)>(std::stof), str);
		else if constexpr( std::is_same_v<T, double> )
			return _sto_float(static_cast<double(*)(const std::basic_string<CharT>&,size_t*)>(std::stod), str);
		else if constexpr( std::is_same_v<T, long double> )
			return _sto_float(static_cast<long double(*)(const std::basic_string<CharT>&,size_t*)>(std::stold), str);
	}
	catch(std::exception&) {}
	return try_stobtot(str, odv);
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> str_to_lower(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result(str.size(), '\0');
	for(size_t i=0; i<str.size(); i++)
		result[i] = ::tolower(str[i]);
	return result;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> str_to_upper(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result(str.size(), '\0');
	for(size_t i=0; i<str.size(); i++)
		result[i] = ::toupper(str[i]);
	return result;
}

template <concept_char_type CharT> /*[[nodiscard]]*/ 
size_t str_replace(std::basic_string<CharT> &str, std::basic_string_view<CharT> _old, std::basic_string_view<CharT> _new, bool step)
{
	if( _old == _new )
		return 0;

	size_t sum = 0;
	size_t old_pos = 0;

	for(;;)
	{
		auto start = str.find(_old, old_pos);
		if( start == std::basic_string<CharT>::npos )
			break;

		str.replace(start, _old.size(), _new);
		old_pos = start;

		if( step )
			old_pos += _new.size();
		sum++;
	}
	return sum;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> str_trimmed(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result;

	size_t left = 0;
	while( left < str.size() )
	{
		if( str[left] >= 1 and str[left] <= 32 )
			left++;
		else
			break;
	}
	if( left >= str.size() )
		return result;

	int right = static_cast<int>(str.size() - 1);
	while( right >= 0 )
	{
		if( str[right] >= 1 and str[right] <= 32 )
			right--;
		else
			break;
	}
	if( right < 0 )
		return result;

	result = str.substr(0, right + 1);
	result = result.substr(left);
	return result;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> 
str_remove(std::basic_string_view<CharT> str, std::basic_string_view<CharT> find, bool step)
{
	std::basic_string<CharT> res(str.data(), str.size());
	str_replace<CharT>(res, find, std::basic_string<CharT>(), step);
	return res;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> str_remove(std::basic_string_view<CharT> str, CharT find, bool)
{
	std::basic_string<CharT> res(str.data(), str.size());
	auto it = std::remove(res.begin(), res.end(), find);
	if( it != res.end() )
		res.erase(it, res.end());
	return res;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> from_percent_encoding(std::basic_string_view<CharT> str)
{
	std::basic_string<CharT> result;
	if( str.empty() )
		return result;

	result = str;
	auto *data = const_cast<CharT*>(result.c_str());
	const CharT *inputPtr = result.c_str();

	size_t i = 0;
	size_t len = str.size();
	size_t outlen = 0;
	int a, b;
	CharT c;

	while( i < len )
	{
		c = inputPtr[i];
		if constexpr( is_char_v<CharT> )
		{
			if( c == '%' and i + 2 < len )
			{
				a = inputPtr[++i];
				b = inputPtr[++i];

				if( a >= '0' and a <= '9' )
					a -= '0';

				else if( a >= 'a' and a <= 'f' )
					a = a - 'a' + 10;

				else if( a >= 'A' && a <= 'F' )
					a = a - 'A' + 10;

				if( b >= '0' and b <= '9' )
					b -= '0';

				else if( b >= 'a' and b <= 'f' )
					b = b - 'a' + 10;

				else if( b >= 'A' and b <= 'F' )
					b = b - 'A' + 10;

				*data++ = static_cast<CharT>((a << 4) | b);
			}
			else
				*data++ = c;
		}
		else
		{
			if( c == L'%' and i + 2 < len )
			{
				a = inputPtr[++i];
				b = inputPtr[++i];

				if( a >= L'0' and a <= L'9' )
					a -= L'0';

				else if( a >= L'a' and a <= L'f' )
					a = a - L'a' + 10;

				else if( a >= L'A' && a <= L'F' )
					a = a - L'A' + 10;

				if( b >= L'0' and b <= L'9' )
					b -= L'0';

				else if( b >= L'a' and b <= L'f' )
					b = b - L'a' + 10;

				else if( b >= L'A' and b <= L'F' )
					b = b - L'A' + 10;

				*data++ = static_cast<CharT>((a << 4) | b);
			}
			else
				*data++ = c;
		}
		++i;
		++outlen;
	}
	if( outlen != len )
		result = result.substr(0, outlen);
	return result;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> file_name(std::basic_string_view<CharT> file_name)
{
	size_t pos = 0;
	if constexpr( is_char_v<CharT> )
		pos = file_name.rfind("/");
	else
		pos = file_name.rfind(L"/");

	std::basic_string<CharT> result;
	if( pos == std::basic_string<CharT>::npos )
		result = std::basic_string<CharT>(file_name.data(), file_name.size());
	else
	{
		auto tmp = file_name.substr(pos + 1);
		result = std::basic_string<CharT>(tmp.data(), tmp.size());
	}
	return result;
}

template <concept_char_type CharT>
[[nodiscard]] std::basic_string<CharT> file_path(std::basic_string_view<CharT> file_name)
{
	size_t pos = 0;
	if constexpr( is_char_v<CharT> )
	{
		pos = file_name.rfind("/");
		if( pos == std::basic_string<CharT>::npos )
			return ".";
	}
	else
	{
		pos = file_name.rfind(L"/");
		if( pos == std::basic_string<CharT>::npos )
			return L".";
	}
	auto tmp = file_name.substr(0, pos + 1);
	return std::basic_string<CharT>(tmp.data(), tmp.size());
}

} //namespace libgs::algorithm_base


#endif //LIBGS_CORE_ALGORITHM_DETAIL_BASE_H
