
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_URL_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_URL_H

#include <libgs/http/protocol/types.h>

namespace libgs::http::protocol
{

class LIBGS_HTTP_API url
{
public:
	using value_t = libgs::value;
	using parameters_t = protocol::parameters;

	template <typename...Args>
	using format_string = std::format_string <
		std::type_identity_t<Args>...
	>;

public:
	template <typename Arg0, typename...Args>
	url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	url(std::string_view url);
	url(const char *url);

	url();
	~url();

	url(const url &other);
	url &operator=(const url &other);

	url(url &&other) noexcept;
	url &operator=(url &&other) noexcept;

public:
	template <typename Arg0, typename...Args>
	url &set(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	url &set(std::string_view url);

	url &set_address(std::string addr);
	url &set_port(uint16_t port);
	url &set_path(std::string_view path);

	url &set_parameter(core_concepts::text_p<char> auto &&key, value_t value) noexcept;
	url &unset_parameter(const core_concepts::text_p<char> auto &key) noexcept;

public:
	[[nodiscard]] std::string_view protocol() const noexcept;
	[[nodiscard]] std::string_view address() const noexcept;
	[[nodiscard]] uint16_t port() const noexcept;

	[[nodiscard]] std::string_view path() const noexcept;
	[[nodiscard]] const parameters_t &parameters() const noexcept;
	[[nodiscard]] parameters_t &parameters() noexcept;

public:
	[[nodiscard]] std::string to_string() const noexcept;
	[[nodiscard]] operator std::string() const noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http::protocol
#include <libgs/http/protocol/utils/client/detail/url.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_URL_H
