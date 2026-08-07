
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_CXX_DETAIL_STREAMER_H
#define LIBGS_CORE_CXX_DETAIL_STREAMER_H

#include <libgs/core/utils/flags.h>
#include <libgs/core/utils/string_tools.h>
#include <libgs/core/cxx/cplusplus.h>

namespace libgs
{

template <>
struct streamer<bool>
{
	[[nodiscard]] static auto encode(bool v) noexcept
	{
		std::vector<std::byte> buf;
		buf.emplace_back(static_cast<std::byte>(v));
		return buf;
	}

	[[nodiscard]] static decoder_data<bool>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() <= offset )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset
			));
		}
		return { std::to_integer<bool>(buf[offset]), sizeof(bool) };
	}
};

template <std::integral T> requires (not std::same_as<T, bool>)
struct streamer<T>
{
	[[nodiscard]] static auto encode(T v) noexcept
	{
		std::vector<std::byte> buf;
		buf.resize(sizeof(T));
		std::memcpy(buf.data(), &v, buf.size());
		return buf;
	}

	[[nodiscard]] static decoder_data<T>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() <= offset )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset
			));
		}
		else if( buf.size() - offset < sizeof(T) )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size() - offset, sizeof(T)
			));
		}
		T value = 0;
		std::memcpy(&value, buf.data() + offset, sizeof(T));
		return { value, sizeof(T) };
	}
};

template <std::floating_point T>
struct streamer<T>
{
	[[nodiscard]] static auto encode(T v) noexcept
	{
		std::vector<std::byte> buf;
		buf.resize(sizeof(T));
		std::memcpy(buf.data(), &v, buf.size());
		return buf;
	}

	[[nodiscard]] static decoder_data<T>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() <= offset )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset
			));
		}
		else if( buf.size() - offset < sizeof(T) )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size() - offset, sizeof(T)
			));
		}
		T value = 0.0;
		std::memcpy(&value, buf.data() + offset, sizeof(T));
		return { value, sizeof(T) };
	};
};

template <concepts::enumerate T>
struct streamer<T>
{
	using num_t = std::underlying_type_t<T>;

	[[nodiscard]] static auto encode(T v) noexcept {
		return streamer<num_t>::encode(static_cast<num_t>(v));
	}
	[[nodiscard]] static decoder_data<T>
	decode(const std::vector<std::byte> &buf, size_t offset = 0) {
		auto data = streamer<num_t>::decode(buf, offset);
		return { static_cast<T>(*data), data.size };
	}
};

template <concepts::flag_template T>
struct streamer<flags<T>>
{
	using enum_t = T;
	using flags_t = flags<enum_t>;

	[[nodiscard]] static auto encode(flags_t v) noexcept {
		return streamer<enum_t>::encode(static_cast<enum_t>(v));
	}
	[[nodiscard]] static decoder_data<flags_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0) {
		auto data = streamer<enum_t>::decode(buf, offset);
		return { flags<T>(*data), data.size };
	}
};

template <typename T, size_t N>
struct streamer<T[N]>
{
	static auto encode(const T v[N])
	{
		std::vector<std::byte> buf;
		buf.resize(8);
		*reinterpret_cast<uint64_t*>(buf.data()) = N;
		for(size_t i=0; i<N; i++)
		{
			auto sub = streamer<T>::encode(v[i]);
			buf.insert(buf.end(),
				std::make_move_iterator(sub.begin()),
				std::make_move_iterator(sub.end())
			);
		}
		return buf;
	}

	[[nodiscard]] static auto decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() < offset + 8 )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset + 8
			));
		}
		auto size = *reinterpret_cast<const uint64_t*>(buf.data() + offset);
		if( size != N )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), N
			));
		}
		offset += 8;
		std::array<T,N> arr;
		size_t sum = 8;

		for(auto &n : arr)
		{
			auto data = streamer<T>::decode(buf, offset);
			n = std::move(*data);

			offset += data.size;
			sum += data.size;
		}
		return decoder_data<std::array<T,N>> { std::move(arr), sum };
	}
};

#define LIBGS_FIELD_MAP(m, ...) \
	LIBGS_CAT(LIBGS_FIELD_MAP_, LIBGS_ARG_COUNT(__VA_ARGS__))(m, __VA_ARGS__)

#define LIBGS_FIELD_MAP_1(m, x     )   m(x)
#define LIBGS_FIELD_MAP_2(m, x, ...)   m(x) LIBGS_FIELD_MAP_1(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_3(m, x, ...)   m(x) LIBGS_FIELD_MAP_2(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_4(m, x, ...)   m(x) LIBGS_FIELD_MAP_3(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_5(m, x, ...)   m(x) LIBGS_FIELD_MAP_4(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_6(m, x, ...)   m(x) LIBGS_FIELD_MAP_5(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_7(m, x, ...)   m(x) LIBGS_FIELD_MAP_6(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_8(m, x, ...)   m(x) LIBGS_FIELD_MAP_7(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_9(m, x, ...)   m(x) LIBGS_FIELD_MAP_8(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_10(m, x, ...)  m(x) LIBGS_FIELD_MAP_9(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_11(m, x, ...)  m(x) LIBGS_FIELD_MAP_10(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_12(m, x, ...)  m(x) LIBGS_FIELD_MAP_11(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_13(m, x, ...)  m(x) LIBGS_FIELD_MAP_12(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_14(m, x, ...)  m(x) LIBGS_FIELD_MAP_13(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_15(m, x, ...)  m(x) LIBGS_FIELD_MAP_14(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_16(m, x, ...)  m(x) LIBGS_FIELD_MAP_15(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_17(m, x, ...)  m(x) LIBGS_FIELD_MAP_16(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_18(m, x, ...)  m(x) LIBGS_FIELD_MAP_17(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_19(m, x, ...)  m(x) LIBGS_FIELD_MAP_18(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_20(m, x, ...)  m(x) LIBGS_FIELD_MAP_19(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_21(m, x, ...)  m(x) LIBGS_FIELD_MAP_20(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_22(m, x, ...)  m(x) LIBGS_FIELD_MAP_21(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_23(m, x, ...)  m(x) LIBGS_FIELD_MAP_22(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_24(m, x, ...)  m(x) LIBGS_FIELD_MAP_23(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_25(m, x, ...)  m(x) LIBGS_FIELD_MAP_24(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_26(m, x, ...)  m(x) LIBGS_FIELD_MAP_25(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_27(m, x, ...)  m(x) LIBGS_FIELD_MAP_26(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_28(m, x, ...)  m(x) LIBGS_FIELD_MAP_27(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_29(m, x, ...)  m(x) LIBGS_FIELD_MAP_28(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_30(m, x, ...)  m(x) LIBGS_FIELD_MAP_29(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_31(m, x, ...)  m(x) LIBGS_FIELD_MAP_30(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_32(m, x, ...)  m(x) LIBGS_FIELD_MAP_31(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_33(m, x, ...)  m(x) LIBGS_FIELD_MAP_32(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_34(m, x, ...)  m(x) LIBGS_FIELD_MAP_33(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_35(m, x, ...)  m(x) LIBGS_FIELD_MAP_34(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_36(m, x, ...)  m(x) LIBGS_FIELD_MAP_35(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_37(m, x, ...)  m(x) LIBGS_FIELD_MAP_36(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_38(m, x, ...)  m(x) LIBGS_FIELD_MAP_37(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_39(m, x, ...)  m(x) LIBGS_FIELD_MAP_38(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_40(m, x, ...)  m(x) LIBGS_FIELD_MAP_39(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_41(m, x, ...)  m(x) LIBGS_FIELD_MAP_40(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_42(m, x, ...)  m(x) LIBGS_FIELD_MAP_41(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_44(m, x, ...)  m(x) LIBGS_FIELD_MAP_42(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_45(m, x, ...)  m(x) LIBGS_FIELD_MAP_44(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_46(m, x, ...)  m(x) LIBGS_FIELD_MAP_45(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_47(m, x, ...)  m(x) LIBGS_FIELD_MAP_46(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_48(m, x, ...)  m(x) LIBGS_FIELD_MAP_47(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_49(m, x, ...)  m(x) LIBGS_FIELD_MAP_48(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_50(m, x, ...)  m(x) LIBGS_FIELD_MAP_49(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_51(m, x, ...)  m(x) LIBGS_FIELD_MAP_50(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_52(m, x, ...)  m(x) LIBGS_FIELD_MAP_51(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_53(m, x, ...)  m(x) LIBGS_FIELD_MAP_52(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_54(m, x, ...)  m(x) LIBGS_FIELD_MAP_53(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_55(m, x, ...)  m(x) LIBGS_FIELD_MAP_54(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_56(m, x, ...)  m(x) LIBGS_FIELD_MAP_55(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_57(m, x, ...)  m(x) LIBGS_FIELD_MAP_56(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_58(m, x, ...)  m(x) LIBGS_FIELD_MAP_57(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_59(m, x, ...)  m(x) LIBGS_FIELD_MAP_58(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_60(m, x, ...)  m(x) LIBGS_FIELD_MAP_59(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_61(m, x, ...)  m(x) LIBGS_FIELD_MAP_60(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_62(m, x, ...)  m(x) LIBGS_FIELD_MAP_61(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_63(m, x, ...)  m(x) LIBGS_FIELD_MAP_62(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_64(m, x, ...)  m(x) LIBGS_FIELD_MAP_63(m, __VA_ARGS__)

#define LIBGS_FIELD_MAP_COMMA(m, ...) \
	LIBGS_CAT(LIBGS_FIELD_MAP_COMMA_, LIBGS_ARG_COUNT(__VA_ARGS__))(m, __VA_ARGS__)

#define LIBGS_FIELD_MAP_COMMA_1(m, x     )   m(x)
#define LIBGS_FIELD_MAP_COMMA_2(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_1(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_3(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_2(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_4(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_3(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_5(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_4(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_6(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_5(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_7(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_6(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_8(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_7(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_9(m, x, ...)   m(x), LIBGS_FIELD_MAP_COMMA_8(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_10(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_9(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_11(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_10(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_12(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_11(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_13(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_12(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_14(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_13(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_15(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_14(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_16(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_15(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_17(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_16(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_18(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_17(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_19(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_18(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_20(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_19(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_21(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_20(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_23(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_21(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_24(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_23(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_25(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_24(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_26(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_25(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_27(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_26(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_28(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_27(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_29(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_28(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_30(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_29(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_31(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_30(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_32(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_31(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_33(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_32(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_34(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_33(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_35(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_34(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_36(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_35(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_37(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_36(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_38(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_37(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_39(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_38(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_40(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_39(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_41(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_40(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_42(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_41(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_43(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_42(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_44(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_43(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_45(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_44(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_46(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_45(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_47(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_46(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_48(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_47(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_49(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_48(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_50(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_49(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_51(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_50(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_52(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_51(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_53(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_52(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_54(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_53(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_55(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_54(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_56(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_55(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_57(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_56(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_58(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_57(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_59(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_58(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_60(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_59(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_61(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_60(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_62(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_61(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_63(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_62(m, __VA_ARGS__)
#define LIBGS_FIELD_MAP_COMMA_64(m, x, ...)  m(x), LIBGS_FIELD_MAP_COMMA_63(m, __VA_ARGS__)

#define LIBGS_PP_PROBE() ~, 1
#define LIBGS_PP_SECOND(a, b, ...) b
#define LIBGS_PP_IS_PROBE(...) LIBGS_PP_SECOND(__VA_ARGS__, 0)
#define LIBGS_PP_PROBE_PAREN(...) LIBGS_PP_PROBE()
#define LIBGS_PP_IS_PAREN(x) LIBGS_PP_IS_PROBE(LIBGS_PP_PROBE_PAREN x)

#define LIBGS_PP_IF_0(t, f) f
#define LIBGS_PP_IF_1(t, f) t
#define LIBGS_PP_IF(c) LIBGS_CAT(LIBGS_PP_IF_, c)

#define LIBGS_PP_UNPAREN(...) __VA_ARGS__
#define LIBGS_PP_MAYBE_UNPAREN(x) \
	LIBGS_PP_IF(LIBGS_PP_IS_PAREN(x))(LIBGS_PP_UNPAREN x, x)

#define LIBGS_FIELD_DECL_IMPL(type, name, ...)  LIBGS_PP_MAYBE_UNPAREN(type) name {__VA_ARGS__};
#define LIBGS_FIELD_DECL(x)  LIBGS_FIELD_DECL_IMPL x

#define LIBGS_FIELD_NAME_IMPL(type, name, ...)  name
#define LIBGS_FIELD_NAME(x)  LIBGS_FIELD_NAME_IMPL x

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STREAMER_H
