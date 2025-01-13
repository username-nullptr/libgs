
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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H

#include <libgs/http/client/request_helper.h>
#include <spdlog/common.h>

namespace libgs::http
{

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
class LIBGS_HTTP_TAPI basic_client<CharT,SessionPool>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	using string_t = std::basic_string<char_t>;
	using helper_t = basic_request_helper<char_t>;
	using string_pool_t = detail::string_pool<char_t>;

public:
	template <core_concepts::match_execution<executor_t> Exec0>
	explicit impl(const Exec0 &exec, string_view_t version = string_pool_t::v_1_1) :
		m_version(version), m_session_pool(exec) {}

	explicit impl(string_view_t version) : m_version(version) {}
	impl() = default;

	impl(impl &&other) noexcept = default;
	impl &operator=(impl &&other) noexcept = default;

public:
	string_t m_version = string_pool_t::v_1_1;
	session_pool_t m_session_pool;
	// TODO: Save cookie ... ...
};

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client(const core_concepts::match_execution<executor_t> auto &exec) :
	m_impl(new impl(exec))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client(core_concepts::match_execution_context<executor_t> auto &context) :
	m_impl(new impl(context.get_executor()))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client() requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl())
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client
(string_view_t version, const core_concepts::match_execution<executor_t> auto &exec) :
	m_impl(new impl(exec, version))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client
(string_view_t version, core_concepts::match_execution_context<executor_t> auto &context) :
	m_impl(new impl(context.get_executor(), version))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client(string_view_t version) requires
	core_concepts::match_default_execution<executor_t> :
	m_impl(new impl(version))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::~basic_client()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool>::basic_client(basic_client &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
basic_client<CharT,SessionPool> &basic_client<CharT,SessionPool>::operator=(basic_client &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::get(request_t request, Token &&token)
{
	return this->request<method_t::GET>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::put(request_t request, concepts::get_body auto &&body, Token &&token)
{
	return this->request<method_t::PUT> (
		std::move(request), std::forward<decltype(body)>(body), std::forward<Token>(token)
	);
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::put(request_t request, Token &&token)
{
	return this->request<method_t::PUT>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::post(request_t request, concepts::get_body auto &&body, Token &&token)
{
	return this->request<method_t::POST> (
		std::move(request), std::forward<decltype(body)>(body), std::forward<Token>(token)
	);
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::post(request_t request, Token &&token)
{
	return this->request<method_t::POST>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::Delete(request_t request, Token &&token)
{
	return this->request<method_t::DELETE>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::head(request_t request, Token &&token)
{
	return this->request<method_t::HEAD>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::options(request_t request, Token &&token)
{
	return this->request<method_t::OPTIONS>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::trace(request_t request, Token &&token)
{
	return this->request<method_t::TRACE>(std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <method_t Method, core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::request(request_t request, concepts::get_body auto &&body, Token &&token)
   	requires (Method == method_t::POST || Method == method_t::PUT)
{
	return this->request(Method,
		std::move(request), std::forward<decltype(body)>(body), std::forward<Token>(token)
	);
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <method_t Method, core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::request(request_t request, Token &&token)
{
	return this->request(Method, std::move(request), std::forward<Token>(token));
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::request
(method_t method, request_t request, concepts::get_body auto &&body, Token &&token)
{
	if( method != method_t::POST and method != method_t::PUT )
		throw std::invalid_argument("libgs::http::client::request: Only POST and PUT method can have body.");

	typename impl::helper_t helper(version(), request);
	using token_t = std::remove_cvref_t<Token>;
	using Body = decltype(body);

	if constexpr( std::is_same_v<token_t, error_code&> )
	{
		// auto session = m_impl->m_session_pool.get({request.url().address(), request.url().port()});
		// helper.header_data();
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto reply = request(method, std::move(helper), std::forward<Body>(body), error);
		if( error )
			throw system_error(error, "libgs::http::client::request");
		return reply;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{

	}
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
template <core_concepts::tf_opt_token Token>
auto basic_client<CharT,SessionPool>::request
(method_t method, request_t request, Token &&token)
{
	// TODO ... ...
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
typename basic_client<CharT,SessionPool>::string_view_t
basic_client<CharT,SessionPool>::version() const noexcept
{
	return m_impl->m_version;
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
const typename basic_client<CharT,SessionPool>::session_pool_t&
basic_client<CharT,SessionPool>::session_pool() const noexcept
{
	return m_impl->m_session_pool;
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
typename basic_client<CharT,SessionPool>::session_pool_t&
basic_client<CharT,SessionPool>::session_pool() noexcept
{
	return m_impl->m_session_pool;
}

template <core_concepts::char_type CharT, concepts::session_pool SessionPool>
typename basic_client<CharT,SessionPool>::executor_t
basic_client<CharT,SessionPool>::get_executor() noexcept
{
	return session_pool().get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
