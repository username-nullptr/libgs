
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

#ifndef LIBGS_HTTP_CLIENT_REPLY_CONTEXT_H
#define LIBGS_HTTP_CLIENT_REPLY_CONTEXT_H

#include <libgs/http/client/reply_parser.h>
#include <libgs/http/opt_token.h>
#include <libgs/core/coroutine.h>

namespace libgs::http
{

template <concepts::stream_requires Stream, core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_client_reply
{
	LIBGS_DISABLE_COPY(basic_client_reply)

public:
	using next_layer_t = Stream;
	using executor_t = typename next_layer_t::executor_type;
	using endpoint_t = typename socket_operation_helper<next_layer_t>::endpoint_t;

	using parser_t = basic_request_parser<CharT>;
	using string_t = typename parser_t::string_t;
	using string_view_t = std::basic_string_view<CharT>;

	using value_t = typename parser_t::value_t;
	using value_list_t = typename parser_t::value_list_t;

	using header_t = typename parser_t::header_t;
	using headers_t = typename parser_t::headers_t;
	using cookies_t = typename parser_t::cookies_t;

public:
	template <typename NextLayer>
	basic_client_reply(NextLayer &&next_layer, parser_t &parser)
		requires core_concepts::constructible<next_layer_t,NextLayer&&>;
	~basic_client_reply();

	basic_client_reply(basic_client_reply &&other) noexcept;
	basic_client_reply &operator=(basic_client_reply &&other) noexcept;

	template <typename Stream0>
	basic_client_reply(basic_client_reply<Stream0,CharT> &&other) noexcept
		requires core_concepts::constructible<Stream,Stream0&&>;

	template <typename Stream0>
	basic_client_reply &operator=(basic_client_reply<Stream0,CharT> &&other) noexcept
		requires core_concepts::assignable<Stream,Stream0&&>;

public:
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] http::status status() const noexcept;

	[[nodiscard]] const value_t &header(string_view_t key) const;
	[[nodiscard]] const value_t &cookie(string_view_t key) const;

	[[nodiscard]] value_t header_or(string_view_t key, value_t def_value = {}) const noexcept;
	[[nodiscard]] value_t cookie_or(string_view_t key, value_t def_value = {}) const noexcept;

public:
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] const value_list_t &chunk_attributes() const noexcept;

public:
	size_t read(const mutable_buffer &buf, error_code &error) noexcept;
	size_t read(const mutable_buffer &buf);

	[[nodiscard]] awaitable<size_t> co_read(const mutable_buffer &buf, error_code &error) noexcept;
	[[nodiscard]] awaitable<size_t> co_read(const mutable_buffer &buf);

public:
	[[nodiscard]] std::string read_all(error_code &error) noexcept;
	[[nodiscard]] std::string read_all();

	[[nodiscard]] awaitable<std::string> co_read_all(error_code &error) noexcept;
	[[nodiscard]] awaitable<std::string> co_read_all();

public:
	size_t save_file(std::string_view file_name, const req_range &range = {});
	size_t save_file(std::string_view file_name, const req_range &range, error_code &error) noexcept;
	size_t save_file(std::string_view file_name, error_code &error) noexcept;

	[[nodiscard]] awaitable<size_t> co_save_file(std::string_view file_name, const req_range &range = {});
	[[nodiscard]] awaitable<size_t> co_save_file(std::string_view file_name, const req_range &range, error_code &error) noexcept;
	[[nodiscard]] awaitable<size_t> co_save_file(std::string_view file_name, error_code &error) noexcept;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool can_read_body() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() const;
	[[nodiscard]] endpoint_t local_endpoint() const;

	[[nodiscard]] const executor_t &get_executor() noexcept;
	basic_client_reply &cancel() noexcept;

public:
	const next_layer_t &next_layer() const noexcept;
	next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec>
using basic_tcp_client_reply = basic_client_reply<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_client_reply = basic_client_reply<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_client_reply = basic_tcp_client_reply<asio::any_io_executor>;
using wtcp_client_reply = wbasic_tcp_client_reply<asio::any_io_executor>;

using client_reply = tcp_client_reply;
using wclient_reply = wtcp_client_reply;

} //namespace libgs::http
#include <libgs/http/client/detail/reply.h>


#endif //LIBGS_HTTP_CLIENT_REPLY_CONTEXT_H