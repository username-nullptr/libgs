
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
		connection_ptr connection,
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
