
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

	impl(session_pool_t &pool, request_arg_t arg) :
		m_pool(pool), m_req_arg(std::move(arg)) {}

	impl(impl &&other) noexcept :
		m_pool(other.m_pool),
		m_req_arg(std::move(other.m_req_arg)) {}

	impl& operator=(impl &&other) noexcept
	{
		m_pool = other.m_pool;
		m_req_arg = std::move(other.m_req_arg);
		return *this;
	}

public:
	template <core_concepts::tf_opt_token Token>
	auto write(const const_buffer &body, Token &&token)
	{
		if( m_state == state_t::finished )
			throw runtime_error("libgs::http::client_request: request already sent.");

		using token_t = std::remove_cvref_t<Token>;
		using Body = decltype(body);

		if constexpr( std::is_same_v<token_t, error_code&> )
		{
			m_session = m_impl->m_pool.get({
				m_req_arg.url().address(), m_req_arg.url().port()
			});
			auto &sock = m_session.opt_helper();
			helper_t helper(m_impl->m_req_arg);

			size_t sent = 0;
			if( m_state == state_t::header_sent )
				sent += sock.write(helper.header_data(body.size()), token);

			if( body.size() == 0 )
			{
				m_state = state_t::finished;
				return sent;
			}
			sent += sock.write(helper.body_data(body), token);

		}
		else if constexpr( is_sync_opt_token_v<token_t> )
		{
			error_code error;
			auto reply = write(std::forward<Body>(body), error);
			if( error )
				throw system_error(error, "libgs::http::client_request");
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
	return m_impl->write({}, std::forward<Token>(token));
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
(const headers_t &headers, Token &&token) requires put_or_post
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

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_url(url_t url)
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_header(string_view_t key, value_t value) noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_cookie(string_view_t key, value_t value) noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_chunk_attribute(value_t attribute)
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::set_chunk_attributes(value_list_t attributes)
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::headers_t&
basic_client_request<CharT,Method,SessionPool,Version>::headers() const noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::cookies_t&
basic_client_request<CharT,Method,SessionPool,Version>::cookies() const noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::value_list_t&
basic_client_request<CharT,Method,SessionPool,Version>::chunk_attributes() const noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
const typename basic_client_request<CharT,Method,SessionPool,Version>::url_t&
basic_client_request<CharT,Method,SessionPool,Version>::url() const noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
typename basic_client_request<CharT,Method,SessionPool,Version>::url_t&
basic_client_request<CharT,Method,SessionPool,Version>::url() noexcept
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::unset_header(string_view_t key)
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::unset_cookie(string_view_t key)
{

}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
basic_client_request<CharT,Method,SessionPool,Version>&
basic_client_request<CharT,Method,SessionPool,Version>::unset_chunk_attribute(const value_t &attributes)
{

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
const typename basic_client_request<CharT,Method,SessionPool,Version>::request_arg_t&
basic_client_request<CharT,Method,SessionPool,Version>::request_arg() const noexcept
{
	return m_impl->m_req_arg;
}

template <core_concepts::char_type CharT, method Method, concepts::session_pool SessionPool, version_t Version>
typename basic_client_request<CharT,Method,SessionPool,Version>::request_arg_t&
basic_client_request<CharT,Method,SessionPool,Version>::request_arg() noexcept
{
	return m_impl->m_req_arg;
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