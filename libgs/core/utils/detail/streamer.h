// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_DETAIL_STREAMER_H
#define LIBGS_CORE_CXX_DETAIL_STREAMER_H

#include <libgs/core/utils/flags.h>
#include <libgs/core/utils/string_tools.h>
#include <libgs/core/cxx/cplusplus.h>

namespace libgs { namespace detail
{

inline void streamer_write_u64(std::byte *buffer, uint64_t value) noexcept {
	std::memcpy(buffer, &value, sizeof(value));
}

[[nodiscard]] inline uint64_t streamer_read_u64(const std::byte *buffer) noexcept
{
	uint64_t value = 0;
	std::memcpy(&value, buffer, sizeof(value));
	return value;
}

} //namespace detail

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
		return {
			.data = std::to_integer<bool>(buf[offset]),
			.size = sizeof(bool)
		};
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

		detail::streamer_write_u64(buf.data(), N);
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
		auto size = detail::streamer_read_u64(buf.data() + offset);
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

#define LIBGS_PP_PARENS ()

#define LIBGS_PP_EXPAND(...) \
	LIBGS_PP_EXPAND4(LIBGS_PP_EXPAND4(LIBGS_PP_EXPAND4(LIBGS_PP_EXPAND4(__VA_ARGS__))))

#define LIBGS_PP_EXPAND4(...) \
	LIBGS_PP_EXPAND3(LIBGS_PP_EXPAND3(LIBGS_PP_EXPAND3(LIBGS_PP_EXPAND3(__VA_ARGS__))))

#define LIBGS_PP_EXPAND3(...) \
	LIBGS_PP_EXPAND2(LIBGS_PP_EXPAND2(LIBGS_PP_EXPAND2(LIBGS_PP_EXPAND2(__VA_ARGS__))))

#define LIBGS_PP_EXPAND2(...) \
	LIBGS_PP_EXPAND1(LIBGS_PP_EXPAND1(LIBGS_PP_EXPAND1(LIBGS_PP_EXPAND1(__VA_ARGS__))))

#define LIBGS_PP_EXPAND1(...) __VA_ARGS__

#define LIBGS_FIELD_MAP(m, ...) \
	__VA_OPT__(LIBGS_PP_EXPAND(LIBGS_FIELD_MAP_IMPL(m, __VA_ARGS__)))

#define LIBGS_FIELD_MAP_IMPL(m, x, ...) \
	m(x) __VA_OPT__(LIBGS_FIELD_MAP_AGAIN LIBGS_PP_PARENS (m, __VA_ARGS__))

#define LIBGS_FIELD_MAP_AGAIN() LIBGS_FIELD_MAP_IMPL

#define LIBGS_FIELD_MAP_COMMA(m, ...) \
	__VA_OPT__(LIBGS_PP_EXPAND(LIBGS_FIELD_MAP_COMMA_IMPL(m, __VA_ARGS__)))

#define LIBGS_FIELD_MAP_COMMA_IMPL(m, x, ...) \
	m(x) __VA_OPT__(, LIBGS_FIELD_MAP_COMMA_AGAIN LIBGS_PP_PARENS (m, __VA_ARGS__))

#define LIBGS_FIELD_MAP_COMMA_AGAIN() LIBGS_FIELD_MAP_COMMA_IMPL

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
#define LIBGS_FIELD_DECL_APPLY(...)  LIBGS_FIELD_DECL_IMPL(__VA_ARGS__, )

#define LIBGS_FIELD_DECL(x)  LIBGS_FIELD_DECL_APPLY x

#define LIBGS_FIELD_NAME_IMPL(type, name, ...)  name
#define LIBGS_FIELD_NAME_APPLY(...)  LIBGS_FIELD_NAME_IMPL(__VA_ARGS__, )

#define LIBGS_FIELD_NAME(x)  LIBGS_FIELD_NAME_APPLY x

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STREAMER_H
