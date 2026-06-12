
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
concept interface = []() consteval -> bool
{
	using topic_t = Interface::topic_t;
	return libgs::concepts::constructible<Interface> and
	requires(Interface &interface, const topic_t &topic, uint64_t sid, const char *buffer, size_t size)
	{
		Interface::publish(topic, buffer, size);
		sid = interface.subscribe (
			[](const topic_t&, const void*, size_t) {}
		);
		interface.cancel_topic(topic);
		interface.cancel_sid(sid);
		interface.cancel();
	};
}();

template <typename T, typename Interface>
concept topic_type = interface<Interface> and []() consteval -> bool
{
	using topic_t = Interface::topic_t;
	return requires(const topic_t &topic) {
		topic_t(std::remove_cvref_t<T>::libgs_sbus_topic_v);
		topic == std::remove_cvref_t<T>::libgs_sbus_topic_v;
	};
}();

#define LIBGS_SBUS_TYPE(Interface, value) \
static constexpr Interface::topic_t libgs_sbus_topic_v = value;

#define LIBGS_LOC_SBUS_TYPE(value) \
static constexpr const char *libgs_sbus_topic_v = #value;

} //namespace concepts

class LIBGS_UTILS_API local_interface final :
	public std::enable_shared_from_this<local_interface>
{
	LIBGS_DISABLE_COPY_MOVE(local_interface)

public:
	using topic_t = std::string_view;

	local_interface();
	~local_interface();

	static void publish(topic_t topic, const void *buffer, size_t size);

	uint64_t subscribe(topic_t topic, std::function<void(const void*, size_t)> func);
	uint64_t subscribe(std::function<void(topic_t topic, const void*, size_t)> func);

	void cancel_topic(topic_t topic);
	void cancel_sid(uint64_t sid);
	void cancel();

private:
	class impl;
	std::unique_ptr<impl> m_impl {};
};

} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_INTERFACE_H