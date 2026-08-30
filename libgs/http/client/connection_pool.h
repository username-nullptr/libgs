
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

#ifndef LIBGS_HTTP_CLIENT_CONNECTION_POOL_H
#define LIBGS_HTTP_CLIENT_CONNECTION_POOL_H

#include <libgs/http/client/connection_lease.h>
#include <libgs/http/client/connector.h>
#include <libgs/core/execution.h>

namespace libgs::http
{

struct connection_pool_config
{
	size_t max_count = std::numeric_limits<size_t>::max();
	using seconds_t = std::chrono::seconds;
	struct {
		seconds_t idle {60}, health {5};
	} timeout;
};

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_connection_pool
{
	LIBGS_DISABLE_COPY(basic_connection_pool)

public:
	using executor_t = Exec;
	using config_t = connection_pool_config;
	using target_t = connect_target;

	using connector_t = basic_connector<executor_t>;
	using connector_ptr = connector_t::ptr_t;

	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

	using lease_t = basic_connection_lease<executor_t>;
	using lease_ptr = lease_t::ptr_t;

public:
	explicit basic_connection_pool(const config_t &config = {}) requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_connection_pool (
		core_concepts::match_sched<executor_t> auto &&exec,
		const config_t &config = {}
	);
	// A configured connector is part of this pool's routing identity. Proxy-aware
	// applications inject one here; the default constructors remain direct-only.
	explicit basic_connection_pool (
		connector_ptr connector_instance, const config_t &config = {}
	);
	~basic_connection_pool();

	basic_connection_pool(basic_connection_pool &&other) noexcept;
	basic_connection_pool &operator=(basic_connection_pool &&other) noexcept;

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto get(const target_t &key, Token &&token = {}) noexcept
		requires concepts::dis_detach_opt_token<Token,error_code,lease_ptr>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto try_get(const target_t &key, Token &&token = {}) noexcept
		requires concepts::dis_detach_opt_token<Token,error_code,lease_ptr>;

public:
	basic_connection_pool &cancel() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

	[[nodiscard]] config_t config() const noexcept;
	[[nodiscard]] size_t count() const noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

using connection_pool = basic_connection_pool<>;

} //namespace libgs::http
#include <libgs/http/client/detail/connection_pool.h>


#endif //LIBGS_HTTP_CLIENT_CONNECTION_POOL_H
