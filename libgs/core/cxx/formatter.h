
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
#include <filesystem>
#include <optional>
#include <thread>
#include <atomic>
#include <memory>

namespace libgs
{

template <concepts::char_type CharT>
struct LIBGS_CORE_TAPI no_parse_formatter
{
	constexpr auto parse(std::basic_format_parse_context<CharT> &context) noexcept
	{
		if constexpr( std::is_same_v<CharT, char> )
			return std::ranges::find(context, '}');
		else
			return std::ranges::find(context, L'}');
	}
};

namespace detail
{

inline uint64_t thread_id_helper(void *id) {
	return reinterpret_cast<uint64_t>(id);
}
inline uint64_t thread_id_helper(uint64_t id) {
	return id;
}

}} //namespace libgs::detail

namespace std
{

template <typename T, libgs::concepts::char_type CharT> requires is_enum_v<T>
struct LIBGS_CORE_TAPI formatter<T,CharT>
{
	auto format(T e, auto &context) const {
		return m_formatter.format(static_cast<uint32_t>(e), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<uint32_t, CharT> m_formatter;
};

#if !defined(_MSC_VER) || !_HAS_CXX23

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::thread::id, CharT>
{
	auto format(const std::thread::id &tid, auto &context) const
	{
		auto handle = *reinterpret_cast<const std::thread::native_handle_type*>(&tid);
		return m_formatter.format(libgs::detail::thread_id_helper(handle), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<uint64_t, CharT> m_formatter;
};

#endif //_MSC_VER && _HAS_CXX23

template <typename T, libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::optional<T>, CharT>
{
	auto format(const std::optional<T> &ov, auto &context) const
	{
		if( ov )
			return m_formatter.format(*ov, context);

		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "optional(null)");
		else
			return format_to(context.out(), L"optional(null)");
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_formatter;
};

template <typename T, libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::atomic<T>, CharT>
{
	auto format(const std::atomic<T> &n, auto &context) const {
		return m_formatter.format(n.load(), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_formatter;
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<error_code, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const error_code &error, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{} ({})", error.message(), error.value());
		else
			return format_to(context.out(), L"{} ({})", error.message(), error.value());
	}
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<asio::ip::tcp::endpoint, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const asio::ip::tcp::endpoint &endpoint, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
		else
			return format_to(context.out(), L"{}:{}", endpoint.address().to_string(), endpoint.port());
	}
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<asio::ip::udp::endpoint, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const asio::ip::udp::endpoint &endpoint, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:{}", endpoint.address().to_string(), endpoint.port());
		else
			return format_to(context.out(), L"{}:{}", endpoint.address().to_string(), endpoint.port());
	}
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<asio::ip::address, CharT>
{
	auto format(const asio::ip::address &addr, auto &context) const {
		return m_formatter.format(addr.to_string(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::string, CharT> m_formatter;
};

template <typename Fir, typename Sec, libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::pair<Fir,Sec>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const std::pair<Fir,Sec> &pair, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "'{}'-'{}'", pair.first, pair.second);
		else
			return format_to(context.out(), L"'{}'-'{}'", pair.first, pair.second);
	}
};

template <typename T, libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::shared_ptr<T>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const std::shared_ptr<T> &ptr, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return format_to(context.out(), "{}:({})", libgs::type_name<T>(), reinterpret_cast<void*>(ptr.get()));
		else
			return format_to(context.out(), L"{}:({})", libgs::type_name<T>(), reinterpret_cast<void*>(ptr.get()));
	}
};

template <libgs::concepts::char_type CharT>
struct LIBGS_CORE_TAPI formatter<std::filesystem::path, CharT>
{
	auto format(const std::filesystem::path &path, auto &context) const
	{
		if constexpr( std::is_same_v<CharT, char> )
			return m_formatter.format(path.string(), context);
		else
			return m_formatter.format(path.wstring(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<std::basic_string<CharT>, CharT> m_formatter;
};

} //namespace std


#endif //LIBGS_CORE_CXX_FORMATTER_H
