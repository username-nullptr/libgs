
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

#ifndef LIBGS_HTTP_NT_CLIENT_CONNECTION_POOL_H
#define LIBGS_HTTP_NT_CLIENT_CONNECTION_POOL_H

#include <libgs/http_nt/utils/connection.h>

namespace libgs::http_nt
{

template <concepts::stream Stream>
struct LIBGS_HTTP_NT_TAPI default_stream_constructor
{
	using socket_t = Stream;
	[[nodiscard]] static socket_t make(auto &&exec);
};

namespace concepts
{

template <typename Stream, template <typename> class Constructor>
concept connection_pool_template = concepts::stream<Stream> and requires
{
	{
		Constructor<Stream>::make (
			std::declval<typename socket_operation_helper<Stream>::executor_t>()
		)
	}
	-> std::same_as<Stream>;
};

} //namespace concepts

struct connection_pool_config
{
	size_t max_count = std::numeric_limits<size_t>::max();
	using seconds_t = std::chrono::seconds;
	struct {
		seconds_t idle {60}, health {5};
	} timeout;
};

template <typename Stream = asio::ip::tcp::socket,
		  template<typename> class Constructor = default_stream_constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
class LIBGS_HTTP_NT_TAPI basic_connection_pool
{
	LIBGS_DISABLE_COPY(basic_connection_pool)

public:
	using socket_t = Stream;
	using config_t = connection_pool_config;

	using constructor_t = Constructor<socket_t>;
	using connection_t = basic_connection<socket_t>;
	using con_expected_t = sys_expected<connection_t>;

	using opt_helper_t = connection_t::opt_helper_t;
	using executor_t = connection_t::executor_t;
	using endpoint_t = connection_t::endpoint_t;

public:
	basic_connection_pool(config_t config = {}) requires
		core_concepts::match_sched<io_executor_t,executor_t>;

	explicit basic_connection_pool (
		core_concepts::match_sched<executor_t> auto &&exec,
		config_t config = {}
	);
	~basic_connection_pool();

	basic_connection_pool(basic_connection_pool &&other) noexcept;
	basic_connection_pool &operator=(basic_connection_pool &&other) noexcept;

public:
	template <typename Token = use_sync_t>
	[[nodiscard]] auto get(const endpoint_t &ep, Token &&token = {})
		requires core_concepts::tf_opt_token<Token,con_expected_t>;

	template <typename Token = use_sync_t>
	[[nodiscard]] auto try_get(const endpoint_t &ep, Token &&token = {})
		requires core_concepts::tf_opt_token<Token,con_expected_t>;

	bool emplace(socket_t &socket);
	void operator<<(socket_t &socket);

	basic_connection_pool &cancel() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

public:
	[[nodiscard]] config_t config() const noexcept;
	[[nodiscard]] size_t count() const noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl;
};

template <typename Exec>
using basic_tcp_connection_pool = basic_connection_pool <
	asio::basic_stream_socket<asio::ip::tcp,Exec>
>;

using tcp_connection_pool = basic_tcp_connection_pool<asio::any_io_executor>;
using connection_pool = tcp_connection_pool;

template <typename>
struct is_connection_pool : std::false_type {};

template <typename Stream, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Constructor>
struct is_connection_pool<basic_connection_pool<Stream,Constructor>> : std::true_type {};

template <typename T>
constexpr bool is_connection_pool_v = is_connection_pool<T>::value;

namespace concepts
{

template <typename T>
concept connection_pool = is_connection_pool_v<T>;

template <typename T>
concept connection_pool_p = connection_pool<std::remove_cvref_t<T>>;

}} //namespace libgs::http_nt::concepts

#if LIBGS_OPENSSL_SUPPORT
namespace libgs { namespace http_nt
{

template <core_concepts::exec Exec>
struct LIBGS_HTTP_NT_TAPI default_stream_constructor
	<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>
{
	using socket_t = asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>;
	[[nodiscard]] static socket_t make(auto &&exec);
};

template <typename Exec>
using basic_ssl_tcp_connection_pool = basic_connection_pool <
	asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>
>;

using ssl_tcp_connection_pool = basic_ssl_tcp_connection_pool<asio::any_io_executor>;
using ssl_connection_pool = ssl_tcp_connection_pool;

} //namespace http_nt

namespace https_nt
{

template <typename Exec>
using basic_tcp_connection_pool = http_nt::basic_ssl_tcp_connection_pool<Exec>;

using tcp_connection_pool = http_nt::ssl_tcp_connection_pool;
using connection_pool = http_nt::ssl_connection_pool;

}} //namespace libgs::https_nt

#endif //LIBGS_OPENSSL_SUPPORT
#include <libgs/http_nt/client/detail/connection_pool.h>

#endif //LIBGS_HTTP_NT_CLIENT_CONNECTION_POOL_H