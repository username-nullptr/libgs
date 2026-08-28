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

#ifndef LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
#define LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H

#include <libgs/http/utils/detail/async_expected.h>

namespace libgs::http
{

template <core_concepts::exec Exec, version_enum Version>
class LIBGS_HTTP_TAPI basic_client<Exec,Version>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	template <method_enum Method>
	using result_t = sys_expected<context_ptr<Method>>;
	using target_t = connection_pool_t::target_t;

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_pool(io_context()), m_cookie_store(std::make_shared<cookie_jar>()) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec) :
		m_pool(exec), m_cookie_store(std::make_shared<cookie_jar>()) {}

	explicit impl(connection_pool_t &&pool) :
		m_pool(std::move(pool)), m_cookie_store(std::make_shared<cookie_jar>()) {}

private:
	[[nodiscard]] static sys_expected<target_t> target_from_url(const url_t &value) noexcept
	{
		// url is a generic hierarchical resource descriptor. HTTP protocol
		// selection is deliberately enforced only at the client boundary.
		try {
			if( not value.is_valid() )
			{
				return sys_unexpected (
					make_error_code(std::errc::invalid_argument)
				);
			}
			auto scheme = strtls::to_lower(value.protocol());
			security_mode security {};
			auto port = value.port();

			if( scheme == "http" )
			{
				security = security_mode::plain;
				if( port == 0 )
					port = 80;
			}
			else if( scheme == "https" )
			{
				security = security_mode::tls;
				if( port == 0 )
					port = 443;
			}
			else
			{
				return sys_unexpected (
					make_error_code(std::errc::protocol_not_supported)
				);
			}
			return target_t {
				strtls::to_lower(value.host()), port, security
			};
		}
		catch(const std::bad_alloc&)
		{
			return sys_unexpected (
				make_error_code(std::errc::not_enough_memory)
			);
		}
		catch(...) {}
		return sys_unexpected(make_error_code(std::errc::io_error));
	}

	[[nodiscard]] static bool redirect_status(status_enum value) noexcept
	{
		return value == status::moved_permanently or value == status::found or
			   value == status::see_other or value == status::temporary_redirect or
			   value == status::permanent_redirect;
	}

	[[nodiscard]] static bool same_origin(const url_t &lhs, const url_t &rhs) noexcept
	{
		try {
			return strtls::to_lower(lhs.protocol()) ==
				   strtls::to_lower(rhs.protocol()) and
				   strtls::to_lower(lhs.host()) == strtls::to_lower(rhs.host()) and
				   lhs.port() == rhs.port();
		}
		catch(...) {}
		return false;
	}

	[[nodiscard]] static bool expects_continue(const req_info &info) noexcept
	{
		if constexpr( version_v <= version::v10 ) {
			return false;
		}
		else
		{
			try {
				auto it = info.arg.headers().find(header::expect);
				return it != info.arg.headers().end() and
					strtls::to_lower(*it->second) == "100-continue";
			}
			catch(...) {}
			return false;
		}
	}

	template <method_enum Method>
	[[nodiscard]] result_t<Method> follow_redirects(context_ptr<Method> current, req_info info) noexcept
	{
		static_assert (
			Method == method::get or Method == method::head
		);
		for(size_t followed=0; ; followed++)
		{
			error_code io_error {};
			auto reply_status = current->wait_reply(io_error);
			if( io_error )
				return sys_unexpected(io_error);

			while( current->reply()->parser().is_informational() )
			{
				reply_status = current->wait_reply(io_error);
				if( io_error )
					return sys_unexpected(io_error);
			}
			if( not redirect_status(reply_status) or followed == info.max_redirects )
				return current;

			auto location = current->reply()->header(header::location);
			if( not location )
				return current;

			if constexpr( Method == method::get )
			{
				std::array<char,8192> data {};
				for(;;)
				{
					ignore_unused(current->reply()->read(buffer(data), io_error));
					if( not io_error )
						continue;
					if( io_error != errc::eof )
						return sys_unexpected(io_error);
					break;
				}
			}
			try {
				auto next = url_t::resolve(info.url, **location);
				if( not same_origin(info.url, next) )
				{
					info.arg.unset_header(header::authorization);
					info.arg.cookies().clear();
				}
				info.url = std::move(next);
			}
			catch(...)
			{
				return sys_unexpected (
					make_error_code(std::errc::protocol_error)
				);
			}
			auto next = make_context<Method>(info);
			if( not next )
				return next;
			current = std::move(*next);

			ignore_unused(current->write(io_error));
			if( io_error )
				return sys_unexpected(io_error);
		}
		return {};
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<result_t<Method>> co_follow_redirects
	(context_ptr<Method> current, req_info info, asio::cancellation_slot cancel_slot) noexcept
	{
		static_assert (
			Method == method::get or Method == method::head
		);
		using namespace libgs::operators;

		for(size_t followed=0; ; followed++)
		{
			error_code io_error {};
			auto reply_status = co_await current->wait_reply (
				use_awaitable | io_error | cancel_slot
			);
			if( io_error )
				co_return sys_unexpected(io_error);

			while( current->reply()->parser().is_informational() )
			{
				reply_status = co_await current->wait_reply(
					use_awaitable | io_error | cancel_slot
				);
				if( io_error )
					co_return sys_unexpected(io_error);
			}
			if( not redirect_status(reply_status) or followed == info.max_redirects )
				co_return current;

			auto location = current->reply()->header(header::location);
			if( not location )
				co_return current;

			if constexpr( Method == method::get )
			{
				std::array<char,8192> data {};
				for(;;)
				{
					ignore_unused(co_await current->reply()->read (
						buffer(data), use_awaitable | io_error | cancel_slot
					));
					if( not io_error )
						continue;
					if( io_error != errc::eof )
						co_return sys_unexpected(io_error);
					break;
				}
			}
			try {
				auto next = url_t::resolve(info.url, **location);
				if( not same_origin(info.url, next) )
				{
					info.arg.unset_header(header::authorization);
					info.arg.cookies().clear();
				}
				info.url = std::move(next);
			}
			catch(...)
			{
				co_return sys_unexpected (
					make_error_code(std::errc::protocol_error)
				);
			}
			auto next = co_await co_make_context<Method>(info, cancel_slot);
			if( not next )
				co_return next;
			current = std::move(*next);

			ignore_unused(co_await current->write (
				use_awaitable | io_error | cancel_slot
			));
			if( io_error )
				co_return sys_unexpected(io_error);
		}
		co_return result_t<Method>();
	}

public:
	template <method_enum Method>
	[[nodiscard]] result_t<Method> request(req_info info) noexcept
	{
		const bool continue_100 = expects_continue(info);
		auto context_expected = make_context<Method>(info);

		if( not context_expected )
			return context_expected;

		error_code io_error {};
		ignore_unused((*context_expected)->write(io_error));
		if( io_error )
			return sys_unexpected(io_error);

		if( continue_100 )
		{
			for(;;)
			{
				auto reply = (*context_expected)->reply();
				auto reply_status = reply->status();

				if( reply_status == status::continue_upload or
					(reply_status != status::none and not reply->parser().is_informational()) )
					break;

				ignore_unused((*context_expected)->wait_reply(io_error));
				if( io_error )
					return sys_unexpected(io_error);
			}
		}
		if constexpr( Method == method::get or Method == method::head )
		{
			if( info.max_redirects > 0 )
			{
				return follow_redirects<Method>(
					std::move(*context_expected), std::move(info)
				);
			}
		}
		return context_expected;
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<result_t<Method>> co_request
	(req_info info, asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		const bool continue_100 = expects_continue(info);
		auto context_expected = co_await co_make_context<Method>(info, cancel_slot);

		if( not context_expected )
			co_return context_expected;

		error_code io_error {};
		ignore_unused(co_await (*context_expected)->write (
			use_awaitable | io_error | cancel_slot
		));
		if( io_error )
			co_return sys_unexpected(io_error);

		if( continue_100 )
		{
			for(;;)
			{
				auto reply = (*context_expected)->reply();
				auto reply_status = reply->status();

				if( reply_status == status::continue_upload or
					(reply_status != status::none and not reply->parser().is_informational()) )
					break;

				ignore_unused(co_await (*context_expected)->wait_reply (
					use_awaitable | io_error | cancel_slot
				));
				if( io_error )
					co_return sys_unexpected(io_error);
			}
		}
		if constexpr( Method == method::get or Method == method::head )
		{
			if( info.max_redirects > 0 )
			{
				co_return co_await co_follow_redirects<Method>(
					std::move(*context_expected), std::move(info), cancel_slot
				);
			}
		}
		co_return context_expected;
	}

public:
	[[nodiscard]] result_t<method::put> upload_file
	(req_info info, auto &&opt, auto &&progress) noexcept
	{
		auto pair = info.arg.set_header(std::forward<decltype(opt)>(opt));
		if( not pair )
			return sys_unexpected(pair.error());

		auto context_expected = request<method::put>(std::move(info));
		if( not context_expected )
			return context_expected;

		auto &context = *context_expected;
		if constexpr( version_v > version::v10 )
		{
			if( context->responded() and context->reply()->status() != status::continue_upload )
				return context_expected;
		}
		else
		{
			if( context->responded() )
				return context_expected;
		}
		error_code io_error {};
		ignore_unused(context->upload_file (
			std::move(pair->first), std::move(pair->second),
			std::forward<decltype(progress)>(progress), io_error
		));
		if( io_error )
			return sys_unexpected(io_error);

		ignore_unused(context->wait_reply(io_error));
		if( io_error )
			return sys_unexpected(io_error);
		return context_expected;
	}

	[[nodiscard]] awaitable<result_t<method::put>> co_upload_file
	(req_info info, auto &opt, auto &progress, asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		auto pair = info.arg.set_header(opt);
		if( not pair )
			co_return sys_unexpected(pair.error());

		auto context_expected = co_await co_request<method::put>(
			std::move(info), cancel_slot
		);
		if( not context_expected )
			co_return context_expected;

		auto &context = *context_expected;
		if constexpr( version_v > version::v10 )
		{
			if( context->responded() and
				context->reply()->status() != status::continue_upload )
			{
				co_return context_expected;
			}
		}
		else
		{
			if( context->responded() )
				co_return context_expected;
		}
		error_code io_error {};
		ignore_unused(co_await context->upload_file (
			std::move(pair->first), std::move(pair->second), progress,
			use_awaitable | io_error | cancel_slot
		));
		if( io_error )
			co_return sys_unexpected(io_error);

		ignore_unused(co_await context->wait_reply (
			use_awaitable | io_error | cancel_slot
		));
		if( io_error )
			co_return sys_unexpected(io_error);
		co_return context_expected;
	}

	[[nodiscard]] result_t<method::get> download_file
	(req_info info, auto &&opt, auto &&progress) noexcept
	{
		auto context_expected = request<method::get>(std::move(info));
		if( not context_expected )
			return context_expected;

		error_code io_error {};
		ignore_unused((*context_expected)->reply()->save_file (
			std::forward<decltype(opt)>(opt),
			std::forward<decltype(progress)>(progress), io_error
		));
		if( io_error )
			return sys_unexpected(io_error);
		return context_expected;
	}

	[[nodiscard]] awaitable<result_t<method::get>> co_download_file
	(req_info info, auto &opt, auto &progress, asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		auto context_expected = co_await co_request<method::get>(
			std::move(info), cancel_slot
		);
		if( not context_expected )
			co_return context_expected;

		error_code io_error {};
		ignore_unused(co_await (*context_expected)->reply()->save_file (
			opt, progress, use_awaitable | io_error | cancel_slot
		));
		if( io_error )
			co_return sys_unexpected(io_error);
		co_return context_expected;
	}

public:
	template <method_enum Method>
	[[nodiscard]] result_t<Method> make_context(req_info info) noexcept
	{
		try {
			auto target_expected = target_from_url(info.url);
			if( not target_expected )
				return sys_unexpected(target_expected.error());

			if( info.proxy )
			{
				target_expected = target_from_url(*info.proxy);
				if( not target_expected )
					return sys_unexpected(target_expected.error());
			}
			for(auto &[name,item] : m_cookie_store->cookies_for(info.url))
			{
				if( not info.arg.contains_cookie(name) )
					info.arg.set_cookie(name, std::move(item));
			}
			auto lease_expected = m_pool.get(*target_expected);
			if( not lease_expected )
				return sys_unexpected(lease_expected.error());

			auto context = std::make_shared<context_t<Method>>(
				std::move(*lease_expected), std::move(info.url),
				typename context_t<Method>::options {
					std::move(info.arg), m_cookie_store, info.proxy ?
						request_target_form::absolute : request_target_form::origin,
					info.auto_decompression
				}
			);
			return context;
		}
		catch(const std::system_error &ex) {
			return sys_unexpected(ex.code());
		}
		catch(const std::bad_alloc&)
		{
			return sys_unexpected (
				make_error_code(std::errc::not_enough_memory)
			);
		}
		catch(...) {}
		return sys_unexpected(make_error_code(std::errc::io_error));
	}

	template <method_enum Method>
	[[nodiscard]] awaitable<result_t<Method>> co_make_context
	(req_info info, asio::cancellation_slot cancel_slot) noexcept
	{
		using namespace libgs::operators;
		auto target_expected = target_from_url(info.url);
		if( not target_expected )
			co_return sys_unexpected(target_expected.error());

		if( info.proxy )
		{
			target_expected = target_from_url(*info.proxy);
			if( not target_expected )
				co_return sys_unexpected(target_expected.error());
		}
		try {
			for(auto &[name,item] : m_cookie_store->cookies_for(info.url))
			{
				if( not info.arg.contains_cookie(name) )
					info.arg.set_cookie(name, std::move(item));
			}
		}
		catch(const std::bad_alloc&)
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_enough_memory)
			);
		}
		catch(...) {
			co_return sys_unexpected(make_error_code(std::errc::io_error));
		}
		error_code io_error {};
		auto lease = co_await m_pool.get (
			*target_expected, use_awaitable | io_error | cancel_slot
		);
		if( io_error )
			co_return sys_unexpected(io_error);
		try {
			auto context = std::make_shared<context_t<Method>>(
				std::move(lease), std::move(info.url),
				typename context_t<Method>::options {
					std::move(info.arg), m_cookie_store, info.proxy ?
						request_target_form::absolute : request_target_form::origin,
					info.auto_decompression
				}
			);
			co_return context;
		}
		catch(const std::system_error &ex) {
			co_return sys_unexpected(ex.code());
		}
		catch(const std::bad_alloc&)
		{
			co_return sys_unexpected (
				make_error_code(std::errc::not_enough_memory)
			);
		}
		catch(...) {}
		co_return sys_unexpected(make_error_code(std::errc::io_error));
	}

public:
	connection_pool_t m_pool;
	std::shared_ptr<cookie_jar> m_cookie_store {};
};

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::basic_client(
	core_concepts::match_sched<executor_t> auto &&exec) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<decltype(exec)>(exec))))
{

}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::basic_client(connection_pool_t &&pool) :
	m_impl(std::make_shared<impl>(std::move(pool)))
{

}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::basic_client(basic_client &&other) noexcept :
	m_impl(std::move(other.m_impl))
{

}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>&
basic_client<Exec,Version>::operator=(basic_client &&other) noexcept
{
	if( this != &other )
		m_impl = std::move(other.m_impl);
	return *this;
}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::~basic_client() = default;

template <core_concepts::exec Exec, version_enum Version>
template <method_enum Method, typename Token>
auto basic_client<Exec,Version>::request(req_info info, Token &&token)
	requires request_token_v<Method,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error (
			m_impl->template request<Method>(std::move(info)), token
		);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw (
			m_impl->template request<Method>(std::move(info))
		);
	}
	else
	{
		return detail::initiate_expected<context_ptr<Method>>(
		get_executor(), [impl = m_impl, info = std::move(info)]
		() mutable -> awaitable<sys_expected<context_ptr<Method>>>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->template co_request<Method>(
				std::move(info), state.slot()
			);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec, version_enum Version>
template <typename T, typename Token>
auto basic_client<Exec,Version>::upload_file(req_info info, T &&opt, Token &&token)
	requires upload_file_opt_token_v<T,Token>
{
	return upload_file(std::move(info), std::forward<T>(opt),
		[](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec, version_enum Version>
template <typename T, typename Progress, typename Token>
auto basic_client<Exec,Version>::upload_file(req_info info, T &&opt, Progress &&progress, Token &&token)
	requires upload_file_opt_token_v<T,Token> and concepts::progress_handler<Progress,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error (
			m_impl->upload_file(std::move(info),
				std::forward<T>(opt), std::forward<Progress>(progress)
			), token
		);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw (
			m_impl->upload_file(std::move(info),
				std::forward<T>(opt), std::forward<Progress>(progress)
			)
		);
	}
	else
	{
		return detail::initiate_expected<context_ptr<method::put>>(get_executor(), [
			impl = m_impl, info = std::move(info),
			opt = detail::capture_async_argument(std::forward<T>(opt)),
			progress = detail::capture_async_argument(std::forward<Progress>(progress))
		]() mutable -> awaitable<sys_expected<context_ptr<method::put>>>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->co_upload_file (
				std::move(info), detail::unwrap_async_argument(opt),
				detail::unwrap_async_argument(progress), state.slot()
			);
		},
		std::forward<Token>(token));
	}
}

template <core_concepts::exec Exec, version_enum Version>
template <typename T, typename Token>
auto basic_client<Exec,Version>::download_file(req_info info, T &&opt, Token &&token)
	requires download_file_opt_token_v<T,Token>
{
	return download_file(std::move(info), std::forward<T>(opt),
		[](size_t,size_t){}, std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec, version_enum Version>
template <typename T, typename Progress, typename Token>
auto basic_client<Exec,Version>::download_file(req_info info, T &&opt, Progress &&progress, Token &&token)
	requires download_file_opt_token_v<T,Token> and concepts::progress_handler<Progress,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error (
			m_impl->download_file(std::move(info),
				std::forward<T>(opt), std::forward<Progress>(progress)
			), token
		);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw (
			m_impl->download_file(std::move(info),
				std::forward<T>(opt), std::forward<Progress>(progress)
			)
		);
	}
	else
	{
		return detail::initiate_expected<context_ptr<method::get>>(get_executor(), [
			impl = m_impl, info = std::move(info),
			opt = detail::capture_async_argument(std::forward<T>(opt)),
			progress = detail::capture_async_argument(std::forward<Progress>(progress))
		]() mutable -> awaitable<sys_expected<context_ptr<method::get>>>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->co_download_file (
				std::move(info), detail::unwrap_async_argument(opt),
				detail::unwrap_async_argument(progress), state.slot()
			);
		},
		std::forward<Token>(token));
	}
}

#define LIBGS_HTTP_CLIENT_REQUEST_METHOD(Name, Method) \
	template <core_concepts::exec Exec, version_enum Version> \
	template <typename Token> \
	auto basic_client<Exec,Version>::request_##Name(req_info info, Token &&token) \
		requires request_token_v<method::Method,Token> { \
		return request<method::Method>( \
			std::move(info), std::forward<Token>(token) \
		); \
	}

LIBGS_HTTP_CLIENT_REQUEST_METHOD(get    , get    )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(put    , put    )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(post   , post   )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(head   , head   )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(patch  , patch  )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(delete , delet  )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(options, options)
LIBGS_HTTP_CLIENT_REQUEST_METHOD(trace  , trace  )
LIBGS_HTTP_CLIENT_REQUEST_METHOD(connect, connect)

#undef LIBGS_HTTP_CLIENT_REQUEST_METHOD

template <core_concepts::exec Exec, version_enum Version>
template <method_enum Method, typename Token>
auto basic_client<Exec,Version>::make_context(req_info info, Token &&token)
	requires request_token_v<Method,Token>
{
	if constexpr( is_error_code_token_v<Token> )
	{
		return detail::expected_value_or_error (
			m_impl->template make_context<Method>(std::move(info)), token
		);
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		return detail::expected_value_or_throw (
			m_impl->template make_context<Method>(std::move(info))
		);
	}
	else
	{
		return detail::initiate_expected<context_ptr<Method>>(get_executor(),
		[impl = m_impl, info = std::move(info)]() mutable -> awaitable<sys_expected<context_ptr<Method>>>
		{
			auto state = co_await asio::this_coro::cancellation_state;
			co_return co_await impl->template co_make_context<Method>(
				std::move(info), state.slot()
			);
		},
		std::forward<Token>(token));
	}
}

#define LIBGS_HTTP_CLIENT_MAKE_METHOD(Name, Method) \
	template <core_concepts::exec Exec, version_enum Version> \
	template <typename Token> \
	auto basic_client<Exec,Version>::make_##Name(req_info info, Token &&token) \
		requires request_token_v<method::Method,Token> { \
		return make_context<method::Method>( \
			std::move(info), std::forward<Token>(token) \
		); \
	}

LIBGS_HTTP_CLIENT_MAKE_METHOD(get    , get    )
LIBGS_HTTP_CLIENT_MAKE_METHOD(put    , put    )
LIBGS_HTTP_CLIENT_MAKE_METHOD(post   , post   )
LIBGS_HTTP_CLIENT_MAKE_METHOD(head   , head   )
LIBGS_HTTP_CLIENT_MAKE_METHOD(patch  , patch  )
LIBGS_HTTP_CLIENT_MAKE_METHOD(delete , delet  )
LIBGS_HTTP_CLIENT_MAKE_METHOD(options, options)
LIBGS_HTTP_CLIENT_MAKE_METHOD(trace  , trace  )
LIBGS_HTTP_CLIENT_MAKE_METHOD(connect, connect)

#undef LIBGS_HTTP_CLIENT_MAKE_METHOD

template <core_concepts::exec Exec, version_enum Version>
std::shared_ptr<cookie_jar> basic_client<Exec,Version>::cookie_store() noexcept
{
	return m_impl->m_cookie_store;
}

template <core_concepts::exec Exec, version_enum Version>
consteval version_enum basic_client<Exec,Version>::version() noexcept
{
	return version_v;
}

template <core_concepts::exec Exec, version_enum Version>
basic_client<Exec,Version>::executor_t basic_client<Exec,Version>::get_executor() noexcept
{
	return m_impl->m_pool.get_executor();
}

} //namespace libgs::http


#endif //LIBGS_HTTP_CLIENT_DETAIL_CLIENT_H
