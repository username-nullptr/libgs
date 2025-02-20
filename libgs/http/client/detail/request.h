
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H
#define LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H

#include <libgs/http/client/request_helper.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
class LIBGS_HTTP_TAPI basic_client_request<CharT,Method,SessionPool,Version>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	using helper_t = basic_request_helper<char_t,version_v>;
	using state_t = typename helper_t::state_t;
	using url_t = typename request_arg_t::url_t;

public:
	impl(session_pool_t &pool, request_arg_t arg) :
		m_pool(pool), m_helper(std::move(arg)) {}

	impl(impl &&other) noexcept :
		m_pool(other.m_pool), m_helper(std::move(other.m_helper)) {}

	impl& operator=(impl &&other) noexcept
	{
		m_pool = other.m_pool;
		m_helper = std::move(other.m_helper);
		return *this;
	}

public:
	template <core_concepts::tf_opt_token<error_code,size_t> Token>
	auto write(const const_buffer &body, Token &&token)
	{
		using token_t = std::remove_cvref_t<Token>;
		if constexpr( std::is_same_v<token_t, error_code> )
			return token ? 0 : base_write(body, token);

		else if constexpr( is_sync_opt_token_v<token_t> )
		{
			error_code error;
			auto res = write(body, error);
			if( error )
				throw system_error(error, "libgs::http::client_request::write");
			return res;
		}
#ifdef LIBGS_USING_BOOST_ASIO
		else if constexpr( is_yield_context_v<token_t> )
		{
			// TODO ... ...
		}
#endif //LIBGS_USING_BOOST_ASIO
		else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
		{
			auto ntoken = unbound_redirect_time(token);
			return asio::co_spawn(get_executor(),
			[this, body, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
			{
				error_code error;
				auto var = co_await (
					co_base_write(body, error) or
					sleep_for(get_executor(), timeout)
				);
				auto res = check_time_out(var, error);

				check_error(remove_const(ntoken), error, "libgs::http::client_request::write");
				co_return res;
			},
			ntoken);
		}
		else
		{
			return asio::co_spawn(get_executor(), [this, body, token]() mutable -> awaitable<size_t>
			{
				error_code error;
				auto res = co_await co_base_write(body, error);
				check_error(remove_const(token), error, "libgs::http::client_request::write");
				co_return res;
			},
			token);
		}
	}

	// TODO ... ...

public:
	[[nodiscard]] size_t base_write(std::string &&data, error_code &error)
	{
		size_t sent = 0;
		get_session(error);
		if( error )
			return sent;

		auto &sock = m_session.opt_helper();
		sock.non_blocking(false, error);
		if( error )
			return sent;

		if( state() == state_t::header )
			sent += sock.write(m_helper.header_data<method_v>(data.size()), error);

		if( not error and state() != state_t::finish )
			sent += sock.write(m_helper.body_data(data), error);
		return sent;
	}

	[[nodiscard]] awaitable<size_t> co_base_write(std::string &&data, error_code &error)
	{
		size_t sent = 0;
		co_await co_get_session(error);
		if( error )
			co_return sent;

		using namespace libgs::operators;
		auto &sock = m_session.opt_helper();

		if( state() == state_t::header )
		{
			sent += co_await sock.write (
				m_helper.header_data<method_v>(data.size()),
				use_awaitable | error
			);
		}
		if( not error and state() != state_t::finish )
		{
			sent += co_await sock.write (
				m_helper.body_data(data), use_awaitable | error
			);
		}
		co_return sent;
	}

public:
	[[nodiscard]] size_t chunk_end(const map_helper_t &headers, error_code &error)
	{
		if( state() != state_t::chunk )
			return 0;
		auto buf = m_helper.chunk_end_data(headers);
		if( buf.empty() )
			return 0;
		return base_write(buffer(buf), error);
	}

	[[nodiscard]] awaitable<size_t> co_chunk_end(const map_helper_t &headers, error_code &error)
	{
		if( state() != state_t::chunk )
			co_return 0;
		auto buf = m_helper.chunk_end_data(headers);
		if( buf.empty() )
			co_return 0;
		co_return co_await co_base_write(buffer(buf), error);
	}

public:
	void get_session(error_code &error)
	{
		if( state() == state_t::finish )
			throw runtime_error("libgs::http::client_request: request already sent.");
		else if( m_session )
			return ;
		m_session = m_impl->m_pool.get (
			{url().address(), url().port()}, error
		);
	}

	[[nodiscard]] awaitable<void> co_get_session(std::string &&data, error_code &error)
	{
		if( state() == state_t::finish )
			throw runtime_error("libgs::http::client_request: request already sent.");
		else if( m_session )
			co_return ;

		using namespace libgs::operators;
		m_session = co_await m_impl->m_pool.get (
			{url().address(), url().port()}, use_awaitable | error
		);
		co_return ;
	}

	[[nodiscard]] size_t check_time_out(const auto &var, error_code &error) const
	{
		if( var.index() == 0 )
			return std::get<0>(var);
		else if( not std::get<1>(var) )
			error = make_error_code(errc::timed_out);
		return 0;
	}

public:
	[[nodiscard]] request_arg_t &arg() noexcept {
		return m_helper.arg();
	}
	[[nodiscard]] url_t &url() noexcept {
		return arg().url();
	}
	[[nodiscard]] state_t state() const noexcept {
		return m_helper.state();
	}

public:
	session_pool_t &m_pool;
	session_t m_session;
	helper_t m_helper;
};

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>::basic_client_request(session_pool_t &pool, request_arg_t arg) :
	m_impl(new impl(pool, std::move(arg)))
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>::~basic_client_request()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>::basic_client_request(basic_client_request &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::operator=(basic_client_request &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::write(Token &&token)
{
	return m_impl->write({nullptr,0}, std::forward<Token>(token));
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::write
(const const_buffer &body, Token &&token) requires put_or_post
{
	return m_impl->write(body, std::forward<Token>(token));
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::send_file
(concepts::char_file_opt_token_arg<file_optype::combine, io_permission::read> auto &&opt, Token &&token)
	requires put_or_post
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::chunk_end
(const map_helper_t &headers, Token &&token) requires put_or_post
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <core_concepts::tf_opt_token<error_code,size_t> Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::chunk_end(Token &&token) requires put_or_post
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
template <typename Token>
auto basic_client_request<CharT,Method,SessionPool,Version>::wait_reply(Token &&token) const
	requires core_concepts::tf_opt_token<error_code,reply_t>
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_arg(request_arg_t arg)
{
	m_impl->m_helper.set_arg(std::move(arg));
	return *this;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::request_arg_t&
basic_client_request<CharT,Method,SessionPool,Version>::arg() const noexcept
{
	return m_impl->arg();
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
typename basic_client_request<CharT,Method,SessionPool,Version>::request_arg_t&
basic_client_request<CharT,Method,SessionPool,Version>::arg() noexcept
{
	return m_impl->arg();
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
bool basic_client_request<CharT,Method,SessionPool,Version>::is_finished() const noexcept
{
	return m_impl->m_state == impl::state_t::finished;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::cancel() noexcept
{
	m_impl->m_session.opt_helper().cancel();
	return *this;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::reset() noexcept
{
	cancel();
	m_impl->m_state = impl::state_t::init;
	return *this;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
consteval method_t basic_client_request<CharT,Method,SessionPool,Version>::method() const noexcept
{
	return method_v;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
consteval version_t basic_client_request<CharT,Method,SessionPool,Version>::version() const noexcept
{
	return version_v;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::session_pool_t&
basic_client_request<CharT,Method,SessionPool,Version>::session_pool() const noexcept
{
	return m_impl->m_pool;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
typename basic_client_request<CharT,Method,SessionPool,Version>::session_pool_t&
basic_client_request<CharT,Method,SessionPool,Version>::session_pool() noexcept
{
	return m_impl->m_pool;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
typename basic_client_request<CharT,Method,SessionPool,Version>::executor_t
basic_client_request<CharT,Method,SessionPool,Version>::get_executor() noexcept
{
	return m_impl->m_pool.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_REQUEST_H