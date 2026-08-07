
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

#ifndef LIBGS_UTILS_UTILS_SBUS_INTERFACE_H
#define LIBGS_UTILS_UTILS_SBUS_INTERFACE_H

#include <libgs/utils/global.h>

namespace libgs::utils::sbus { namespace concepts
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

} //namespace concepts

class LIBGS_UTILS_API local_interface final :
	public std::enable_shared_from_this<local_interface>
{
	LIBGS_DISABLE_COPY_MOVE(local_interface)

public:
	local_interface();
	~local_interface();

	static void publish(std::string_view topic, const void *buffer, size_t size);

	uint64_t subscribe(std::string_view topic, std::function<void(const void*, size_t)> func);
	uint64_t subscribe(std::function<void(std::string_view topic, const void*, size_t)> func);

	void cancel_topic(std::string_view topic);
	void cancel_sid(uint64_t sid);
	void cancel();

private:
	class impl;
	std::unique_ptr<impl> m_impl {};
};

} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_INTERFACE_H
