
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_CLIENT_REPLY_H
#define LIBGS_HTTP_CLIENT_REPLY_H

#include <libgs/http/protocol/utils/client/parser.h>
#include <libgs/http/utils/connection.h>

namespace libgs::http
{

template <concepts::connection Connection = connection>
class LIBGS_HTTP_TAPI basic_reply
{
	LIBGS_DISABLE_COPY(basic_reply)

public:
	using connection_t = Connection;
	using executor_t = connection_t::executor_t;
	using parser_t = protocol::client_parser;

	using value_t = parser_t::value_t;
	using headers_t = parser_t::headers_t;

	using cookie_t = parser_t::cookie_t;
	using cookies_t = parser_t::cookies_t;

public:
	explicit basic_reply(connection_t &&connection);
	basic_reply(connection_t &&connection, parser_t &&parser);
	~basic_reply();

	basic_reply(basic_reply &&other) noexcept;
	basic_reply &operator=(basic_reply &&other) noexcept;

public:
	[[nodiscard]] protocol::version_enum version() const noexcept;
	[[nodiscard]] protocol::status_enum status() const noexcept;

	[[nodiscard]] optional<value_t> header(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;

	[[nodiscard]] optional<cookie_t> cookie(const core_concepts::text_p<char> auto &key) const noexcept;
	[[nodiscard]] const protocol::cookies &cookies() const noexcept;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		core_concepts::tf_opt_token<Token,error_code,Value...> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename Token = use_sync_t>
	auto parse(connection_t &&connection, Token &&token = {}) noexcept
		requires task_token_v<Token,protocol::status_enum>;

	template <typename Token = use_sync_t>
	auto parse(Token &&token = {}) noexcept
		requires task_token_v<Token,protocol::status_enum>;

public:
	template <typename Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {}) noexcept
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto read(Token &&token = {}) noexcept
		requires task_token_v<Token,std::string>;

	template <typename T>
	static constexpr bool file_opt_token = concepts::file_opt_token_p <
		T, char, file_optype::single, io_permission::write
	>;
	template <typename T, typename Token = use_sync_t>
	auto download_file(T &&opt, Token &&token = {}) noexcept
		requires file_opt_token<T> and task_token_v<Token,size_t>;

public:
	[[nodiscard]] bool valid() const noexcept;
	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

	[[nodiscard]] const connection_t &connection() const noexcept;
	[[nodiscard]] connection_t &connection() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_reply &cancel() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using reply = basic_reply<>;

} //namespace libgs::http
#include <libgs/http/client/detail/reply.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http {
using ssl_reply = basic_reply<ssl_connection>;
} //namespace http

namespace https {
using reply = http::ssl_reply;
}} //namespace libgs::https

#endif //LIBGS_ENABLE_OPENSS
#endif //LIBGS_HTTP_CLIENT_REPLY_H