
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

#ifndef LIBGS_HTTP_NT_CLIENT_REPLY_H
#define LIBGS_HTTP_NT_CLIENT_REPLY_H

#include <libgs/http_nt/protocol/utils/client/parser.h>
#include <libgs/http_nt/utils/connection.h>

namespace libgs::http_nt
{

template <concepts::connection Connection = connection>
class LIBGS_HTTP_NT_TAPI basic_reply final :
	public const_headers<basic_reply<Connection>>,
	public const_cookies<cookie,basic_reply<Connection>>
{
	LIBGS_DISABLE_COPY_MOVE(basic_reply)

public:
	using connection_t = Connection;
	using connection_ptr = std::shared_ptr<connection_t>;

	using executor_t = connection_t::executor_t;
	using parser_t = client_parser;

	explicit basic_reply(connection_ptr connection);
	basic_reply(connection_ptr connection, parser_t &&parser);
	~basic_reply() override;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		core_concepts::tf_opt_token<Token,error_code,Value...> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename Token = use_sync_t>
	auto wait(Token &&token = {}) noexcept
		requires task_token_v<Token,status_enum>;

	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] status_enum status() const noexcept;

public:
	template <typename Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {}) noexcept
		requires task_token_v<Token,size_t>;

	template <typename Token = use_sync_t>
	auto read(Token &&token = {}) noexcept
		requires task_token_v<Token,std::string>;

	template <typename T, typename Token>
	static constexpr bool file_task_token =
		core_concepts::tf_opt_token<Token,error_code,size_t> and
		concepts::file_opt_token_p <
			T, char, file_optype::single, io_permission::write
		>;

	template <typename T, typename Token = use_sync_t>
	auto save_file(T &&opt, Token &&token = {}) noexcept
		requires file_task_token<T,Token>;

	template <typename T, typename Progress, typename Token = use_sync_t>
	auto save_file(T &&opt, Progress &&progress, Token &&token = {}) noexcept
		requires file_task_token<T,Token> and concepts::progress_callback<Progress,Token>;

public:
	[[nodiscard]] bool valid() const noexcept;
	[[nodiscard]] error_code first_error() const noexcept;

	[[nodiscard]] bool is_chunked() const noexcept;
	[[nodiscard]] bool is_eof() const noexcept;

	[[nodiscard]] const connection_t &connection() const noexcept;
	[[nodiscard]] connection_t &connection() noexcept;

	[[nodiscard]] const parser_t &parser() const noexcept;
	[[nodiscard]] parser_t &parser() noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_reply &cancel() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

} //namespace libgs::http_nt
#include <libgs/http_nt/client/detail/reply.h>

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http_nt {
using ssl_reply = basic_reply<ssl_connection>;
} //namespace http_nt

namespace https_nt {
using reply = http_nt::ssl_reply;
}} //namespace libgs::https_nt

#endif //LIBGS_ENABLE_OPENSS
#endif //LIBGS_HTTP_NT_CLIENT_REPLY_H