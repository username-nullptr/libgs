
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_SERVER_REQUEST_H
#define LIBGS_HTTP_SERVER_REQUEST_H

#include <libgs/http/server/request_parser.h>
#include <libgs/http/basic/opt_token.h>
#include <libgs/io/socket.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server_response;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_server_request : public io::device_base<Exec>
{
	LIBGS_DISABLE_COPY_MOVE(basic_server_request)
	using base_type = io::device_base<Exec>;

public:
	using executor_type = typename base_type::executor_type;
	using socket_ptr = io::basic_socket_ptr<Exec>;
	using parser_type = basic_request_parser<CharT>;

	using str_type = typename parser_type::str_type;
	using str_view_type = std::basic_string_view<CharT>;
	using value_type = basic_value<CharT>;

	using header_type = typename parser_type::header_type;
	using headers_type = typename parser_type::headers_type;
	using cookies_type = typename parser_type::cookies_type;
	using parameters_type = typename parser_type::parameters_type;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	basic_server_request(socket_ptr socket, parser_type &parser);
	~basic_server_request() override;

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] str_view_type version() const noexcept;
	[[nodiscard]] str_view_type path() const noexcept;

	[[nodiscard]] const parameters_type &parameters() const noexcept;
	[[nodiscard]] const headers_type &headers() const noexcept;
	[[nodiscard]] const cookies_type &cookies() const noexcept;

public:
	[[nodiscard]] const value_type &parameter(str_view_type key) const;
	[[nodiscard]] const value_type &header(str_view_type key) const;
	[[nodiscard]] const value_type &cookie(str_view_type key) const;

	[[nodiscard]] value_type parameter_or(str_view_type key, value_type def_value = {}) const noexcept;
	[[nodiscard]] value_type header_or(str_view_type key, value_type def_value = {}) const noexcept;
	[[nodiscard]] value_type cookie_or(str_view_type key, value_type def_value = {}) const noexcept;

public:
	[[nodiscard]] awaitable<size_t> read(buffer<void*> buf, opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> read(buffer<std::string&> buf, opt_token<error_code&> tk = {});

	[[nodiscard]] awaitable<std::string> read_all(opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> save_file(const std::string &file_name, opt_token<begin_t,total_t,error_code&> tk = {});

public:
	[[nodiscard]] bool keep_alive() const;
	[[nodiscard]] bool support_gzip() const;
	[[nodiscard]] bool is_chunked() const;
	[[nodiscard]] bool can_read_body() const;

public:
	[[nodiscard]] io::ip_endpoint remote_endpoint() const;
	[[nodiscard]] io::ip_endpoint local_endpoint() const;
	void cancel() noexcept override;

private:
	friend class basic_server_response<CharT,Exec>;
	class impl;
	impl *m_impl;
};

using server_request = basic_server_request<char>;
using server_wrequest = basic_server_request<wchar_t>;

template <concept_char_type CharT, concept_execution Exec = asio::any_io_executor>
using basic_server_request_ptr = std::shared_ptr<basic_server_request<CharT,Exec>>;

using server_request_ptr = std::shared_ptr<basic_server_request<char>>;
using server_wrequest_ptr = std::shared_ptr<basic_server_request<char>>;

} //namespace libgs::http
#include <libgs/http/server/detail/request.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_H
