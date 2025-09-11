
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

#include <libgs/http/protocol/utils/server/parser.h>
#include <libgs/http/utils/socket_operation_helper.h>
#include <libgs/http/utils/request_template.h>

namespace libgs::http
{

template <concepts::stream Stream>
using basic_server_request = basic_request<protocol::model::server,Stream>;

template <concepts::stream Stream>
class LIBGS_HTTP_TAPI basic_request<protocol::model::server,Stream>
{
	LIBGS_DISABLE_COPY(basic_request)

public:
	using next_layer_t = Stream;
	using executor_t = next_layer_t::executor_type;
	using endpoint_t = socket_operation_helper<next_layer_t>::endpoint_t;

	using parser_t = protocol::server_parser;
	using value_t = parser_t::value_t;
	using path_args_t = parser_t::path_args_t;

	using parameters_t = protocol::parameters;
	using headers_t = protocol::headers;

public:
	template <typename NextLayer>
	basic_request(NextLayer &&next_layer, parser_t &parser)
		requires core_concepts::constructible<next_layer_t,NextLayer&&>;
	~basic_request();

	basic_request(basic_request &&other) noexcept;
	basic_request &operator=(basic_request &&other) noexcept;

	template <typename Stream0>
	basic_request(basic_server_request<Stream0> &&other) noexcept
		requires core_concepts::constructible<Stream,Stream0&&>;

	template <typename Stream0>
	basic_request &operator=(basic_server_request<Stream0> &&other) noexcept
		requires core_concepts::assignable<Stream,Stream0&&>;

public:
	[[nodiscard]] protocol::method_enum method() const noexcept;
	[[nodiscard]] protocol::version_enum version() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;

public:
	[[nodiscard]] optional<value_t> parameter(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] optional<value_t> parameter(size_t index) const;
	[[nodiscard]] const parameters_t &parameters() const noexcept;

	[[nodiscard]] optional<value_t> header(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;

	[[nodiscard]] optional<value_t> cookie(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const protocol::cookie_values &cookies() const noexcept;

	[[nodiscard]] optional<value_t> path_arg(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] optional<value_t> path_arg(size_t index) const;

	[[nodiscard]] const path_args_t &path_args() const noexcept;
	int32_t path_match(std::string_view rule);

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto read(Token &&token = {});

	template <typename T>
	static constexpr bool file_opt_token = concepts::file_opt_token_p <
		T, char, file_optype::single, io_permission::write
	>;
	template <typename T, core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto save_file(T &&opt, Token &&token = {}) requires file_opt_token<T>;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool can_read_body() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

public:
	[[nodiscard]] endpoint_t remote_endpoint() const;
	[[nodiscard]] endpoint_t local_endpoint() const;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_request &cancel() noexcept;

public:
	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::exec Exec>
using basic_tcp_server_request = basic_server_request<
	asio::basic_stream_socket<asio::ip::tcp,Exec>
>;

using tcp_server_request = basic_tcp_server_request<asio::any_io_executor>;
using server_request = tcp_server_request;

} //namespace libgs::http
#include <libgs/http/server/detail/request.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_H
