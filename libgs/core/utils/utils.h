// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_UTILS_UTILITIES_H
#define LIBGS_CORE_UTILS_UTILITIES_H

#include <libgs/core/utils/asio_concepts.h>
#include <libgs/core/utils/streamer.h>

#include <libgs/core/cxx/string_concepts.h>
#include <libgs/core/cxx/attributes.h>

namespace libgs
{

enum class ip_type {
	v4, v6, loopback
};

template <typename Protocol>
struct LIBGS_CORE_TAPI basic_endpoint_wrapper
{
	using protocol_t = Protocol;
	using endpoint_t = asio::ip::basic_endpoint<protocol_t>;
	endpoint_t value;

	basic_endpoint_wrapper() = default;
	basic_endpoint_wrapper(const concepts::any_string_p auto &address, uint16_t port);
	basic_endpoint_wrapper(const concepts::any_string_p auto &address);

	basic_endpoint_wrapper(ip_type type, uint16_t port);
	basic_endpoint_wrapper(ip_type type);

	template <typename...Args>
	basic_endpoint_wrapper(Args&&...args) requires
		concepts::constructible<endpoint_t,Args&&...>;

	operator endpoint_t&();
	operator const endpoint_t&() const;

	endpoint_t &operator*();
	endpoint_t *operator->();
};

using tcp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::tcp>;
using udp_endpoint_wrapper = basic_endpoint_wrapper<asio::ip::udp>;

} //namespace libgs
#include <libgs/core/utils/detail/utils.h>


#endif //LIBGS_CORE_UTILS_UTILITIES_H
