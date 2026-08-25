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

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_COOKIE_JAR_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_COOKIE_JAR_H

#include <libgs/http_nt/protocol/utils/client/url.h>
#include <libgs/http_nt/protocol/cookie.h>

namespace libgs::http_nt
{

class LIBGS_HTTP_NT_API cookie_jar
{
	LIBGS_DISABLE_COPY_MOVE(cookie_jar)

public:
	struct entry
	{
		std::string name {};
		std::string value {};
		std::string domain {};
		std::string path {};

		optional <
			std::chrono::system_clock::time_point
		> expires {};

		bool secure = false;
		bool http_only = false;
		bool host_only = true;

		uint64_t creation_index = 0;
	};

public:
	cookie_jar();
	~cookie_jar();

	bool store(const url &origin,
		std::string name, const cookie &value
	);
	void store(const url &origin,
		const std::vector<std::pair<std::string,cookie>> &values
	);
	[[nodiscard]] cookie_values cookies_for(const url &target);
	[[nodiscard]] std::vector<entry> entries();

	[[nodiscard]] size_t size();
	void clear() noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http_nt

#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CLIENT_COOKIE_JAR_H
