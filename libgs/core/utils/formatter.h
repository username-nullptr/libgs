
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_UTILS_FORMATTER_H
#define LIBGS_CORE_UTILS_FORMATTER_H

#include <libgs/core/utils/string_tools.h>
#include <libgs/core/cxx/formatter.h>
#include <libgs/core/cxx/expected.h>
#include <libgs/core/cxx/tools.h>
#include <asio.hpp>

#include <filesystem>
#include <optional>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>

namespace libgs { namespace detail
{

inline uint64_t thread_id_helper(void *id) {
	return reinterpret_cast<uint64_t>(id);
}
inline uint64_t thread_id_helper(uint64_t id) {
	return id;
}

} //namespace detail

namespace concepts
{

template <typename CharT, typename...Args>
concept formatter = sizeof...(Args) > 0 and requires(Args&&...args) {
	(std::format(l_str(CharT,"{}"), std::forward<Args>(args)), ...);
};

template <typename...Args>
concept any_formatter =
	formatter<char,Args...> or formatter<wchar_t,Args...> or
	formatter<char8_t,Args...> or formatter<char16_t,Args...> or formatter<char32_t,Args...>;

}} //namespace libgs::concepts

namespace std
{

template <libgs::concepts::enumerate T, libgs::concepts::character CharT>
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

template <libgs::concepts::pointer T, libgs::concepts::character CharT>
requires (not is_same_v<std::remove_const_t<T>,void*>)
struct LIBGS_CORE_TAPI formatter<T,CharT>
{
	auto format(T p, auto &context) const {
		return m_formatter.format(static_cast<void*>(p), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<void*, CharT> m_formatter;
};

#if !defined(_MSC_VER) || !_HAS_CXX23

template <libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<thread::id, CharT>
{
	auto format(const thread::id &tid, auto &context) const
	{
		auto handle = *reinterpret_cast<const thread::native_handle_type*>(&tid);
		return m_formatter.format(libgs::detail::thread_id_helper(handle), context);
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<uint64_t, CharT> m_formatter;
};

#endif //_MSC_VER && _HAS_CXX23

template <typename T, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<optional<T>, CharT>
{
	auto format(const optional<T> &ov, auto &context) const
	{
		if( ov )
			return m_formatter.format(*ov, context);
		return format_to(context.out(), l_str(CharT,"optional(null)"));
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_formatter;
};

template <typename T, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<libgs::optional<T>, CharT>
{
	auto format(const libgs::optional<T> &ov, auto &context) const
	{
		if( ov )
			return m_formatter.format(*ov, context);
		return format_to(context.out(), l_str(CharT,"optional(null)"));
	}

	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_formatter;
};

template <typename E, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<libgs::unexpected<E>, CharT>
{
	auto format(const libgs::unexpected<E> &ov, auto &context) {
		return m_formatter.format(ov.error(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<E, CharT> m_formatter;
};

template <typename T, typename E, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<libgs::expected<T,E>, CharT>
{
	auto format(const libgs::expected<T,E> &ov, auto &context) const
	{
		m_has_formatter = ov.has_value();
		return m_has_formatter ?
			m_value_formatter.format(*ov, context) :
			m_error_formatter.format(ov.error(), context);
	}

	constexpr auto parse(auto &context) noexcept
	{
		return m_has_formatter ?
			m_value_formatter.parse(context) :
			m_error_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_value_formatter;
	formatter<E, CharT> m_error_formatter;
	mutable bool m_has_formatter = false;
};

template <typename E, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<libgs::expected<void,E>, CharT>
{
	auto format(const libgs::expected<void,E> &ov, auto &context) const
	{
		m_has_formatter = ov.has_value();
		return m_has_formatter ?
			std::basic_string<CharT>(l_str(CharT,"OK")) :
			m_formatter.format(ov.error(), context);
	}

	constexpr auto parse(auto &context) noexcept
	{
		return m_has_formatter ?
			context.begin() : m_formatter.parse(context);
	}

private:
	formatter<E, CharT> m_formatter;
	mutable bool m_has_formatter = false;
};

#if LIBGS_STD_CXX >= 23
template <typename T, typename E, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<expected<T,E>, CharT>
{
	auto format(const expected<T,E> &ov, auto &context)
	{
		m_has_formatter = ov.has_value();
		return m_has_formatter ?
			m_value_formatter.format(*ov, context) :
			m_error_formatter.format(ov.error(), context);
	}

	constexpr auto parse(auto &context) noexcept
	{
		return m_has_formatter ?
			m_value_formatter.parse(context) :
			m_error_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_value_formatter;
	formatter<E, CharT> m_error_formatter;
	bool m_has_formatter = false;
};
#endif //LIBGS_STD_CXX >= 23

template <typename T, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<atomic<T>, CharT>
{
	auto format(const atomic<T> &n, auto &context) const {
		return m_formatter.format(n.load(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<T, CharT> m_formatter;
};

template <libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<error_code, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const error_code &error, auto &context) const {
		return format_to(context.out(), l_str(CharT,"{} ({})"), error.message(), error.value());
	}
};

template <typename Protocol, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<asio::ip::basic_endpoint<Protocol>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const asio::ip::basic_endpoint<Protocol> &endpoint, auto &context) const
	{
		return format_to(context.out(), l_str(CharT,"{}:{}"),
			libgs::strtls::detail::ascii_transition<CharT>(endpoint.address().to_string()), endpoint.port()
		);
	}
};

template <libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<asio::ip::address, CharT>
{
	auto format(const asio::ip::address &addr, auto &context) const {
		return m_formatter.format(libgs::strtls::detail::ascii_transition<CharT>(addr.to_string()), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<basic_string<CharT>, CharT> m_formatter;
};

template <typename Fir, typename Sec, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<pair<Fir,Sec>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const pair<Fir,Sec> &pair, auto &context) const {
		return format_to(context.out(), l_str(CharT,"'{}'-'{}'"), pair.first, pair.second);
	}
};

template <typename T, libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<shared_ptr<T>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const shared_ptr<T> &ptr, auto &context) const
	{
		return format_to(context.out(), l_str(CharT,"{}:({})"),
			libgs::type_name<T>(), reinterpret_cast<void*>(ptr.get())
		);
	}
};

template <libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<filesystem::path, CharT>
{
	auto format(const filesystem::path &path, auto &context) const {
		return m_formatter.format(path.string<CharT>(), context);
	}
	constexpr auto parse(auto &context) noexcept {
		return m_formatter.parse(context);
	}

private:
	formatter<basic_string<CharT>, CharT> m_formatter;
};

template <libgs::concepts::character CharT>
struct LIBGS_CORE_TAPI formatter<std::vector<std::byte>, CharT> : libgs::no_parse_formatter<CharT>
{
	auto format(const std::vector<std::byte> &buffer, auto &context) const
	{
		std::basic_string<CharT> text {};
		if( buffer.empty() )
		{
			return format_to(context.out(),
				l_str(CharT,"empty[std::vector<std::byte>]")
			);
		}
		for(auto &byte : buffer)
			text += std::format(l_str(CharT,"0x{:X}, "), byte);
		text.pop_back();
		text.pop_back();

		return format_to(context.out(),
			l_str(CharT,"{}"), text
		);
	}
};

} //namespace std


#endif //LIBGS_CORE_UTILS_FORMATTER_H
