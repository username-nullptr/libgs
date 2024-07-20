
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
#include <libgs/core/coroutine.h>

namespace libgs::http
{

template <typename Stream, concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_server_request
{
	LIBGS_DISABLE_COPY(basic_server_request)

public:
	using next_layer_t = Stream;
	using executor_t = typename next_layer_t::executor_type;
	using endpoint_t = typename next_layer_t::endpoint;

	using parser_t = basic_request_parser<CharT>;
	using string_t = typename parser_t::string_t;
	using string_view_t = std::basic_string_view<CharT>;
	using value_t = basic_value<CharT>;

	using header_t = typename parser_t::header_t;
	using headers_t = typename parser_t::headers_t;
	using cookies_t = typename parser_t::cookies_t;
	using parameters_t = typename parser_t::parameters_t;

	void ffff()
	{
		asio::ip::udp::socket u;
		u.async_receive_from();

		asio::ip::tcp::socket t;
		t.get_executor();

		char b[1111];
		error_code error;
		t.async_read_some(buffer(b,1111), use_awaitable|error);

//		asio::ssl::stream<asio::ip::tcp::socket> ss;
//		ss.get_executor();
	}

public:
	template <typename NextLayer>
	basic_server_request(NextLayer &&next_layer, parser_t &parser)
		requires concept_constructible<next_layer_t, NextLayer>;
	~basic_server_request();

	basic_server_request(basic_server_request &&other) noexcept;
	basic_server_request &operator=(basic_server_request &&other) noexcept;

public:
	[[nodiscard]] http::method method() const noexcept;
	[[nodiscard]] string_view_t version() const noexcept;
	[[nodiscard]] string_view_t path() const noexcept;

	[[nodiscard]] const parameters_t &parameters() const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

public:
	[[nodiscard]] const value_t &parameter(string_view_t key) const;
	[[nodiscard]] const value_t &header(string_view_t key) const;
	[[nodiscard]] const value_t &cookie(string_view_t key) const;

	[[nodiscard]] value_t parameter_or(string_view_t key, value_t def_value = {}) const noexcept;
	[[nodiscard]] value_t header_or(string_view_t key, value_t def_value = {}) const noexcept;
	[[nodiscard]] value_t cookie_or(string_view_t key, value_t def_value = {}) const noexcept;

public:
	size_t read(const mutable_buffer &buf);
	size_t read(const mutable_buffer &buf, error_code &error) noexcept;

	template <concept_callable<std::string,error_code> Func>
	void async_read(const mutable_buffer &buf, Func &&callback) noexcept;

	template <asio::completion_token_for<void> Token>
	[[nodiscard]] awaitable<size_t> async_read(const mutable_buffer &buf, Token &&token);

	size_t async_read(const mutable_buffer &buf, asio::yield_context &yc);

public:
	[[nodiscard]] std::string read_all();
	[[nodiscard]] std::string read_all(error_code &error) noexcept;

	template <concept_callable<size_t,error_code> Func>
	void async_read_all(Func &&callback) noexcept;

	template <asio::completion_token_for<void> Token>
	[[nodiscard]] awaitable<std::string> async_read_all(Token &&token);

	[[nodiscard]] std::string async_read_all(asio::yield_context &yc);

public:
	size_t save_file(const std::string &file_name, const file_range &range = {});
	size_t save_file(const std::string &file_name, const file_range &range, error_code &error) noexcept;
	size_t save_file(const std::string &file_name, error_code &error) noexcept;

	template <concept_callable<size_t,error_code> Func>
	void async_save_file(const std::string &file_name, const file_range &range, Func &&callback) noexcept;

	template <concept_callable<size_t,error_code> Func>
	void async_save_file(const std::string &file_name, Func &&callback) noexcept;

	template <asio::completion_token_for<void> Token>
	[[nodiscard]] awaitable<size_t> async_save_file(const mutable_buffer &buf, const file_range &range, Token &&token);

	template <asio::completion_token_for<void> Token>
	[[nodiscard]] awaitable<size_t> async_save_file(const mutable_buffer &buf, Token &&token);

	size_t async_save_file(const mutable_buffer &buf, const file_range &range, asio::yield_context &yc);
	size_t async_save_file(const mutable_buffer &buf, asio::yield_context &yc);

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool can_read_body() const noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() const;
	[[nodiscard]] endpoint_t local_endpoint() const;
	void cancel() noexcept;

public:
	const next_layer_t &next_layer() const noexcept;
	next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <concept_execution Exec>
using basic_tcp_server_request = basic_server_request<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <concept_execution Exec>
using basic_tcp_server_wrequest = basic_server_request<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_server_request = basic_tcp_server_request<asio::any_io_executor>;
using tcp_server_wrequest = basic_tcp_server_wrequest<asio::any_io_executor>;

} //namespace libgs::http
#include <libgs/http/server/detail/request.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_H
