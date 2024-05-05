
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SERVER_H
#define LIBGS_HTTP_SERVER_DETAIL_SERVER_H

#include <libgs/core/lock_free_queue.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_execution Exec>
class basic_server<CharT,Exec>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using request_queue = lock_free_queue<request_ptr>;
	using callback_queue = lock_free_queue<callback_t<request_ptr,error_code>>;

public:
	template <typename...Args>
	explicit impl(Args&&...args) requires requires { tcp_server_type(std::forward<Args>(args)...); } :
		impl(std::make_shared<tcp_server_type>(std::forward<Args>(args)...)) {}

	explicit impl(tcp_server_ptr &&tcp_server) :
		m_tcp_server(std::move(tcp_server)) {}

public:
	void async_start(size_t max) noexcept
	{
		if( m_is_start )
			return ;
		m_is_start = true;

		co_spawn_detached([this, max]() -> awaitable<void>
		{
			bool abd = false;
			try {
				co_await do_tcp_accept(max);
			}
			catch(std::exception &ex)
			{
				libgs_log_error("libgs::http::server: Unhandled exception: {}.", ex);
				abd = true;
			}
			catch(...)
			{
				libgs_log_error("libgs::http::server: Unknown exception.");
				abd = true;
			}
			co_await m_tcp_server->co_cancel();
			m_is_start = false;
			if( abd )
				abort();
			co_return ;
		});
	}

	void rule_path_check(str_type &str)
	{
		auto n_it = std::unique(str.begin(), str.end(), [](CharT c0, CharT c1){
			return c0 == c1 and c0 == 0x2F/*/*/;
		});
		if( n_it != str.end() )
			str.erase(n_it, str.end());

		constexpr auto root = detail::_key_static_string<CharT>::root;
		if( not str.starts_with(root) )
			str = root + str;
	}

private:
	[[nodiscard]] awaitable<void> do_tcp_accept(size_t max)
	{
		m_tcp_server->start(max);
		tcp_socket_ptr socket;
		for(;;)
		{
			try {
				socket = co_await m_tcp_server->accept();
			}
			catch(std::system_error &ex)
			{
				call_on_system_error(ex.code());
				continue;
			}
			auto ktime = m_keepalive_timeout;
			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = std::move(ktime)]() -> awaitable<void>
			{
				bool abd = false;
				try {
					co_await do_tcp_service(std::move(socket), ktime);
				}
				catch(std::exception &ex)
				{
					libgs_log_error("libgs::http::server: service: Unhandled exception: {}.", ex);
					abd = true;
				}
				catch(...)
				{
					libgs_log_error("libgs::http::server: service: Unknown exception.");
					abd = true;
				}
				error_code error;
				socket->close(error);
				if( abd )
					abort();
				co_return ;
			},
			m_tcp_server->pool());
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(io::tcp_socket_ptr socket, const std::chrono::milliseconds &keepalive_time)
	{
		basic_server::parser parser;
		char buf[65536] = {0};
		for(;;)
		{
			try {
				auto size = co_await socket->read({buf, 65536}, {keepalive_time});
				bool parse_res = parser.append({buf, size});

				if( not parse_res )
					continue;
			}
			catch(std::system_error &ex)
			{
				if( ex.code().value() == errc::eof or ex.code().value() == errc::timed_out )
					break;
				call_on_system_error(ex.code());
			}
			context_type context(socket, parser);
			co_await call_on_request(parser, context);

			if( not context.response().headers_writed() )
				co_await call_on_default(context);
			parser.reset();
		}
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> call_on_request(basic_server::parser &parser, context_type &context)
	{
		tk_handler_ptr handler {};
		size_t weight = 0;

		for(auto &[rule, _handler] : m_request_handler_map)
		{
			auto _weight = wildcard_match(rule, parser.path());
			if( _weight == 0 )
				continue;
			else if( _weight > weight )
			{
				handler = _handler;
				weight = _weight;
			}
		}
		if( weight == 0 )
		{
			context.response().set_status(status::not_found);
			co_return ;
		}
		else if( (handler->method & parser.method()) == 0 )
		{
			context.response().set_status(status::method_not_allowed);
			co_return ;
		}
		else if( handler->aop.index() == 0 )
			co_await for_aop(std::get<0>(handler->aop), context);
		else // index == 1
			co_await ctrlr_aop(std::get<1>(handler->aop), context);
		co_return ;
	}

	[[nodiscard]] awaitable<void> call_on_default(context_type &context)
	{
		if( m_default_handler )
		{
			try {
				co_await m_default_handler(context);
			}
			catch(std::exception &ex) {
				call_on_exception(context, ex);
			}
			if( context.response().headers_writed() )
				co_return ;
		}
		co_await context.response().write();
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> for_aop(aop_token &tk, context_type &context)
	{
		try {
			for(auto &aop : tk.aops)
			{
				if( co_await aop->before(context) )
					co_return ;
			}
			if( tk.callback )
				co_await tk.callback(context);
			for(auto &aop : tk.aops)
			{
				if( co_await aop->after(context) )
					break;
			}
		}
		catch(std::exception &ex)
		{
			for(auto &aop : tk.aops)
			{
				if( aop->exception(context, ex) )
					co_return ;
			}
			call_on_exception(context, ex);
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> ctrlr_aop(ctrlr_aop_ptr &ctrlr, context_type &context)
	{
		try
		{
			if( co_await ctrlr->before(context))
				co_return;

			co_await ctrlr->service(context);
			co_await ctrlr->after(context);
		}
		catch(std::exception &ex)
		{
			if( ctrlr->exception(context, ex) )
				co_return ;
			call_on_exception(context, ex);
		}
		co_return ;
	}

private:
	void call_on_system_error(const error_code &error)
	{
		if( m_system_error_callback )
		{
			if( m_system_error_callback(error) )
				return ;
		}
		throw system_error(error, "libgs::http::server");
	}

	void call_on_exception(context_type &context, std::exception &ex)
	{
		context.response().set_status(status::internal_server_error);
		if( m_exception_callback )
		{
			if( m_exception_callback(context, ex) )
				return ;
		}
		throw runtime_error("libgs::http::server");
	}

public:
	struct tk_handler
	{
		using aop_var = std::variant<aop_token, ctrlr_aop_ptr>;
		explicit tk_handler(aop_var aop) : aop(std::move(aop)) {}

		template <http::method...method>
		tk_handler &bind_method()
		{
			if constexpr( sizeof...(method) == 0 )
			{
				using hm = http::method;
				this->method = {
					hm::GET, hm::PUT, hm::POST, hm::HEAD, hm::DELETE, hm::OPTIONS, hm::CONNECT, hm::TRACH
				};
			}
			else
			{
				(void) std::initializer_list<int> {
					(this->method |= method, 0) ...
				};
			}
			return *this;
		}
		http::methods method {};
		aop_var aop {};
	};
	using tk_handler_ptr = std::shared_ptr<tk_handler>;

public:
	tcp_server_ptr m_tcp_server {};
	std::map<str_type, tk_handler_ptr> m_request_handler_map;

	request_handler m_default_handler {};
	system_error_handler m_system_error_callback {};
	exception_handler m_exception_callback {};

	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(size_t tcount) :
	base_type(execution::io_context().get_executor()),
	m_impl(new impl(tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution_context Context>
basic_server<CharT,Exec>::basic_server(Context &context, size_t tcount) :
	base_type(context.get_executor()),
	m_impl(new impl(context, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
template <concept_execution Exec0>
basic_server<CharT,Exec>::basic_server(asio_acceptor &&acceptor, size_t tcount) :
	base_type(acceptor.get_executor()),
	m_impl(new impl(std::move(acceptor), tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(const executor_type &exec, size_t tcount) :
	base_type(exec),
	m_impl(new impl(exec, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::basic_server(tcp_server_ptr tcp_server) :
	base_type(tcp_server.executor()),
	m_impl(new impl(std::move(tcp_server)))
{

}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec>::~basic_server()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::bind(io::ip_endpoint ep, opt_token<error_code&> tk)
{
	m_impl->m_tcp_server->bind(std::move(ep), tk);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::start(start_token tk)
{
	m_impl->async_start(tk.max);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename Func, typename...AopPtr>
basic_server<CharT,Exec>::aop_token::aop_token(Func &&callback, AopPtr&&...a) requires
	detail::concept_request_handler<Func,CharT,Exec> and detail::concept_aop_ptr_list<CharT,Exec,AopPtr...> :
	aops{aop_ptr(std::forward<AopPtr>(a))...}, callback(std::forward<Func>(callback))
{
	assert([this]
	{
		for(auto &aop : aops)
		{
			if( not aop )
				return false;
		}
		return true;
	}());
}

template <concept_char_type CharT, concept_execution Exec>
template <http::method...method>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_request(str_view_type path_rule, aop_token tk)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	str_type rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	it->second = std::make_shared<typename impl::tk_handler>(std::move(tk));
	it->second->template bind_method<method...>();
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <http::method...method>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_request(str_view_type path_rule, ctrlr_aop_ptr ctrlr)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	str_type rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	it->second = std::make_shared<typename impl::tk_handler>(std::move(ctrlr));
	it->second->template bind_method<method...>();
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <http::method...method>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_request(str_view_type path_rule, ctrlr_aop *ctrlr)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	str_type rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr(ctrlr));
	it->second->template bind_method<method...>();
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename Func>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_default(Func &&callback)
	requires detail::concept_request_handler<Func,CharT,Exec>
{
	m_impl->m_default_handler = std::forward<Func>(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_system_error(system_error_handler callback)
{
	m_impl->m_system_error_callback = std::move(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_exception(exception_handler callback)
{
	m_impl->m_exception_callback = std::move(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_request(str_view_type path_rule)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::unbound_request: path_rule is empty.");
	m_impl->m_request_handler_map.erase({path_rule.data(), path_rule.size()});
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_system_error()
{
	m_impl->m_system_error_callback = {};
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_exception()
{
	m_impl->m_exception_callback = {};
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
template <typename Rep, typename Period>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::set_keepalive_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_keepalive_timeout = duration_cast<milliseconds>(d);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
awaitable<void> basic_server<CharT,Exec>::co_cancel() noexcept
{
	m_impl->m_is_start = false;
	co_await m_impl->m_tcp_server->co_cancel();
}

template <concept_char_type CharT, concept_execution Exec>
void basic_server<CharT,Exec>::cancel() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_tcp_server->cancel();
}

template <concept_char_type CharT, concept_execution Exec>
const io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object() const
{
	return *m_impl->m_tcp_server;
}

template <concept_char_type CharT, concept_execution Exec>
io::basic_tcp_server<Exec> &basic_server<CharT,Exec>::native_object()
{
	return *m_impl->m_tcp_server;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
