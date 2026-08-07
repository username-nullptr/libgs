
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

#ifndef LIBGS_HTTP_NT_SERVER_RESPONSE_H
#define LIBGS_HTTP_NT_SERVER_RESPONSE_H

#include <libgs/http_nt/protocol/utils/core/container_helper.h>
#include <libgs/http_nt/utils/connection.h>

namespace libgs::http_nt
{

template <concepts::connection Connection>
class basic_request;

template <concepts::connection Connection>
class LIBGS_HTTP_NT_TAPI basic_response :
	public mutable_headers<basic_response<Connection>>,
	public mutable_cookies<cookie,basic_response<Connection>>,
	public mutable_chunk_attributes<basic_response<Connection>>
{
	LIBGS_DISABLE_COPY_MOVE(basic_response)

public:
	using connection_t = Connection;
	using connection_ptr = std::shared_ptr<connection_t>;
	using executor_t = connection_t::executor_t;

	using request_t = basic_request<connection_t>;
	using socket_t = connection_t::socket_t;
	using endpoint_t = socket_t::endpoint_t;

	using value_t = libgs::value;
	using headers_t = http_nt::headers;

public:
	explicit basic_response(connection_ptr connection);
	~basic_response() override;

public:
	[[nodiscard]] std::string_view version() const noexcept;
	basic_response &set_status(status_enum status);
	basic_response &auto_set(request_t &request);

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {}) noexcept;

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto write(Token &&token = {}) noexcept;

	template <typename T>
	static constexpr bool file_opt_token = concepts::file_opt_token_p <
		T, char, file_optype::combine, io_permission::read
	>;
	template <typename T, core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto send_file(T &&opt, Token &&token = {}) requires file_opt_token<T>;

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto redirect(core_concepts::text_p<char> auto &&url,
		redirect_enum redi, Token &&token = {}
	);

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto redirect(core_concepts::text_p<char> auto &&url,
		Token &&token = {}
	);

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto continues(Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto chunk_end(const headers_t &headers, Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto chunk_end(Token &&token = {});

public:
	[[nodiscard]] status_enum status() const noexcept;
	[[nodiscard]] bool is_finished() const noexcept;

	[[nodiscard]] executor_t get_executor() noexcept;
	basic_response &cancel() noexcept;

private:
	class impl;
	impl *m_impl;
};

using response = basic_response<connection>;

} //namespace libgs::http_nt
#include <libgs/http_nt/server/detail/response.h>


#endif //LIBGS_HTTP_NT_SERVER_RESPONSE_H
