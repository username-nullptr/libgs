
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


template <concepts::stream_requires Stream, core_concepts::char_type CharT>
class LIBGS_HTTP_VAPI basic_server_response
{
	LIBGS_DISABLE_COPY(basic_server_response)

public:
	using next_layer_t = basic_server_request<Stream,CharT>;
	using executor_t = typename next_layer_t::executor_t;

	using helper_t = basic_response_helper<CharT>;
	using string_t = typename next_layer_t::string_t;
	using string_view_t = typename next_layer_t::string_view_t;

	using header_t = typename next_layer_t::header_t;
	using headers_t = typename next_layer_t::headers_t;

	using cookie_t = basic_cookie<CharT>;
	using cookies_t = basic_cookies<CharT>;

	using value_t = typename next_layer_t::value_t;
	using value_list_t = basic_value_list<CharT>;

public:
	explicit basic_server_response(next_layer_t &&next_layer);
	~basic_server_response();

	basic_server_response(basic_server_response &&other) noexcept;
	basic_server_response &operator=(basic_server_response &&other) noexcept;

	template <typename Stream0>
	basic_server_response(basic_server_response<Stream0,CharT> &&other) noexcept
		requires core_concepts::constructible<next_layer_t,basic_server_request<Stream0,CharT>&&>;

	template <typename Stream0>
	basic_server_response &operator=(basic_server_response<Stream0,CharT> &&other) noexcept
		requires core_concepts::assignable<Stream,Stream0&&>;

public:
	basic_server_response &set_status(uint32_t status);
	basic_server_response &set_status(http::status status);

	basic_server_response &set_header(string_view_t key, value_t value);
	basic_server_response &set_cookie(string_view_t key, cookie_t cookie);

public:
	size_t write(const const_buffer &body = {nullptr,0});
	size_t write(const const_buffer &body, error_code &error);
	size_t write(error_code &error);

	[[nodiscard]] awaitable<size_t> co_write(const const_buffer &body = {nullptr,0});
	[[nodiscard]] awaitable<size_t> co_write(const const_buffer &body, error_code &error);
	[[nodiscard]] awaitable<size_t> co_write(error_code &error);

public:
	size_t redirect(string_view_t url, http::redirect redi = http::redirect::moved_permanently);
	size_t redirect(string_view_t url, http::redirect redi, error_code &error);
	size_t redirect(string_view_t url, error_code &error);

	[[nodiscard]] awaitable<size_t> co_redirect(string_view_t url, http::redirect redi = http::redirect::moved_permanently);
	[[nodiscard]] awaitable<size_t> co_redirect(string_view_t url, http::redirect redi, error_code &error);
	[[nodiscard]] awaitable<size_t> co_redirect(string_view_t url, error_code &error);

public:
	size_t send_file(std::string_view file_name, const resp_ranges &ranges = {});
	size_t send_file(std::string_view file_name, const resp_ranges &ranges, error_code &error);
	size_t send_file(std::string_view file_name, error_code &error);

	[[nodiscard]] awaitable<size_t> co_send_file(std::string_view file_name, const resp_ranges &ranges = {});
	[[nodiscard]] awaitable<size_t> co_send_file(std::string_view file_name, const resp_ranges &ranges, error_code &error);
	[[nodiscard]] awaitable<size_t> co_send_file(std::string_view file_name, error_code &error);

public:
	basic_server_response &set_chunk_attribute(value_t attribute);
	basic_server_response &set_chunk_attributes(value_list_t attributes);

	size_t chunk_end(const headers_t &headers = {});
	size_t chunk_end(const headers_t &headers, error_code &error);
	size_t chunk_end(error_code &error);

	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers = {});
	[[nodiscard]] awaitable<size_t> co_chunk_end(const headers_t &headers, error_code &error);
	[[nodiscard]] awaitable<size_t> co_chunk_end(error_code &error);

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

	[[nodiscard]] bool headers_writed() const noexcept;
	[[nodiscard]] bool chunk_end_writed() const noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_server_response &cancel() noexcept;

public:
	basic_server_response &unset_header(string_view_t key);
	basic_server_response &unset_cookie(string_view_t key);
	basic_server_response &unset_chunk_attribute(const value_t &attribute);

public:
	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec>
using basic_tcp_server_response = basic_server_response<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_server_response = basic_server_response<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_server_response = basic_tcp_server_response<asio::any_io_executor>;
using wtcp_server_response = wbasic_tcp_server_response<asio::any_io_executor>;

using server_response = tcp_server_response;
using wserver_response = wtcp_server_response;

} //namespace libgs::http
#include <libgs/http/server/detail/response.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_H
