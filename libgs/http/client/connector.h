
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

#ifndef LIBGS_HTTP_CLIENT_CONNECTOR_H
#define LIBGS_HTTP_CLIENT_CONNECTOR_H

#include <libgs/http/utils/connection.h>
#include <libgs/http/cxx/concepts.h>

namespace libgs::http
{

enum class security_mode {
	plain, tls, // ... ...
};

struct connect_target
{
	std::string host {};
	uint16_t port = 0;
	security_mode security {};

	friend bool operator== (
		const connect_target&, const connect_target&
	) = default;
};

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_connector
{
	LIBGS_DISABLE_COPY_MOVE(basic_connector)

public:
	using executor_t = Exec;
	using ptr_t = std::shared_ptr<basic_connector>;

	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

public:
	explicit basic_connector (
		core_concepts::match_sched<executor_t> auto &&exec
	);
#if LIBGS_OPENSSL_SUPPORT
	basic_connector (
		core_concepts::match_sched<executor_t> auto &&exec,
		asio::ssl::context &tls_context
	);
#endif //LIBGS_OPENSSL_SUPPORT
	virtual ~basic_connector();

	template <typename Token = use_sync_t>
	auto connect(const connect_target &target, Token &&token = {}) noexcept
		requires concepts::dis_detach_opt_token<Token,error_code,connection_ptr>;

	[[nodiscard]] executor_t get_executor() const noexcept;

protected:
	// The default implementation connects directly. Proxy support is provided by
	// deriving a configured connector; callers that do not configure one never
	// need to model proxy routes in their connect_target.
	[[nodiscard]] virtual sys_expected<connection_ptr>
	do_connect(const connect_target &target) noexcept;

	[[nodiscard]] virtual awaitable<sys_expected<connection_ptr>>
	co_do_connect(const connect_target &target) noexcept;

private:
	class impl;
	impl *m_impl;
};

using connector = basic_connector<>;

} //namespace libgs::http
#include <libgs/http/client/detail/connector.h>


#endif //LIBGS_HTTP_CLIENT_CONNECTOR_H
