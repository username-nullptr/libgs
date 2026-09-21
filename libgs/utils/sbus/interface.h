// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_UTILS_SBUS_INTERFACE_H
#define LIBGS_UTILS_UTILS_SBUS_INTERFACE_H

#include <libgs/utils/global.h>

namespace libgs::utils::sbus::concepts
{

template <typename Interface>
concept interface = libgs::concepts::constructible<Interface> and requires
	(Interface &interface, std::string_view topic, uint64_t sid, const char *buffer, size_t size)
	{
		Interface::publish(topic, buffer, size);
		sid = interface.subscribe (
			[](std::string_view, const void*, size_t) {}
		);
		interface.cancel_topic(topic);
		interface.cancel_sid(sid);
		interface.cancel();
	};

template <typename T>
concept topic_type = requires(std::string_view topic) {
	std::string_view(std::remove_cvref_t<T>::libgs_sbus_topic_v);
	topic == std::remove_cvref_t<T>::libgs_sbus_topic_v;
};

#define LIBGS_UTILS_SBUS_TYPE_IMPL(value) \
	static constexpr std::string_view libgs_sbus_topic_v = value;

#define LIBGS_UTILS_SBUS_TOPIC(value) \
	"libgs.utils.sbus.topic." #value

#define LIBGS_UTILS_SBUS_TYPE(value) \
	LIBGS_UTILS_SBUS_TYPE_IMPL(LIBGS_UTILS_SBUS_TOPIC(value));

#define LIBGS_UTILS_SBUS_META_TYPE(value, ...) \
	LIBGS_UTILS_SBUS_TYPE(value) LIBGS_META_FIELDS(__VA_ARGS__)

#define LIBGS_UTILS_SBUS_AUTO_TOPIC \
	"libgs.utils.sbus.topic." __FILE__ LIBGS_SHARP(:LIBGS_AUTO_XX_NAME())

#define LIBGS_UTILS_SBUS_AUTO_TYPE \
	LIBGS_UTILS_SBUS_TYPE_IMPL(LIBGS_UTILS_SBUS_AUTO_TOPIC);

#define LIBGS_UTILS_SBUS_AUTO_META_TYPE(...) \
	LIBGS_UTILS_SBUS_AUTO_TYPE LIBGS_META_FIELDS(__VA_ARGS__)

} //namespace libgs::utils::sbus::concepts

#include <libgs/utils/sbus/detail/local_interface.h>
#include <libgs/utils/sbus/detail/udp_interface.h>

namespace libgs::utils::sbus
{

#if LIBGS_UTILS_SBUS_DEFAULT_INTERFACE_UDP
using default_interface = udp_interface;
#else //local
using default_interface = local_interface;
#endif //

} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_INTERFACE_H
