
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

#ifndef LIBGS_CORE_CXX_DETAIL_STREAMER_STATEFUL_H
#define LIBGS_CORE_CXX_DETAIL_STREAMER_STATEFUL_H

#include <optional>
#include <variant>

namespace libgs
{

template <>
struct streamer<std::monostate>
{
	[[nodiscard]] static auto encode(std::monostate) noexcept {
		return std::vector<std::byte> {};
	}
	[[nodiscard]] static decoder_data<std::monostate>
	decode(const std::vector<std::byte>&, size_t = 0) noexcept {
		return { {}, 0 };
	}
};

template <typename T>
struct streamer<std::optional<T>>
{
	using optional_t = std::optional<T>;

	[[nodiscard]] static auto encode(const optional_t &v)
	{
		std::vector<std::byte> buf;
		buf.emplace_back(static_cast<std::byte>(v.has_value()));
		if( v )
		{
			auto sub = streamer<T>::encode(*v);
			buf.insert(buf.end(),
				std::make_move_iterator(sub.begin()),
				std::make_move_iterator(sub.end())
			);
		}
		return buf;
	}

	[[nodiscard]] static decoder_data<optional_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() <= offset )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset
			));
		}
		auto has_value = std::to_integer<bool>(buf[offset++]);
		if( not has_value )
			return { std::nullopt, 1 };

		auto data = streamer<T>::decode(buf, offset);
		return { optional_t { std::move(*data) }, 1 + data.size };
	}
};

template <typename T>
struct streamer<optional<T>>
{
	using optional_t = optional<T>;

	[[nodiscard]] static auto encode(const optional_t &v)
	{
		std::vector<std::byte> buf;
		buf.emplace_back(static_cast<std::byte>(v.has_value()));
		if( v )
		{
			auto sub = streamer<T>::encode(*v);
			buf.insert(buf.end(),
				std::make_move_iterator(sub.begin()),
				std::make_move_iterator(sub.end())
			);
		}
		return buf;
	}

	[[nodiscard]] static decoder_data<optional_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() <= offset )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset
			));
		}
		auto has_value = std::to_integer<bool>(buf[offset++]);
		if( not has_value )
			return { nullopt, 1 };

		auto data = streamer<T>::decode(buf, offset);
		return { optional_t { std::move(*data) }, 1 + data.size };
	}
};

template <typename...Ts>
struct streamer<std::variant<Ts...>>
{
	using variant_t = std::variant<Ts...>;

	[[nodiscard]] static auto encode(const variant_t &v)
	{
		if( v.valueless_by_exception() )
			runtime_error::loc_throw("bad variant");

		std::vector<std::byte> buf;
		buf.resize(8);
		*reinterpret_cast<uint64_t*>(buf.data()) = v.index();

		std::visit([&]<typename T>(const T &value) {
			auto sub = streamer<std::remove_cvref_t<T>>::encode(value);
			buf.insert(buf.end(),
				std::make_move_iterator(sub.begin()),
				std::make_move_iterator(sub.end())
			);
		}, v);
		return buf;
	}

	[[nodiscard]] static decoder_data<variant_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		if( buf.size() < offset + 8 )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} / {} bytes", buf.size(), offset + 8
			));
		}
		auto index = *reinterpret_cast<const uint64_t*>(buf.data() + offset);
		if( index >= sizeof...(Ts) )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} [{}]", sizeof...(Ts), index
			));
		}
		offset += 8;
		size_t sum = 8;
		return { decode_impl(index, buf, offset, sum), sum };
	}

private:
	template <size_t I = 0>
	[[nodiscard]] static variant_t decode_impl(
		size_t index, const std::vector<std::byte> &buf, size_t &offset, size_t &sum)
	{
		if constexpr( I >= sizeof...(Ts) )
		{
			runtime_error::loc_throw(std::format (
				"bad packet: {} [{}]", sizeof...(Ts), I
			));
		}
		else
		{
			if( index == I )
			{
				using T = std::variant_alternative_t<I, variant_t>;
				auto data = streamer<T>::decode(buf, offset);

				offset += data.size;
				sum += data.size;

				return variant_t { std::in_place_index<I>, std::move(*data) };
			}
			return decode_impl<I + 1>(index, buf, offset, sum);
		}
	}
};

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STREAMER_STATEFUL_H