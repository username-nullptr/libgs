
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

#include <libgs/http/protocol/utils/server/generator.h>
#include <libgs/http/server/request.h>

namespace libgs::http
{

template <concepts::stream Stream>
class LIBGS_HTTP_VAPI basic_response
{
	LIBGS_DISABLE_COPY(basic_response)

public:
	using next_layer_t = basic_server_request<Stream>;
	using executor_t = typename next_layer_t::executor_t;

	using helper_t = protocol::server_generator;
	using value_t = typename next_layer_t::value_t;
	using headers_t = typename next_layer_t::headers_t;

	using cookie_t = protocol::cookie;
	using cookies_t = protocol::cookies;


public:
	explicit basic_response(next_layer_t &&next_layer);
	~basic_response();

	basic_response(basic_response &&other) noexcept;
	basic_response &operator=(basic_response &&other) noexcept;

	template <typename Stream0>
	basic_response(basic_response<Stream0> &&other) noexcept
		requires core_concepts::constructible<next_layer_t,basic_server_request<Stream0>&&>;

	template <typename Stream0>
	basic_response &operator=(basic_response<Stream0> &&other) noexcept
		requires core_concepts::assignable<Stream,Stream0&&>;

public:
	[[nodiscard]] std::string_view version() const noexcept;
	basic_response &set_status(protocol::status_enum status);
	[[nodiscard]] protocol::status_enum status() const noexcept;

public:
	basic_response &set_header (
		core_concepts::text_p<char> auto &&key, value_t value
	) noexcept;

	basic_response &unset_header (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] headers_t &headers() noexcept;

public:
	basic_response &set_cookie (
		core_concepts::text_p<char> auto &&key, cookie_t cookie
	) noexcept;

	 basic_response &unset_cookie (
		const core_concepts::text_p<char> auto &key
	) noexcept;

	[[nodiscard]] const cookies_t &cookies() const noexcept;
	[[nodiscard]] cookies_t &cookies() noexcept;

public:
	basic_response &set_chunk_attribute(value_t attr) noexcept;
	basic_response &unset_chunk_attribute(const value_t &attr) noexcept;

	[[nodiscard]] const std::set<value_t> &chunk_attributes() const noexcept;
	[[nodiscard]] std::set<value_t> &chunk_attributes() noexcept;

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto write(Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto redirect(core_concepts::text_p<char> auto &&url,
		protocol::redirect_enum redi, Token &&token = {}
	);

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto redirect(core_concepts::text_p<char> auto &&url,
		Token &&token = {}
	);

public:
	template <typename T>
	static constexpr bool file_opt_token = concepts::file_opt_token_p <
		T, char, file_optype::combine, io_permission::read
	>;
	template <typename T, core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto send_file(T &&opt, Token &&token = {}) requires file_opt_token<T>;

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto chunk_end(const headers_t &headers, Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto chunk_end(Token &&token = {});

public:
	[[nodiscard]] bool is_finished() const noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;
	basic_response &cancel() noexcept;

public:
	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::exec Exec>
using basic_tcp_server_response =
	basic_response<asio::basic_stream_socket<asio::ip::tcp,Exec>>;

using tcp_server_response = basic_tcp_server_response<asio::any_io_executor>;
using server_response = tcp_server_response;

} //namespace libgs::http
#include <libgs/http/server/detail/response.h>


#endif //LIBGS_HTTP_SERVER_RESPONSE_H
