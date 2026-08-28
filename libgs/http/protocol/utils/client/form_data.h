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

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_FORM_DATA_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_FORM_DATA_H

#include <libgs/http/protocol/utils/client/request_arg.h>

namespace libgs::http
{

struct LIBGS_HTTP_API form_data_part
{
	headers fields {};
	std::string name {};
	std::string filename {};
	std::string data {};
};

using form_data_parts = std::vector<form_data_part>;

class LIBGS_HTTP_API multipart_form_data
{
public:
	explicit multipart_form_data(std::string boundary = {});
	~multipart_form_data();

	multipart_form_data(const multipart_form_data &other);
	multipart_form_data &operator=(const multipart_form_data &other);

	multipart_form_data(multipart_form_data &&other) noexcept;
	multipart_form_data &operator=(multipart_form_data &&other) noexcept;

public:
	multipart_form_data &add_file(std::string name, std::string filename,
		std::string data, std::string content_type = "application/octet-stream"
	);
	multipart_form_data &add_field(std::string name, std::string value);
	multipart_form_data &add_part(form_data_part part);

	[[nodiscard]] std::string_view boundary() const noexcept;
	[[nodiscard]] std::string content_type() const;

	[[nodiscard]] std::string body() const;
	[[nodiscard]] const form_data_parts &parts() const noexcept;

	request_arg &apply(request_arg &argument) const;

private:
	class impl;
	impl *m_impl;
};

[[nodiscard]] LIBGS_HTTP_API optional<std::string>
form_data_boundary(std::string_view content_type) noexcept;

[[nodiscard]] LIBGS_HTTP_API sys_expected<form_data_parts>
parse_multipart_form_data(std::string_view content_type, std::string_view body) noexcept;

} //namespace libgs::http

#endif //LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_FORM_DATA_H
