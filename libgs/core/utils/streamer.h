// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_CXX_STREAMER_H
#define LIBGS_CORE_CXX_STREAMER_H

#include <libgs/core/cxx/type_traits.h>

namespace libgs
{

template <typename>
struct streamer {};

template <typename T>
struct decoder_data
{
	T data {};
	size_t size = 0;

	const T &operator*() const noexcept { return data; }
	T &operator*() noexcept { return data; }

	const T *operator->() const noexcept { return &data; }
	T *operator->() noexcept { return &data; }

	operator const T&() const noexcept { return data; }
	operator T&() noexcept { return data; }
};

#define LIBGS_SERIALIZE_FIELDS(...) \
	auto meta_fields() { return std::tie(__VA_ARGS__); } \
	auto meta_fields() const { return std::tie(__VA_ARGS__); }

#define LIBGS_META_FIELDS(...) \
	LIBGS_FIELD_MAP(LIBGS_FIELD_DECL, __VA_ARGS__) \
	LIBGS_SERIALIZE_FIELDS(LIBGS_FIELD_MAP_COMMA(LIBGS_FIELD_NAME,__VA_ARGS__))

} //namespace libgs
#include <libgs/core/utils/detail/streamer.h>
#include <libgs/core/utils/detail/streamer_container.h>
#include <libgs/core/utils/detail/streamer_stateful.h>
#include <libgs/core/utils/detail/streamer_chrono.h>
#include <libgs/core/utils/detail/streamer_custom.h>
#include <libgs/core/utils/detail/streamer_asio.h>


#endif //LIBGS_CORE_CXX_STREAMER_H
