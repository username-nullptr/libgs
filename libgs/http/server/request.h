
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
	using endpoint_t = typename next_layer_t::endpoint_type;

	using parser_t = basic_request_parser<CharT>;
	using string_t = typename parser_t::string_t;
	using string_view_t = std::basic_string_view<CharT>;
	using value_t = basic_value<CharT>;

	using header_t = typename parser_t::header_t;
	using headers_t = typename parser_t::headers_t;
	using cookies_t = typename parser_t::cookies_t;
	using parameters_t = typename parser_t::parameters_t;

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

	template <asio::completion_token_for<void(size_t,error_code)> Token>
	auto async_read(const mutable_buffer &buf, Token &&token);

public:
	[[nodiscard]] std::string read_all();
	[[nodiscard]] std::string read_all(error_code &error) noexcept;

	template <asio::completion_token_for<void(std::string_view,error_code)> Token>
	auto async_read_all(Token &&token);

public:
	size_t save_file(std::string_view file_name, const req_range &range = {});
	size_t save_file(std::string_view file_name, const req_range &range, error_code &error) noexcept;
	size_t save_file(std::string_view file_name, error_code &error) noexcept;

	template <asio::completion_token_for<void(size_t,error_code)> Token>
	auto async_save_file(std::string_view file_name, const req_range &range, Token &&token);

	template <asio::completion_token_for<void(size_t,error_code)> Token>
	auto async_save_file(std::string_view file_name, Token &&token);

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
	basic_server_request &cancel() noexcept;

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
