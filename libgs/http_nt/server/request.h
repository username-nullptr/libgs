
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_NT_SERVER_REQUEST_H
#define LIBGS_HTTP_NT_SERVER_REQUEST_H

#include <libgs/http_nt/protocol/utils/server/parser.h>
#include <libgs/http_nt/protocol/utils/core/upgrade.h>
#include <libgs/http_nt/utils/connection.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_request :
	public const_headers<basic_request<Connection>>,
	public const_cookies<value,basic_request<Connection>>,
	public const_parameters<basic_request<Connection>>
{
	LIBGS_DISABLE_COPY_MOVE(basic_request)

public:
	using connection_t = Connection;
	using connection_ptr = std::shared_ptr<connection_t>;
	using executor_t = connection_t::executor_t;

	using socket_t = connection_t::socket_t;
	using endpoint_t = connection_t::endpoint_t;

	using parser_t = server_parser;
	using value_t = parser_t::value_t;
	using path_args_t = parser_t::path_args_t;

	using parameters_t = http_nt::parameters;
	using headers_t = http_nt::headers;

public:
	explicit basic_request(connection_ptr connection);
	basic_request(connection_ptr connection, parser_t &&parser);
	~basic_request() override;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		core_concepts::dis_func_tf_opt_token<Token,Value...> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename Token = use_sync_t>
	auto wait(Token &&token = {}) requires
		task_token_v<Token,status_enum>;

	int32_t path_match(std::string_view rule);

public:
	[[nodiscard]] method_enum method() const noexcept;
	[[nodiscard]] request_target_form target_form() const noexcept;
	[[nodiscard]] std::string_view target() const noexcept;
	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] std::string_view path() const noexcept;

public:
	[[nodiscard]] optional<value_t> path_arg (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] bool contains_path_arg (
		const core_concepts::text_p<char> auto &key
	) const noexcept;

	[[nodiscard]] optional<value_t> path_arg(size_t index) const;
	[[nodiscard]] bool contains_path_arg(size_t index) const noexcept;

	[[nodiscard]] const parameters_t &path_args() const noexcept;

public:
	template <typename Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {})
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto read(Token &&token = {})
		requires task_token_v<Token,std::string>;

	template <typename T, typename Token>
	static constexpr bool file_task_token_v =
		core_concepts::dis_func_tf_opt_token<Token,size_t> and
		not is_detached_v<std::remove_cvref_t<Token>> and
		concepts::file_opt_token_p <
			T, char, file_optype::single, io_permission::write
		>;
	template <typename T, typename Token = use_sync_t>
	auto save_file(T &&opt, Token &&token = {})
		requires file_task_token_v<T,Token>;

public:
	[[nodiscard]] bool keep_alive() const noexcept;
	[[nodiscard]] bool support_gzip() const noexcept;
	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool can_read_body() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;
	[[nodiscard]] bool is_upgrade() const noexcept;
	[[nodiscard]] std::string take_pending_data();

public:
	[[nodiscard]] endpoint_t remote_endpoint() const;
	[[nodiscard]] endpoint_t local_endpoint() const;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_request &cancel() noexcept;

public:
	[[nodiscard]] const connection_t &connection() const noexcept;
	[[nodiscard]] connection_t &connection() noexcept;

private:
	class impl;
	impl *m_impl;
};

using request = basic_request<connection>;

} //namespace libgs::http_nt

#include <libgs/http_nt/server/detail/request.h>
#if LIBGS_OPENSSL_SUPPORT

namespace libgs { namespace http_nt {
using ssl_request = basic_request<ssl_connection>;
} //namespace http_nt

namespace https_nt {
using request = http_nt::ssl_request;
}} //namespace libgs::https_nt

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_NT_SERVER_REQUEST_H
