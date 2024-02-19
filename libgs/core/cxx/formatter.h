
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS3                                                       *
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

#ifndef LIBGS_CORE_CXX_FORMATTER_H
#define LIBGS_CORE_CXX_FORMATTER_H

#include <libgs/core/cxx/utilities.h>
#include <rttr/type>
#include <asio.hpp>

#include <thread>
#include <atomic>
#include <memory>

namespace libgs
{

template <concept_char_type CharT>
class no_parse_formatter
{
public:
	constexpr auto parse(std::basic_format_parse_context<CharT> &context) noexcept 
	{
		if constexpr( std::is_same_v<CharT, char> )
			return std::ranges::find(context, '}');
		else
			return std::ranges::find(context, L'}');
	}
};

} //namespace libgs

namespace std
{

template <typename T, libgs::concept_char_type CharT> requires is_enum_v<T>
class formatter<T,CharT> 
{
public:
	auto format(T e, auto &context) const {
		return m_formatter.format(static_cast<uint32_t>(e), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<uint32_t, CharT> m_formatter;
};

template <libgs::concept_char_type CharT>
class formatter<std::thread::id, CharT>
{
public:
	auto format(const std::thread::id &tid, auto &context) const 
	{
		auto handle = *reinterpret_cast<const std::thread::native_handle_type *>(&tid);
		return m_formatter.format(reinterpret_cast<uint64_t>(handle), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<uint64_t, CharT> m_formatter;
};

template <typename T, libgs::concept_char_type CharT>
class formatter<std::atomic<T>, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const std::atomic<T> &n, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<CharT>, n.load());
	}
};

template <libgs::concept_char_type CharT>
class formatter<std::error_code, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const std::error_code &error, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{} ({})", error.message(), error.value());
		else
			return format_to(context.out(), L"{} ({})", error.message(), error.value());
	}
};

template <libgs::concept_char_type CharT>
class formatter<asio::ip::tcp::endpoint, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const asio::ip::tcp::endpoint &endpoint, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
		else
			return format_to(context.out(), L"{}:{}", endpoint.address().to_string(), endpoint.port());
	}
};

template <libgs::concept_char_type CharT>
class formatter<asio::ip::udp::endpoint, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const asio::ip::udp::endpoint &endpoint, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
		else
			return format_to(context.out(), L"{}:{}", endpoint.address().to_string(), endpoint.port());
	}
};

template <libgs::concept_char_type CharT>
class formatter<asio::ip::address, CharT> 
{
public:
	auto format(const asio::ip::address &addr, auto &context) const {
		return m_formatter.format(addr.to_string(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::string, CharT> m_formatter;
};

template <typename Fir, typename Sec, libgs::concept_char_type CharT>
class formatter<std::pair<Fir,Sec>, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const std::pair<Fir,Sec> &pair, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "'{}'-'{}'", pair.first, pair.second);
		else
			return format_to(context.out(), L"'{}'-'{}'", pair.first, pair.second);
	}
};

template <typename T, libgs::concept_char_type CharT>
class formatter<std::shared_ptr<T>, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const std::shared_ptr<T> &ptr, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:({})", libgs::type_name<T>(), reinterpret_cast<void*>(ptr.get()));
		else
			return format_to(context.out(), L"{}:({})", libgs::type_name<T>(), reinterpret_cast<void*>(ptr.get()));
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::basic_string_view<CharT>, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::basic_string_view<CharT> &str, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<CharT>, std::basic_string<CharT>(str));
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::type, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::type &type, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<CharT>, type.get_name().data());
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::method, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::method &method, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<CharT>, method.get_name().data());
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::property, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::property &property, auto &context) const {
		return format_to(context.out(), libgs::default_format_v<CharT>, property.get_name().data());
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::variant, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::variant &var, auto &context)const
	{
		if constexpr( std::is_same_v<CharT, char> )
		{
			std::string str;
			if( var.get_type() == libgs::type_id<bool>() )
				str = std::format("{}", var.get_value<bool>());

			else if( var.get_type() == libgs::type_id<signed char>() )
				str = std::format("{}", var.get_value<signed char>());
			else if( var.get_type() == libgs::type_id<unsigned char>() )
				str = std::format("{}", var.get_value<unsigned char>());

			else if( var.get_type() == libgs::type_id<short>() )
				str = std::format("{}", var.get_value<short>());
			else if( var.get_type() == libgs::type_id<unsigned short>() )
				str = std::format("{}", var.get_value<unsigned short>());

			else if( var.get_type() == libgs::type_id<int>() or var.get_type().is_enumeration() )
				str = std::format("{}", var.get_value<int>());
			else if( var.get_type() == libgs::type_id<unsigned int>() )
				str = std::format("{}", var.get_value<unsigned int>());

			else if( var.get_type() == libgs::type_id<long>() )
				str = std::format("{}", var.get_value<long>());
			else if( var.get_type() == libgs::type_id<unsigned long>() )
				str = std::format("{}", var.get_value<unsigned long>());

			else if( var.get_type() == libgs::type_id<long long>() )
				str = std::format("{}", var.get_value<long long>());
			else if( var.get_type() == libgs::type_id<unsigned long long>() )
				str = std::format("{}", var.get_value<unsigned long long>());

			else if( var.get_type() == libgs::type_id<float>() )
				str = std::format("{}", var.get_value<float>());
			else if( var.get_type() == libgs::type_id<double>() )
				str = std::format("{}", var.get_value<double>());
			else if( var.get_type() == libgs::type_id<long double>() )
				str = std::format("{}", var.get_value<long double>());

			else if( var.get_type() == libgs::type_id<char*>() )
				str = std::format("'{}'", var.get_value<char*>());
			else if( var.get_type() == libgs::type_id<std::string>() )
				str = std::format("'{}'", var.get_value<std::string>());
			else if( var.get_type() == libgs::type_id<std::string_view>() )
				str = std::format("'{}'", var.get_value<std::string_view>());
			else if( var.get_type() == libgs::type_id<rttr::string_view>() )
				str = std::format("'{}'", var.get_value<rttr::string_view>().to_string());

			return format_to(context.out(), "rttr::variant({}:{})", var.get_type().get_name().data(), str);
		}
		else
		{
			std::wstring str;
			if( var.get_type() == libgs::type_id<bool>() )
				str = std::format(L"{}", var.get_value<bool>());

			else if( var.get_type() == libgs::type_id<signed char>() )
				str = std::format(L"{}", var.get_value<signed char>());
			else if( var.get_type() == libgs::type_id<unsigned char>() )
				str = std::format(L"{}", var.get_value<unsigned char>());

			else if( var.get_type() == libgs::type_id<short>() )
				str = std::format(L"{}", var.get_value<short>());
			else if( var.get_type() == libgs::type_id<unsigned short>() )
				str = std::format(L"{}", var.get_value<unsigned short>());

			else if( var.get_type() == libgs::type_id<int>() or var.get_type().is_enumeration() )
				str = std::format(L"{}", var.get_value<int>());
			else if( var.get_type() == libgs::type_id<unsigned int>() )
				str = std::format(L"{}", var.get_value<unsigned int>());

			else if( var.get_type() == libgs::type_id<long>() )
				str = std::format(L"{}", var.get_value<long>());
			else if( var.get_type() == libgs::type_id<unsigned long>() )
				str = std::format(L"{}", var.get_value<unsigned long>());

			else if( var.get_type() == libgs::type_id<long long>() )
				str = std::format(L"{}", var.get_value<long long>());
			else if( var.get_type() == libgs::type_id<unsigned long long>() )
				str = std::format(L"{}", var.get_value<unsigned long long>());

			else if( var.get_type() == libgs::type_id<float>() )
				str = std::format(L"{}", var.get_value<float>());
			else if( var.get_type() == libgs::type_id<double>() )
				str = std::format(L"{}", var.get_value<double>());
			else if( var.get_type() == libgs::type_id<long double>() )
				str = std::format(L"{}", var.get_value<long double>());

			else if( var.get_type() == libgs::type_id<wchar_t*>() )
				str = std::format(L"'{}'", var.get_value<wchar_t*>());
			else if( var.get_type() == libgs::type_id<std::wstring>() )
				str = std::format(L"'{}'", var.get_value<std::wstring>());
			else if( var.get_type() == libgs::type_id<std::wstring_view>() )
				str = std::format(L"'{}'", var.get_value<std::wstring_view>());
			else if( var.get_type() == libgs::type_id<rttr::basic_string_view<CharT>>() )
				str = std::format(L"'{}'", var.get_value<rttr::basic_string_view<CharT>>().to_string());

			return format_to(context.out(), L"rttr::variant({}:{})", var.get_type().get_name().data(), str);
		}
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::argument, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::argument &arg, auto &context) const {
		return formatter<rttr::variant, CharT>().format(arg.get_value<rttr::variant>(), context);
	}
};

template <libgs::concept_char_type CharT>
class formatter<rttr::instance, CharT> : public libgs::no_parse_formatter<CharT>
{
public:
	auto format(const rttr::argument &arg, auto &context) const {
		return formatter<rttr::type, CharT>().format(arg.get_type(), context);
	}
};

} //namespace std


#endif //LIBGS_CORE_CXX_FORMATTER_H
