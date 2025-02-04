
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

#include <libgs/http/cxx/socket_operation_helper.h>
#include <libgs/http/server/request_parser.h>

namespace libgs::http
{

template <concepts::stream Stream, core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_server_request
{
	LIBGS_DISABLE_COPY(basic_server_request)

public:
	using next_layer_t = Stream;
	using executor_t = typename next_layer_t::executor_type;
	using endpoint_t = typename socket_operation_helper<next_layer_t>::endpoint_t;

	using char_t = CharT;
	using parser_t = basic_request_parser<char_t>;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	using value_t = typename parser_t::value_t;
	using path_args_t = typename parser_t::path_args_t;

	using header_t = typename parser_t::header_t;
	using headers_t = typename parser_t::headers_t;
	using cookies_t = typename parser_t::cookies_t;
	using parameters_t = typename parser_t::parameters_t;

public:
	template <typename NextLayer>
	basic_server_request(NextLayer &&next_layer, parser_t &parser)
		requires core_concepts::constructible<next_layer_t,NextLayer&&>;
	~basic_server_request();

	basic_server_request(basic_server_request &&other) noexcept;
	basic_server_request &operator=(basic_server_request &&other) noexcept;

	template <typename Stream0>
	basic_server_request(basic_server_request<Stream0,char_t> &&other) noexcept
		requires core_concepts::constructible<Stream,Stream0&&>;

	template <typename Stream0>
	basic_server_request &operator=(basic_server_request<Stream0,char_t> &&other) noexcept
		requires core_concepts::assignable<Stream,Stream0&&>;

public:
	[[nodiscard]] method_t method() const noexcept;
	[[nodiscard]] version_t version() const noexcept;
	[[nodiscard]] string_view_t path() const noexcept;

	[[nodiscard]] const parameters_t &parameters() const noexcept;
	[[nodiscard]] const path_args_t &path_args() const noexcept;
	[[nodiscard]] const headers_t &headers() const noexcept;
	[[nodiscard]] const cookies_t &cookies() const noexcept;

public:
	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) parameter(core_concepts::basic_string_type<char_t> auto &&key) const;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) header(core_concepts::basic_string_type<char_t> auto &&key) const;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) cookie(core_concepts::basic_string_type<char_t> auto &&key) const;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) parameter_or (
		core_concepts::basic_string_type<char_t> auto &&key, T &&def_value = T()
	) const noexcept;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) header_or (
		core_concepts::basic_string_type<char_t> auto &&key, T &&def_value = T()
	) const noexcept;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) cookie_or (
		core_concepts::basic_string_type<char_t> auto &&key, T &&def_value = T()
	) const noexcept;

public:
	int32_t path_match(string_view_t rule);

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) path_arg(core_concepts::basic_string_type<char_t> auto &&key) const;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) path_arg(size_t index) const;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) path_arg_or (
		core_concepts::basic_string_type<char_t> auto &&key, T &&def_value = T()
	) const noexcept;

	template <core_concepts::basic_text_arg<CharT> T = value_t>
	[[nodiscard]] decltype(auto) path_arg_or (
		size_t index, T &&def_value = T()
	) const noexcept;

public:
	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto read(Token &&token = {});

	template <core_concepts::dis_func_tf_opt_token Token = use_sync_t>
	auto save_file (
		concepts::char_file_opt_token_arg<file_optype::single, io_permission::write> auto &&opt,
		Token &&token = {}
	);

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
	basic_server_request &cancel() noexcept;

public:
	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

private:
	class impl;
	impl *m_impl;
};

template <core_concepts::execution Exec>
using basic_tcp_server_request = basic_server_request<asio::basic_stream_socket<asio::ip::tcp,Exec>,char>;

template <core_concepts::execution Exec>
using wbasic_tcp_server_request = basic_server_request<asio::basic_stream_socket<asio::ip::tcp,Exec>,wchar_t>;

using tcp_server_request = basic_tcp_server_request<asio::any_io_executor>;
using wtcp_server_request = wbasic_tcp_server_request<asio::any_io_executor>;

using server_request = tcp_server_request;
using wserver_request = wtcp_server_request;

} //namespace libgs::http
#include <libgs/http/server/detail/request.h>


#endif //LIBGS_HTTP_SERVER_REQUEST_H
