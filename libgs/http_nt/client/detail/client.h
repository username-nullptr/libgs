
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

#ifndef LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H

namespace libgs::http_nt { namespace detail
{

template <concepts::stream>
struct protocol_name {
	static constexpr auto name = "http";
};

#if LIBGS_OPENSSL_SUPPORT
template <core_concepts::exec Exec>
struct protocol_name
<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>> {
	static constexpr auto name = "https";
};
#endif //LIBGS_OPENSSL_SUPPORT

template <concepts::stream Stream>
constexpr auto protocol_name_v = protocol_name<Stream>::name;

} //namespace detail

template <concepts::connection_pool ConnectionPool, version_enum Version>
class LIBGS_HTTP_NT_TAPI basic_client<ConnectionPool,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using socket_t = connection_t::socket_t;

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_pool(io_context()) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec) :
		m_pool(exec) {}

	explicit impl(connection_pool_t &&pool) :
		m_pool(std::move(pool)) {}

public:
	template <method_enum Method>
	[[nodiscard]] ctx_expected_t<Method> make_context(req_info info) noexcept
	{
		if( strtls::to_lower(info.url.protocol()) != detail::protocol_name_v<socket_t> )
		{
			return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = connection_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		endpoint_t ep;
		std::error_code error;

		auto addr = asio::ip::make_address(info.url.address().data(), error);
		if( not error )
			ep = {addr, info.url.port()};
		else
		{
			using resolver_t = asio::ip::basic_resolver<protocol_t>;
			resolver_t resolver(m_pool.get_executor());

			auto results = resolver.resolve(info.url.address(), "", error);
			if( error )
				return sys_unexpected(error);

			else if( results.empty() )
			{
				return sys_unexpected (
					make_error_code(std::errc::host_unreachable)
				);
			}
			ep = {results.begin()->endpoint().address(), info.url.port()};
		}
		auto expected = m_pool.get(ep);
		if( not expected )
			return sys_unexpected(expected.error());

		return std::make_shared<context_t<Method>>(request_t<Method>(
			std::move(*expected), std::move(info.url), std::move(info.arg)
		));
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_make_context(req_info info,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		if( strtls::to_lower(info.url.protocol()) != detail::protocol_name_v<socket_t> )
		{
			co_return sys_unexpected (
				make_error_code(std::errc::protocol_error)
			);
		}
		using protocol_t = connection_t::opt_helper_t::protocol_t;
		using endpoint_t = asio::ip::basic_endpoint<protocol_t>;

		using namespace libgs::operators;
		auto task = libgs::dispatch(m_pool.get_executor(),
		[&]() mutable -> awaitable<ctx_expected_t<Method>>
		{
			endpoint_t ep;
			std::error_code error;

			auto addr = asio::ip::make_address(info.url.address().data(), error);
			if( not error )
				ep = {addr, info.url.port()};
			else
			{
				using resolver_t = asio::ip::basic_resolver<protocol_t>;
				resolver_t resolver(m_pool.get_executor());

				auto results = co_await resolver.async_resolve (
					info.url.address(), "", use_awaitable | cancel_slot | error
				);
				if( error )
					co_return sys_unexpected(error);

				else if( results.empty() )
				{
					co_return sys_unexpected (
						make_error_code(std::errc::host_unreachable)
					);
				}
				ep = {results.begin()->endpoint().address(), info.url.port()};
			}
			auto expected = co_await m_pool.get(ep, use_awaitable | cancel_slot);
			if( not expected )
				co_return sys_unexpected(expected.error());

			co_return std::make_shared<context_t<Method>>(request_t<Method>(
				std::move(*expected), std::move(info.url), std::move(info.arg)
			));
		},
		use_awaitable);

		using namespace std::chrono_literals;
		ctx_expected_t<Method> expected;

		if( timeout == 0ns )
			expected = co_await std::move(task);
		else
		{
			auto var = co_await(std::move(task) or
				coro::sleep_for(m_pool.get_executor(), timeout)
			);
			if( var.index() == 0 )
				expected = std::get<0>(var);
			else if( not std::get<1>(var) )
				expected.despair(make_error_code(errc::timed_out));
			else
				expected.despair(std::get<1>(var));
		}
		co_return expected;
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<ctx_expected_t<Method>> co_make_context(std::error_code &error,
		req_info info, asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_make_context<Method>(std::move(info),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

public:
	connection_pool_t m_pool;
};

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(connection_pool_t &&pool) :
	m_impl(std::make_shared<impl>(std::move(pool)))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::basic_client(basic_client &&other) noexcept :
	m_impl(std::make_shared<impl>(std::move(*other.m_impl)))
{

}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>&
basic_client<ConnectionPool,Version>::operator=(basic_client &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::connection_pool ConnectionPool, version_enum Version>
basic_client<ConnectionPool,Version>::~basic_client() = default;

template <concepts::connection_pool ConnectionPool, version_enum Version>
template <method_enum Method, typename Token>
auto basic_client<ConnectionPool,Version>::request(req_info info, Token &&token)
	noexcept requires request_token_v<Method,Token>
{

}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_CLIENT_DETAIL_CLIENT_H