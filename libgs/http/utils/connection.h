
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

#ifndef LIBGS_HTTP_UTILS_CONNECTION_H
#define LIBGS_HTTP_UTILS_CONNECTION_H

#include <libgs/http/utils/opt_token.h>
#include <libgs/core/execution.h>
#include <span>

namespace libgs::http
{

struct endpoint
{
	asio::ip::address address {};
	uint16_t port = 0;

	bool from_string(std::string_view text);
	[[nodiscard]] std::string to_string() const;
};

struct tcp_socket_options
{
	optional<bool> no_delay {};
	optional<bool> keep_alive {};
	optional<size_t> send_buffer_size {};
	optional<size_t> receive_buffer_size {};
	optional<asio::socket_base::linger> linger {};
};

struct tcp_socket_state
{
	bool no_delay = false;
	bool keep_alive = false;
	size_t send_buffer_size = 0;
	size_t receive_buffer_size = 0;
	asio::socket_base::linger linger {};
};

enum class connection_probe_state {
	no_event, data_pending, peer_closed, indeterminate
};

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_connection
{
	LIBGS_DISABLE_COPY_MOVE(basic_connection)

public:
	using executor_t = Exec;
	using probe_state_t = connection_probe_state;

	using ptr_t = std::shared_ptr<basic_connection>;

	basic_connection() = default;
	virtual ~basic_connection() = 0;

public:
	template <typename Token, typename...Value>
	static constexpr bool task_token_v =
		core_concepts::tf_opt_token<Token,error_code,Value...> and
		not is_detached_v<std::remove_cvref_t<Token>>;

	template <typename Token = use_sync_t>
	auto read(const mutable_buffer &buf, Token &&token = {})
		noexcept requires task_token_v<Token,size_t>;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(const const_buffer &body, Token &&token = {}) noexcept;

	template <core_concepts::tf_opt_token<error_code,size_t> Token = use_sync_t>
	auto write(std::span<const const_buffer> buffers, Token &&token = {}) noexcept;

	virtual sys_expected<> cancel() noexcept = 0;
	// close() is a non-waiting transport close. In particular, TLS
	// implementations must not wait for the peer's close_notify.
	virtual sys_expected<> close() noexcept = 0;

public:
	virtual sys_expected<> set_options(const tcp_socket_options &options) noexcept = 0;
	[[nodiscard]] virtual sys_expected<tcp_socket_state> options() const noexcept = 0;

	[[nodiscard]] virtual bool is_open() const noexcept = 0;
	// probe() must only inspect immediately available transport state; it must
	// not wait for network input. Unknown/error states are not reusable by a pool.
	[[nodiscard]] virtual sys_expected<probe_state_t> probe() noexcept = 0;

	[[nodiscard]] virtual endpoint remote_endpoint() const noexcept = 0;
	[[nodiscard]] virtual endpoint local_endpoint() const noexcept = 0;

	[[nodiscard]] virtual executor_t get_executor() noexcept = 0;

protected:
	// read() deliberately preserves read_some semantics: one successful stream
	// read completes the operation even when the buffer still has free space.
	[[nodiscard]] virtual io_expected read_some(mutable_buffer buffer) noexcept = 0;
	[[nodiscard]] virtual io_expected write_all(const const_buffer &buffer) noexcept = 0;
	[[nodiscard]] virtual io_expected write_all(std::span<const const_buffer> buffers) noexcept;

	[[nodiscard]] virtual awaitable<io_expected> co_read_some(mutable_buffer buffer) noexcept = 0;
	[[nodiscard]] virtual awaitable<io_expected> co_write_all(const const_buffer &buffer) noexcept = 0;
	[[nodiscard]] virtual awaitable<io_expected> co_write_all(std::span<const const_buffer> buffers) noexcept;
};

using connection = basic_connection<>;
using connection_ptr = basic_connection<>::ptr_t;

} //namespace libgs::http
#include <libgs/http/utils/detail/connection.h>


#endif //LIBGS_HTTP_UTILS_CONNECTION_H
