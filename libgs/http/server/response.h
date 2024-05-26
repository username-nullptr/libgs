
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

#ifndef LIBGS_HTTP_SERVER_RESPONSE_H
#define LIBGS_HTTP_SERVER_RESPONSE_H

#include <libgs/http/server/request.h>
#include <libgs/http/server/response_helper.h>
#include <libgs/core/value.h>

namespace libgs::http
{

template <typename Request, concept_char_type CharT, typename Derived = void>
class LIBGS_HTTP_VAPI basic_server_response :
	public io::device_base<crtp_derived_t<Derived,basic_server_response<Request,CharT,Derived>>, typename Request::executor_t>
{
	LIBGS_DISABLE_COPY(basic_server_response)

public:
	using next_layer_t = Request;
	using executor_t = typename next_layer_t::executor_t;

	using derived_t = crtp_derived_t<Derived,basic_server_response>;
	using base_t = io::device_base<derived_t,executor_t>;

	using helper_t = basic_response_helper<CharT>;
	using string_t = typename next_layer_t::string_t;
	using string_view_t = typename next_layer_t::string_view_t;

	using headers_t = typename next_layer_t::headers_t;
	using cookie_t = basic_cookie<CharT>;
	using cookies_t = basic_cookies<CharT>;

	using value_t = typename next_layer_t::value_t;
	using value_list_t = basic_value_list<CharT>;

	template <typename T>
	using buffer = io::buffer<T>;

public:
	explicit basic_server_response(next_layer_t &&next_layer);
	~basic_server_response();

	basic_server_response(basic_server_response &&other) noexcept;
	basic_server_response &operator=(basic_server_response &&other) noexcept;

public:
	derived_t &set_status(uint32_t status);
	derived_t &set_status(http::status status);

	derived_t &set_header(string_view_t key, value_t value);
	derived_t &set_cookie(string_view_t key, cookie_t cookie);

public:
	[[nodiscard]] awaitable<size_t> write(opt_token<error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> write(buffer<std::string_view> buf, opt_token<error_code&> tk = {});

	[[nodiscard]] awaitable<size_t> redirect(string_view_t url, opt_token<http::redirect,error_code&> tk = {});
	[[nodiscard]] awaitable<size_t> send_file(std::string_view file_name, opt_token<ranges,error_code&> tk = {});

public:
	derived_t &set_chunk_attribute(value_t attribute);
	derived_t &set_chunk_attributes(value_list_t attributes);
	[[nodiscard]] awaitable<size_t> chunk_end(opt_token<const headers_t&, error_code&> tk = {});

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

	[[nodiscard]] bool headers_writed() const noexcept;
	[[nodiscard]] bool chunk_end_writed() const noexcept;

	derived_t &cancel() noexcept;

public:
	derived_t &unset_header(string_view_t key);
	derived_t &unset_cookie(string_view_t key);
	derived_t &unset_chunk_attribute(const value_t &attribute);

public:
	const next_layer_t &next_layer() const noexcept;
	next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <concept_execution Exec>
using basic_tcp_server_response = basic_server_response<basic_tcp_server_request<Exec>,char>;

template <concept_execution Exec>
using basic_tcp_server_wresponse = basic_server_response<basic_tcp_server_request<Exec>,wchar_t>;

using tcp_server_response = basic_tcp_server_response<asio::any_io_executor>;
using tcp_server_wresponse = basic_tcp_server_wresponse<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/response.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_H
