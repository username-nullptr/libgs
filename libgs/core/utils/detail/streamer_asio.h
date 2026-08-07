
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

#ifndef LIBGS_CORE_CXX_DETAIL_STREAMER_ASIO_H
#define LIBGS_CORE_CXX_DETAIL_STREAMER_ASIO_H

#include <asio.hpp>
#include <chrono>
#include <ctime>

namespace libgs
{

template <>
struct streamer<asio::ip::address>
{
	using addr_t = asio::ip::address;

	[[nodiscard]] static auto encode(const addr_t &v) {
		return streamer<std::string>::encode(v.to_string());
	}

	[[nodiscard]] static decoder_data<addr_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		auto data = streamer<std::string>::decode(buf, offset);
		return { asio::ip::make_address(*data), data.size };
	}
};

template <>
struct streamer<asio::ip::address_v4>
{
	using addr_t = asio::ip::address_v4;

	[[nodiscard]] static auto encode(const addr_t &v) {
		return streamer<std::string>::encode(v.to_string());
	}

	[[nodiscard]] static decoder_data<addr_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		auto data = streamer<std::string>::decode(buf, offset);
		return { asio::ip::make_address_v4(*data), data.size };
	}
};

template <>
struct streamer<asio::ip::address_v6>
{
	using addr_t = asio::ip::address_v6;

	[[nodiscard]] static auto encode(const addr_t &v) {
		return streamer<std::string>::encode(v.to_string());
	}

	[[nodiscard]] static decoder_data<addr_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		auto data = streamer<std::string>::decode(buf, offset);
		return { asio::ip::make_address_v6(*data), data.size };
	}
};

template <typename InternetProtocol>
struct streamer<asio::ip::basic_endpoint<InternetProtocol>>
{
	using endpoint_t = asio::ip::basic_endpoint<InternetProtocol>;

	[[nodiscard]] static auto encode(const endpoint_t &v)
	{
		auto buf = streamer<asio::ip::address>::encode(v.address());
		auto sub = streamer<asio::ip::port_type>::encode(v.port()); // uint16_t
		buf.insert(buf.end(),
			std::make_move_iterator(sub.begin()),
			std::make_move_iterator(sub.end())
		);
		return buf;
	}

	[[nodiscard]] static decoder_data<endpoint_t>
	decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		auto addr = streamer<asio::ip::address>::decode(buf, offset);
		auto port = streamer<asio::ip::port_type>::decode(buf, offset + addr.size);
		return { endpoint_t { *addr, *port }, addr.size + port.size };
	}
};

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STREAMER_ASIO_H
