
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

template <concept_char_type CharT, concept_execution Exec, typename Derived>
class basic_server<CharT,Exec,Derived>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using socket_ptr = typename next_layer_t::socket_ptr;

public:
	template <typename...Args>
	explicit impl(Args&&...args) requires requires { next_layer_t(std::forward<Args>(args)...); } :
		m_next_layer(std::forward<Args>(args)...) {}

	explicit impl(io::basic_tcp_server<executor_t> &&next_layer) :
		m_next_layer(std::move(next_layer)) {}

	impl(impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)),
		m_request_handler_map(std::move(other.m_next_layer)),
		m_default_handler(std::move(other.m_default_handler)),
		m_system_error_handler(std::move(other.m_system_error_handler)),
		m_exception_handler(std::move(other.m_exception_handler)),
		m_keepalive_timeout(other.m_keepalive_timeout),
		m_is_start(other.m_is_start)
	{
		other.m_keepalive_timeout = std::chrono::milliseconds(5000);
		other.m_is_start = false;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		m_request_handler_map = std::move(other.m_next_layer);

		m_default_handler = std::move(other.m_default_handler);
		m_system_error_handler = std::move(other.m_system_error_handler);
		m_exception_handler = std::move(other.m_exception_handler);

		m_keepalive_timeout = other.m_keepalive_timeout;
		m_is_start = other.m_is_start;
	
		other.m_keepalive_timeout = std::chrono::milliseconds(5000);
		other.m_is_start = false;
		return *this;
	}

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
			co_await m_next_layer.co_cancel();
			m_is_start = false;
			if( abd )
				forced_termination();
			co_return ;
		},
		m_next_layer.executor());
	}

	void rule_path_check(string_t &str)
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
		m_next_layer.start(max);
		socket_ptr socket;
		for(;;)
		{
			try {
				socket = co_await m_next_layer.accept();
			}
			catch(std::system_error &ex)
			{
				call_on_system_error(ex.code());
				continue;
			}
			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = std::move(m_keepalive_timeout)]() -> awaitable<void>
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
					forced_termination();
				co_return ;
			},
			m_next_layer.pool());
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(socket_ptr socket, const std::chrono::milliseconds &keepalive_time)
	{
		basic_server::parser parser;
		char buf[65536] = {0};
		for(;;)
		{
			try {
				auto size = co_await socket->read_some({buf, 65536}, {keepalive_time});
				if( size == 0 )
					break;

				bool parse_res = parser.append({buf, size});
				if( not parse_res )
					continue;
			}
			catch(std::system_error &ex)
			{
				auto eno = ex.code().value();
				if( eno == errc::bad_descriptor or eno == errc::eof or eno == errc::timed_out )
					break;
				call_on_system_error(ex.code());
			}
			context _context(std::move(*socket), parser);
			co_await call_on_request(parser, _context);

			if( not _context.response().headers_writed() )
				co_await call_on_default(_context);
			parser.reset();
		}
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> call_on_request(basic_server::parser &parser, context &_context)
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
			_context.response().set_status(status::not_found);
			co_return ;
		}
		else if( (handler->method & parser.method()) == 0 )
		{
			_context.response().set_status(status::method_not_allowed);
			co_return ;
		}
		try
		{
			if( co_await handler->aop->before(_context) )
				co_return ;

			co_await handler->aop->service(_context);
			co_await handler->aop->after(_context);
		}
		catch(std::exception &ex)
		{
			if( handler->aop->exception(_context, ex) )
				co_return ;
			call_on_exception(_context, ex);
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> call_on_default(context &_context)
	{
		if( m_default_handler )
		{
			try {
				co_await m_default_handler(_context);
			}
			catch(std::exception &ex) {
				call_on_exception(_context, ex);
			}
			if( _context.response().headers_writed() )
				co_return ;
		}
		co_await _context.response().write();
		co_return ;
	}

private:
	void call_on_system_error(const error_code &error)
	{
		if( m_system_error_handler )
		{
			if( m_system_error_handler(error) )
				return ;
		}
		throw system_error(error, "libgs::http::server");
	}

	void call_on_exception(context &_context, std::exception &ex)
	{
		_context.response().set_status(status::internal_server_error);
		if( m_exception_handler )
		{
			if( m_exception_handler(_context, ex) )
				return ;
		}
		throw ex;
	}

public:
	class multi_ctrlr_aop : public ctrlr_aop
	{
	public:
		template <typename Func, typename...AopPtr>
		multi_ctrlr_aop(Func &&func, AopPtr&&...aops) :
			m_aops{aop_ptr(std::forward<AopPtr>(aops))...},
			m_func(std::forward<Func>(func))
		{
			assert(m_func);
		}

	public:
		awaitable<bool> before(context &_context) override
		{
			for(auto &aop : m_aops)
			{
				if( co_await aop->before(_context) )
					co_return true;
			}
			co_return false;
		}

		awaitable<bool> after(context &_context) override
		{
			for(auto &aop : m_aops)
			{
				if( co_await aop->after(_context) )
					co_return true;
			}
			co_return false;
		}

		bool exception(context &_context, std::exception &ex) override
		{
			for(auto &aop : m_aops)
			{
				if( aop->exception(_context, ex) )
					return true;
			}
			return false;
		}

	public:
		awaitable<void> service(context &_context) override {
			co_return co_await m_func(_context);
		}

	private:
		std::vector<aop_ptr> m_aops {};
		request_handler m_func {};
	};

	struct tk_handler
	{
		explicit tk_handler(ctrlr_aop_ptr aop) : aop(std::move(aop)) {}

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
		ctrlr_aop_ptr aop {};
	};
	using tk_handler_ptr = std::shared_ptr<tk_handler>;

public:
	next_layer_t m_next_layer;
	std::map<string_t, tk_handler_ptr> m_request_handler_map;

	request_handler m_default_handler {};
	system_error_handler m_system_error_handler {};
	exception_handler m_exception_handler {};

	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <concept_char_type CharT, concept_execution Exec, typename Derived>
basic_server<CharT,Exec,Derived>::basic_server(size_t tcount) :
	base_t(execution::io_context().get_executor()),
	m_impl(new impl(tcount))
{

}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <concept_execution_context Context>
basic_server<CharT,Exec,Derived>::basic_server(Context &context, size_t tcount) :
	base_t(context.get_executor()),
	m_impl(new impl(context, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <concept_execution Exec0>
basic_server<CharT,Exec,Derived>::basic_server(io::basic_tcp_server<executor_t> &&next_layer, size_t tcount) :
	base_t(next_layer.executor()),
	m_impl(new impl(std::move(next_layer), tcount))
{

}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
basic_server<CharT,Exec,Derived>::basic_server(const executor_t &exec, size_t tcount) :
	base_t(exec),
	m_impl(new impl(exec, tcount))
{

}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
basic_server<CharT,Exec,Derived>::~basic_server()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
basic_server<CharT,Exec,Derived>::basic_server(basic_server &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
basic_server<CharT,Exec,Derived> &basic_server<CharT,Exec,Derived>::operator=(basic_server &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::bind
(io::ip_endpoint ep, opt_token<error_code&> tk)
{
	m_impl->m_next_layer.bind(std::move(ep), tk);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::start(start_token tk)
{
	m_impl->async_start(tk.max);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <http::method...method, typename Func, typename...AopPtr>
typename basic_server<CharT,Exec,Derived>::derived_t&
basic_server<CharT,Exec,Derived>::on_request(string_view_t path_rule, Func &&func, AopPtr&&...aops) requires
	detail::concept_request_handler<Func,socket_t,CharT> and 
	detail::concept_aop_ptr_list<socket_t,CharT,AopPtr...>
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	string_t rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	auto aop = new typename impl::multi_ctrlr_aop(std::forward<Func>(func), std::forward<AopPtr>(aops)...);
	it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr(aop));
	it->second->template bind_method<method...>();
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <http::method...method>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::on_request
(string_view_t path_rule, ctrlr_aop_ptr ctrlr)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	string_t rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	it->second = std::make_shared<typename impl::tk_handler>(std::move(ctrlr));
	it->second->template bind_method<method...>();
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <http::method...method>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::on_request
(string_view_t path_rule, ctrlr_aop *ctrlr)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

	string_t rule(path_rule.data(), path_rule.size());
	m_impl->rule_path_check(rule);
	auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

	if( not res )
		throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

	it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr(ctrlr));
	it->second->template bind_method<method...>();
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <typename Func>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::on_default(Func &&func)
	requires detail::concept_request_handler<Func,socket_t,CharT>
{
	m_impl->m_default_handler = std::forward<Func>(func);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t&
basic_server<CharT,Exec,Derived>::on_system_error(system_error_handler func)
{
	m_impl->m_system_error_handler = std::move(func);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t&
basic_server<CharT,Exec,Derived>::on_exception(exception_handler func)
{
	m_impl->m_exception_handler = std::move(func);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t&
basic_server<CharT,Exec,Derived>::unbound_request(string_view_t path_rule)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::unbound_request: path_rule is empty.");
	m_impl->m_request_handler_map.erase({path_rule.data(), path_rule.size()});
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::unbound_system_error()
{
	m_impl->m_system_error_handler = {};
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::unbound_exception()
{
	m_impl->m_exception_handler = {};
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
template <typename Rep, typename Period>
typename basic_server<CharT,Exec,Derived>::derived_t&
basic_server<CharT,Exec,Derived>::set_keepalive_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_keepalive_timeout = duration_cast<milliseconds>(d);
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
awaitable<void> basic_server<CharT,Exec,Derived>::co_cancel() noexcept
{
	m_impl->m_is_start = false;
	co_await m_impl->m_next_layer.co_cancel();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::derived_t &basic_server<CharT,Exec,Derived>::cancel() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_next_layer.cancel();
	return this->derived();
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
const typename basic_server<CharT,Exec,Derived>::next_layer_t &basic_server<CharT,Exec,Derived>::next_layer() const
{
	return m_impl->m_next_layer;
}

template <concept_char_type CharT, concept_execution Exec, typename Derived>
typename basic_server<CharT,Exec,Derived>::next_layer_t &basic_server<CharT,Exec,Derived>::next_layer()
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
