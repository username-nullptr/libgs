
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H
#define LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H

#include <libgs/coro/utils.h>
#include <map>
#include <set>

namespace libgs::http
{

template <concepts::stream Stream>
default_stream_constructor<Stream>::socket_t
default_stream_constructor<Stream>::make(auto &&exec)
{
	return socket_t(get_executor_helper (
		std::forward<decltype(exec)>(exec))
	);
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
class LIBGS_HTTP_TAPI basic_connection_pool<Stream,Exec,Constructor>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using opt_helper_t = connection_t::opt_helper_t;

public:
	explicit impl(const auto &exec) : m_exec(exec) {}
	impl() : m_exec(libgs::get_executor()) {}

	~impl() {
		*m_valid = false;
	}

public:
	[[nodiscard]] sys_expected<connection_t> get(const endpoint_t &ep, auto &&exec) noexcept
	{
		auto session = _get(ep, std::forward<decltype(exec)>(exec));
		auto &sock_helper = session.opt_helper();

		if( sock_helper.is_open() )
			return std::move(session);

		std::error_code error;
		sock_helper.connect(ep, error);

		if( error )
		{
			m_sock_map.erase(ep);
			return sys_unexpected(error);
		}
		return std::move(session);
	}

	[[nodiscard]] awaitable<sys_expected<connection_t>> co_get(const endpoint_t &ep, auto &&exec,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto session = _get(ep, std::forward<decltype(exec)>(exec));
		auto &sock_helper = session.opt_helper();

		if( sock_helper.is_open() )
			co_return std::move(session);
		m_curr_tasks.emplace(&sock_helper);

		using namespace libgs::operators;
		using namespace std::chrono_literals;

		std::error_code error;
		auto task = sock_helper.connect(ep, use_awaitable | cancel_slot | error);

		if( timeout == 0ns )
		{
			co_await std::move(task);
			m_curr_tasks.erase(&sock_helper);
			if( error )
			{
				m_sock_map.erase(ep);
				co_return sys_unexpected(error);
			}
			co_return std::move(session);
		}
		auto var = co_await(std::move(task) or
			coro::sleep_for(sock_helper.get_executor(), timeout)
		);
		m_curr_tasks.erase(&sock_helper);

		if( var.index() == 0 )
			co_return std::move(session);

		else if( not std::get<1>(var) )
			co_return sys_unexpected(make_error_code(errc::timed_out));

		co_return sys_unexpected(std::get<1>(var));
	}

	[[nodiscard]] sys_expected<connection_t> co_get(std::error_code &error, const endpoint_t &ep, auto &&exec,
		asio::cancellation_slot cancel_slot, std::chrono::nanoseconds timeout) noexcept
	{
		auto expected = co_await co_get(std::forward<decltype(exec)>(exec),
			std::move(cancel_slot), std::move(timeout)
		);
		if( not expected )
			error = expected.error();
		co_return expected;
	}

	void emplace(socket_t &&socket)
	{
		opt_helper_t opt(socket);
		if( opt.is_open() )
		{
			m_sock_map.emplace(std::make_pair (
				opt.remote_endpoint(), std::move(socket)
			));
		}
	}

private:
	[[nodiscard]] connection_t _get(const endpoint_t &ep, auto &&exec) noexcept
	{
		auto it = m_sock_map.find(ep);
		if( it == m_sock_map.end() )
		{
			auto socket = constructor_t::make (
				std::forward<decltype(exec)>(exec)
			);
			return make(std::move(socket));
		}
		socket_t socket(std::move(it->second));
		m_sock_map.erase(it);

		std::error_code error;
		asio::socket_base::receive_buffer_size op;

		opt_helper_t(socket).get_option(op, error);
		if( error )
		{
			socket = constructor_t::make (
				std::forward<decltype(exec)>(exec)
			);
		}
		return make(std::move(socket));
	}

	[[nodiscard]] connection_t make(socket_t &&socket) noexcept
	{
		return connection_t(std::move(socket), [this, valid = m_valid](socket_t &&sock) mutable
		{
			if( not opt_helper_t(sock).is_open() )
				return ;
			dispatch(m_exec, [this, valid = std::move(valid), sock = std::move(sock)]() mutable
			{
				if( *valid )
					emplace(std::move(sock));
			});
		});
	}

public:
	std::shared_ptr<bool> m_valid {new bool(true)};
	std::map<endpoint_t,socket_t> m_sock_map;

	std::set<const opt_helper_t*> m_curr_tasks {};
	executor_t m_exec;
};

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>::basic_connection_pool() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(new impl())
{

}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>::basic_connection_pool(core_concepts::match_sched<Exec> auto &&exec) :
	m_impl(new impl(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>::~basic_connection_pool()
{
	delete m_impl;
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>::basic_connection_pool(basic_connection_pool &&other) noexcept :
	m_impl(other.m_impl)
{
	other.m_impl = new impl();
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>&
basic_connection_pool<Stream,Exec,Constructor>::operator=(basic_connection_pool &&other) noexcept
{
	if( this == &other )
		return *this;
	delete m_impl;
	m_impl = other.m_impl;
	other.m_impl = new impl();
	return *this;
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Exec,Constructor>::get(const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,connection_t>
{
	return get(m_impl->m_exec, ep, std::forward<Token>(token));
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
template <typename Token>
auto basic_connection_pool<Stream,Exec,Constructor>::get
(core_concepts::match_sched<socket_executor_t> auto &&exec, const endpoint_t &ep, Token &&token)
	requires core_concepts::tf_opt_token<Token,error_code,connection_t>
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( is_error_code_token_v<Token> )
	{
		return m_impl->get(ep, std::forward<decltype(exec)>(exec))
			.or_else([&token](const error_code &error) {
				token = error;
			});
	}
	else if constexpr( is_sync_opt_token_v<Token> )
		return m_impl->get(ep, std::forward<decltype(exec)>(exec));

	else if constexpr( is_redirect_time_v<token_t> )
	{
		decltype(auto) ntoken = unbound_redirect_time(token);
		using ntoken_t = std::remove_cvref_t<decltype(ntoken)>;

		decltype(auto) nntoken = unbound_token(ntoken);
		using nntoken_t = std::remove_cvref_t<decltype(nntoken)>;

		if constexpr( is_use_awaitable_v<nntoken_t> or is_deferred_v<nntoken_t> )
		{
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				return m_impl->co_get(ntoken.ec_, ep, std::forward<decltype(exec)>(exec),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
			else
			{
				return m_impl->co_get(ep, std::forward<decltype(exec)>(exec),
					asio::get_associated_cancellation_slot(nntoken),
					get_associated_redirect_time(token)
				);
			}
		}
		else if constexpr( is_use_future_v<nntoken_t> )
		{
			auto promise = std::make_shared<std::promise<io_expected>>();
			if constexpr( is_redirect_error_v<ntoken_t> )
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(), ntoken, ep,
					exec = get_executor_helper(std::forward<decltype(exec)>(exec)), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						ntoken.ec_, ep, exec, cancel_slot, timeout
					));
					co_return ;
				});
			}
			else
			{
				libgs::dispatch(m_impl->m_session.get_executor(), [impl = m_impl->shared_from_this(), ep,
					exec = get_executor_helper(std::forward<decltype(exec)>(exec)), promise = std::move(promise),
					cancel_slot = asio::get_associated_cancellation_slot(nntoken),
					timeout = get_associated_redirect_time(token)
				]() mutable -> awaitable<void>
				{
					promise->set_value(co_await co_get (
						ep, exec, cancel_slot, timeout
					));
					co_return ;
				});
			}
			return promise->get_future();
		}
		else if constexpr( is_redirect_error_v<ntoken_t> )
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), ntoken, nntoken, ep,
				exec = get_executor_helper(std::forward<decltype(exec)>(exec)),
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					ntoken.ec_, ep, exec, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
		else
		{
			libgs::dispatch(m_impl->m_session.get_executor(), [
				impl = m_impl->shared_from_this(), nntoken, ep,
				exec = get_executor_helper(std::forward<decltype(exec)>(exec)),
				timeout = get_associated_redirect_time(token),
				cancel_slot = asio::get_associated_cancellation_slot(ntoken)
			]() mutable -> awaitable<void>
			{
				auto expected = co_await impl->co_get (
					ep, exec, cancel_slot, timeout
				);
				expected
				.transform([&callback = nntoken](int code) {
					callback(error_code(), code);
				})
				.or_else([&callback = nntoken](const error_code &error) {
					callback(error, 255);
				});
			});
		}
	}
	else
	{
		using namespace libgs::operators;
		using namespace std::chrono_literals;
		return get(std::forward<decltype(exec)>(exec), ep, token | 0ns);
	}
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>&
basic_connection_pool<Stream,Exec,Constructor>::emplace(socket_t &&socket)
{
	m_impl->emplace(std::move(socket));
	return *this;
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
void basic_connection_pool<Stream,Exec,Constructor>::operator<<(socket_t &&socket)
{
	emplace(std::move(socket));
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>&
basic_connection_pool<Stream,Exec,Constructor>::cancel() noexcept
{
	for(auto &task : m_impl->m_curr_tasks)
		task->cancel();
	return *this;
}

template <typename Stream, typename Exec, template<typename> class Constructor>
	requires concepts::connection_pool_template<Stream,Exec,Constructor>
basic_connection_pool<Stream,Exec,Constructor>::executor_t
basic_connection_pool<Stream,Exec,Constructor>::get_executor() noexcept
{
	return m_impl->m_exec;
}

} //namespace libgs::http

#if LIBGS_OPENSSL_SUPPORT
namespace libgs::http { namespace detail
{

[[nodiscard]] LIBGS_HTTP_API
asio::ssl::context &default_ssl_context() noexcept;

} //namespace detail

template <core_concepts::exec Exec>
default_stream_constructor<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::socket_t
default_stream_constructor<asio::ssl::stream<asio::basic_stream_socket<asio::ip::tcp,Exec>>>::make(auto &&exec)
{
	using next_layer_t = socket_t::next_layer_type;
	using next_layer_constructor_t = default_stream_constructor<next_layer_t>;

	auto next_layer = next_layer_constructor_t::make (
		std::forward<decltype(exec)>(exec)
	);
	return socket_t(std::move(next_layer),
		detail::default_ssl_context()
	);
}

} //namespace libgs::http

#endif //LIBGS_OPENSSL_SUPPORT
#endif //LIBGS_HTTP_CLIENT_DETAIL_CONNECTION_POOL_H