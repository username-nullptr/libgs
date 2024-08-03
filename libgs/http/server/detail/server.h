
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

#include <spdlog/spdlog.h>

namespace libgs::http
{

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
class basic_server<CharT,Stream,Exec>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	template <concept_execution_context Context>
	explicit impl(basic_acceptor_wrap<socket_t> &&next_layer, Context &&service_exec) :
		m_next_layer(std::move(next_layer)), m_service_exec(service_exec.get_executor()) {}

	explicit impl(basic_acceptor_wrap<socket_t> &&next_layer, service_exec_t &service_exec) :
		m_next_layer(std::move(next_layer)), m_service_exec(service_exec) {}

	template <typename Stream0, typename Exec0>
	impl(typename basic_server<CharT,Stream0,Exec0>::impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)),
		m_service_exec(other.m_service_exec),
		m_request_handler_map(std::move(other.m_request_handler_map)),
		m_sss(std::move(other.m_sss)),
		m_default_handler(std::move(other.m_default_handler)),
		m_system_error_handler(std::move(other.m_system_error_handler)),
		m_exception_handler(std::move(other.m_exception_handler)),
		m_keepalive_timeout(other.m_keepalive_timeout),
		m_is_start(other.m_is_start)
	{
		other.m_keepalive_timeout = std::chrono::milliseconds(5000);
		other.m_is_start = false;
	}

	impl(impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)),
		m_service_exec(other.m_service_exec),
		m_request_handler_map(std::move(other.m_request_handler_map)),
		m_sss(std::move(other.m_sss)),
		m_default_handler(std::move(other.m_default_handler)),
		m_system_error_handler(std::move(other.m_system_error_handler)),
		m_exception_handler(std::move(other.m_exception_handler)),
		m_keepalive_timeout(other.m_keepalive_timeout),
		m_is_start(other.m_is_start)
	{
		other.m_keepalive_timeout = std::chrono::milliseconds(5000);
		other.m_is_start = false;
	}

	template <typename Stream0, typename Exec0>
	impl &operator=(typename basic_server<CharT,Stream0,Exec0>::impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		m_service_exec = other.m_service_exec;

		m_request_handler_map = std::move(other.m_request_handler_map);
		m_sss = std::move(other.m_sss);

		m_default_handler = std::move(other.m_default_handler);
		m_system_error_handler = std::move(other.m_system_error_handler);
		m_exception_handler = std::move(other.m_exception_handler);

		m_keepalive_timeout = other.m_keepalive_timeout;
		m_is_start = other.m_is_start;
	
		other.m_keepalive_timeout = std::chrono::milliseconds(5000);
		other.m_is_start = false;
		return *this;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		m_service_exec = other.m_service_exec;

		m_request_handler_map = std::move(other.m_request_handler_map);
		m_sss = std::move(other.m_sss);

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
	void async_start(size_t max, error_code&) noexcept
	{
		if( m_is_start )
			return ;
		m_is_start = true;

		co_spawn_detached([this, max]() -> awaitable<void>
		{
			bool abd = false;
			try
			{
				m_next_layer.acceptor().listen(static_cast<int>(max));
				co_await do_tcp_accept();
			}
			catch(std::exception &ex)
			{
				spdlog::error("libgs::http::server: Unhandled exception: {}.", ex);
				abd = true;
			}
			catch(...)
			{
				spdlog::error("libgs::http::server: Unknown exception.");
				abd = true;
			}
			m_next_layer.acceptor().cancel();
			error_code error;
			m_next_layer.acceptor().close(error);
			m_is_start = false;
			if( abd )
				forced_termination();
			co_return ;
		},
		m_next_layer.acceptor().get_executor());
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
	[[nodiscard]] awaitable<void> do_tcp_accept()
	{
		do try
		{
			auto socket = co_await m_next_layer.accept(m_service_exec);
			if( not socket_operation_helper<socket_t>::is_open(socket) )
				continue;

			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = m_keepalive_timeout]() mutable -> awaitable<void>
			{
				bool abd = false;
				try {
					co_await do_tcp_service(socket, ktime);
				}
				catch(std::exception &ex)
				{
					spdlog::error("libgs::http::server: service: Unhandled exception: {}.", ex);
					abd = true;
				}
				catch(...)
				{
					spdlog::error("libgs::http::server: service: Unknown exception.");
					abd = true;
				}
				socket_operation_helper<socket_t>::close(socket);
				if( abd )
					forced_termination();
				co_return ;
			},
			m_service_exec);
		}
		catch(std::system_error &ex)
		{
			call_on_system_error(ex.code());
			continue;
		}
		while(true);
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(socket_t &socket, const std::chrono::milliseconds &keepalive_time)
	{
		basic_server::parser parser;
		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] = {0};
		for(;;)
		{
			try {
				auto var = co_await (socket.async_read_some(buffer(buf, buf_size), use_awaitable) or
									 co_sleep_for(keepalive_time));
				if( var.index() == 1 )
					break;

				auto size = std::get<0>(var);
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

			context _context(std::move(socket), parser, m_sss);
			co_await call_on_request(_context);

			if( not _context.response().headers_writed() )
				co_await call_on_default(_context);

			parser.reset();
			socket = std::move(_context.request().next_layer());
		}
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> call_on_request(context &_context)
	{
		tk_handler_ptr handler {};
		int32_t weight = std::numeric_limits<int32_t>::max();

		for(auto &[rule, _handler] : m_request_handler_map)
		{
			auto _weight = _context.request().path_match(rule);
			if( _weight < 0 )
				continue;
			else if( _weight < weight )
			{
				handler = _handler;
				weight = _weight;
			}
		}
		if( weight < 0 )
		{
			_context.response().set_status(status::not_found);
			co_return ;
		}
		else if( (handler->method & _context.request().method()) == 0 )
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
		constexpr const char *def_html =
			"<!DOCTYPE html>"
			"<html>"
			"<head>"
			"	<meta charset=\"utf-8\">"
			"	<title>{0}</title>"
			"</head>"
			"<body>"
			"	<h1>{0}</h1>{1}"
			"	<p>[ This is the server's default reply ]</p>"
			"	<p>-----------------------------------------------</p>"
			"	<p>This is an open source C++ (ASIO) server.</p>"
			"	<a href=\"https://gitee.com/jin-xiaoqiang/libgs.git\" target=\"_blank\">"
			"		Source code repository (Gitee)"
			"	</a>"
			"</body>"
			"</html>";
		std::string data;
		if( _context.response().status() == status::ok )
			data = std::format(def_html, "Welcome to LIBGS", "");
		else
		{
			auto status = std::format("<h2>{} ({})</h2>", to_status_description(_context.response().status()), _context.response().status());
			data = std::format(def_html, "LIBGS", status);
		}
		co_await _context.response()
				.set_header(header::content_type, "text/html")
				.async_write(asio::buffer(data, data.size()), use_awaitable);
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
	service_exec_t m_service_exec;

	std::map<string_t, tk_handler_ptr> m_request_handler_map;
	session_set m_sss;

	request_handler m_default_handler {};
	system_error_handler m_system_error_handler {};
	exception_handler m_exception_handler {};

	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <concept_execution_context Context>
basic_server<CharT,Stream,Exec>::basic_server(basic_acceptor_wrap<socket_t> &&next_layer, Context &&service_exec) :
	m_impl(new impl(std::move(next_layer), std::forward<Context>(service_exec)))
{

}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec>::~basic_server()
{
	delete m_impl;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <typename Stream0, typename Exec0>
basic_server<CharT,Stream,Exec>::basic_server(basic_server<CharT,Stream0,Exec0> &&other) noexcept
	requires concept_constructible<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
			 concept_constructible<service_exec_t,typename Stream::executor_type> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <typename Stream0, typename Exec0>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::operator=
(basic_server<CharT,Stream0,Exec0> &&other) noexcept
	requires concept_assignable<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
			 concept_assignable<service_exec_t,typename Stream::executor_type>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::bind(endpoint_t ep)
{
	error_code error;
	bind(std::move(ep), error);
	if( error )
		throw system_error(error, "libgs::http::basic_server::bind");
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::bind
(endpoint_t ep, error_code &error) noexcept
{
	auto &acceptor = m_impl->m_next_layer.acceptor();
	if( not acceptor.is_open() )
	{
		if( ep.address().is_v4())
			acceptor.open(asio::ip::tcp::v4(), error);
		else
			acceptor.open(asio::ip::tcp::v6(), error);
		if( error )
			return *this;
	}
	acceptor.set_option(asio::socket_base::reuse_address(true), error);
	if( error )
		return *this;

	acceptor.bind(std::move(ep), error);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start(size_t max)
{
	error_code error;
	start(max, error);
	if( error )
		throw system_error(error, "libgs::http::basic_server::start");
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start
(size_t max, error_code &error) noexcept
{
	m_impl->async_start(max, error);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start
(error_code &error) noexcept
{
	return start(asio::socket_base::max_listen_connections, error);
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <http::method...method, typename Func, typename...AopPtr>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::on_request(string_view_t path_rule, Func &&func, AopPtr&&...aops) requires
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
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <http::method...method>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_request
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
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <http::method...method>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_request
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
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <typename Func>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_default(Func &&func)
	requires detail::concept_request_handler<Func,socket_t,CharT>
{
	m_impl->m_default_handler = std::forward<Func>(func);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::on_system_error(system_error_handler func)
{
	m_impl->m_system_error_handler = std::move(func);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::on_exception(exception_handler func)
{
	m_impl->m_exception_handler = std::move(func);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::unbound_request(string_view_t path_rule)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::unbound_request: path_rule is empty.");
	m_impl->m_request_handler_map.erase({path_rule.data(), path_rule.size()});
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::unbound_system_error()
{
	m_impl->m_system_error_handler = {};
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::unbound_exception()
{
	m_impl->m_exception_handler = {};
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
template <typename Rep, typename Period>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::set_keepalive_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_keepalive_timeout = duration_cast<milliseconds>(d);
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
awaitable<void> basic_server<CharT,Stream,Exec>::co_stop() noexcept
{
	m_impl->m_is_start = false;
	co_return co_await m_impl->m_next_layer.acceptor().co_stop();
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
const typename basic_server<CharT,Stream,Exec>::executor_t&
basic_server<CharT,Stream,Exec>::get_executor() noexcept
{
	return m_impl->m_next_layer.acceptor().get_executor();
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::stop() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_next_layer.acceptor().stop();
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::cancel() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_next_layer.acceptor().cancel();
	return *this;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
const typename basic_server<CharT,Stream,Exec>::next_layer_t&
basic_server<CharT,Stream,Exec>::next_layer() const
{
	return m_impl->m_next_layer;
}

template <concept_char_type CharT, concept_tcp_stream Stream, concept_execution Exec>
typename basic_server<CharT,Stream,Exec>::next_layer_t&
basic_server<CharT,Stream,Exec>::next_layer()
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
