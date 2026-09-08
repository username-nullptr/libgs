// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_UTILS_SBUS_PUBLISH_H
#define LIBGS_UTILS_UTILS_SBUS_PUBLISH_H

#include <libgs/utils/sbus/interface.h>

namespace libgs::utils::sbus
{

namespace concepts
{

template <typename T>
concept unregistered_type =
	not std::is_pointer_v<std::remove_cvref_t<T>> and
	not libgs::concepts::any_string<T> and
	not topic_type<T>;

template <typename T>
concept unregistered_type_p = unregistered_type<std::remove_cvref_t<T>>;

} //namespace concepts

template <concepts::interface Interface>
LIBGS_UTILS_TAPI void publish (
	std::string_view topic, const void *buffer, size_t size
);

template <concepts::interface Interface, libgs::concepts::any_string_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::interface Interface, concepts::unregistered_type_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::interface Interface, concepts::topic_type...Args>
LIBGS_UTILS_TAPI void publish(Args&&...args)
	requires (sizeof...(Args) > 0);

LIBGS_UTILS_API void publish (
	std::string_view topic, const void *buffer, size_t size
);

template <libgs::concepts::any_string_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::unregistered_type_p...Args>
LIBGS_UTILS_TAPI void publish(std::string_view topic, Args&&...args)
	requires (sizeof...(Args) > 0);

template <concepts::topic_type...Args>
LIBGS_UTILS_TAPI void publish(Args&&...args)
	requires (sizeof...(Args) > 0);

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/publish.h>


#endif //LIBGS_UTILS_UTILS_SBUS_PUBLISH_H
