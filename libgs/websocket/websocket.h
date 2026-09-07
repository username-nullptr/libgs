
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_WEBSOCKET_WEBSOCKET_H
#define LIBGS_WEBSOCKET_WEBSOCKET_H

#include <libgs/core/url.h>
#include <libgs/websocket/global.h>
#include <libgs/http/protocol/utils.h>
#include <libgs/http/utils/connection.h>

namespace libgs
{

template <concepts::exec Exec = asio::any_io_executor>
class LIBGS_WEBSOCKET_TAPI basic_websocket
{
	LIBGS_DISABLE_COPY(basic_websocket)

public:
	using executor_t = Exec;

	template <typename Exec0>
	using connection_t = http::basic_connection<Exec0>;

	template <typename Exec0>
	using connection_ptr = std::shared_ptr<connection_t<Exec0>>;

public:
	basic_websocket() requires
		concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_websocket (
		concepts::match_sched<executor_t> auto &&exec
	);

	basic_websocket(basic_websocket &&other) noexcept;
	basic_websocket &operator=(basic_websocket &&other) noexcept;
	~basic_websocket();

public:
	template <typename Token, typename...Args>
	static constexpr bool dis_detached_token_v =
		concepts::tf_opt_token<Token,Args...> and
		not is_detached_v<token_unbound_t<Token>>;

	template <typename T>
	static constexpr bool is_buffer_v =
		libgs::is_buffer_v<T> and not is_array_buffer_v<T>;

	enum class wait_option {
		writeable, readable, opened, closed, error,
	};
	enum class body_type {
		text, binary
	};

	template <typename Buffer>
	requires is_buffer_v<Buffer>
	struct recv_buf
	{
		using buffer_t = Buffer;
		body_type type {};
		buffer_t body {};
	};

	enum class close_reason
	{

	};

public:
	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto open(const url &url, Token &&token = {});

	template <body_type Type, concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {});

	template <concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {});  // default binary

	template <typename Buffer, typename Token = use_sync_t>
	auto read(Token &&token = {}) requires
		dis_detached_token_v<Token,recv_buf<Buffer>>;

	template <typename Token = use_sync_t>
	auto read(Token &&token = {}) requires
		dis_detached_token_v<Token,std::vector<std::byte>>;

	template <wait_option Option, typename Token = use_sync_t>
	auto wait(Token &&token = {})
		requires dis_detached_token_v<Token,size_t>;

public:
	template <concepts::exec Exec0>
	void adopt(connection_ptr<Exec0> connection, std::string pending_data, role) requires
		concepts::match_sched<Exec0,executor_t>;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using websocket = basic_websocket<>;

} //namespace libgs
#include <libgs/websocket/detail/websocket.h>


#endif //LIBGS_WEBSOCKET_WEBSOCKET_H
