// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_CLIENT_CONNECTION_LEASE_H
#define LIBGS_HTTP_CLIENT_CONNECTION_LEASE_H

#include <libgs/http/utils/connection.h>

namespace libgs::http
{

template <core_concepts::exec Exec = asio::any_io_executor>
class LIBGS_HTTP_TAPI basic_connection_lease
{
	LIBGS_DISABLE_COPY_MOVE(basic_connection_lease)

public:
	using executor_t = Exec;
	using ptr_t = std::shared_ptr<basic_connection_lease>;

	using connection_t = basic_connection<executor_t>;
	using connection_ptr = connection_t::ptr_t;

	basic_connection_lease (
		connection_ptr conn,
		std::function<void(connection_ptr)> give_back
	);
	~basic_connection_lease(); // close

public:
	[[nodiscard]] connection_t &get() noexcept;
	[[nodiscard]] connection_t &operator*() noexcept;
	[[nodiscard]] connection_t *operator->() noexcept;

	[[nodiscard]] const connection_t &get() const noexcept;
	[[nodiscard]] const connection_t &operator*() const noexcept;
	[[nodiscard]] const connection_t *operator->() const noexcept;

	// Detach the connection from the pool. The caller becomes responsible for it.
	[[nodiscard]] connection_ptr take() noexcept;

	// Explicitly offer the connection back to the pool. Destruction without
	// release closes it instead of making an unverified implicit reuse decision.
	void release();

	[[nodiscard]] bool is_valid() const noexcept;
	[[nodiscard]] operator bool() const noexcept;

private:
	class impl;
	impl *m_impl;
};

using connection_lease = basic_connection_lease<>;

} //namespace libgs::http
#include <libgs/http/client/detail/connection_lease.h>


#endif //LIBGS_HTTP_CLIENT_CONNECTION_LEASE_H
