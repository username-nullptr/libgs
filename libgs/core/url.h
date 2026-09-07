
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

#ifndef LIBGS_CORE_URL_H
#define LIBGS_CORE_URL_H

#include <libgs/core/container.h>

namespace libgs
{

class LIBGS_CORE_API url : public mutable_parameters<url>
{
public:
	template <typename...Args>
	using format_string = std::format_string <
		std::type_identity_t<Args>...
	>;

public:
	template <typename Arg0, typename...Args>
	url(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	url(std::string_view url_text);
	url(const std::string &url);
	url(const char *url);

	url();
	~url() override;

	url(const url &other);
	url &operator=(const url &other);

	url(url &&other) noexcept;
	url &operator=(url &&other) noexcept;

public:
	template <typename Arg0, typename...Args>
	url &emplace(format_string<Arg0,Args...> fmt, Arg0 &&arg0, Args&&...args);
	url &emplace(std::string_view url_text);

	url &set_address(std::string addr);
	url &set_port(uint16_t port);
	url &set_path(std::string_view path);

public:
	[[nodiscard]] std::string_view protocol() const noexcept;
	[[nodiscard]] std::string_view host() const noexcept;
	[[nodiscard]] uint16_t port() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;
	[[nodiscard]] bool is_valid() const noexcept;

public:
	[[nodiscard]] std::string to_string() const noexcept;
	[[nodiscard]] explicit operator std::string() const noexcept;

	[[nodiscard]] static url resolve (
		const url &base, std::string_view reference
	);

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs
#include <libgs/core/detail/url.h>


#endif //LIBGS_CORE_URL_H
