
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

namespace libgs::http
{

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
class basic_server<CharT,Stream,Exec>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using request_handler_t = std::function<awaitable<void>(context_t&)>;

public:
	explicit impl(basic_acceptor_wrap<socket_t> &&next_layer, const service_exec_t &service_exec) :
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
	void async_start(size_t max, error_code &error) noexcept
	{
		if( m_is_start )
			return ;
		m_next_layer.acceptor().listen(static_cast<int>(max), error);
		if( error )
			return ;
		else
			m_is_start = true;

		co_spawn_detached([this]() -> awaitable<void>
		{
			bool abd = false;
			try {
				co_await do_tcp_accept();
			}
			catch(const std::exception &ex)
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

		constexpr auto root = detail::string_pool<CharT>::root;
		if( not str.starts_with(root) )
			str = root + str;
	}

private:
	[[nodiscard]] awaitable<void> do_tcp_accept()
	{
		do try
		{
			auto socket = co_await m_next_layer.accept(m_service_exec);
			if( not socket_operation_helper<socket_t>(socket).is_open() )
				continue;

			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = m_keepalive_timeout]() mutable -> awaitable<void>
			{
				bool abd = false;
				try {
					co_await do_tcp_service(socket, ktime);
				}
				catch(const std::exception &ex)
				{
					spdlog::error("libgs::http::server: service: Unhandled exception: {}.", ex);
					abd = true;
				}
				catch(...)
				{
					spdlog::error("libgs::http::server: service: Unknown exception.");
					abd = true;
				}
				socket_operation_helper<socket_t>(socket).close();
				if( abd )
					forced_termination();
				co_return ;
			},
			m_service_exec);
		}
		catch(std::system_error &ex)
		{
			if( not m_is_start )
				break;
			call_on_system_error(ex.code());
			continue;
		}
		while(true);
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(socket_t &socket, const std::chrono::milliseconds &keepalive_time)
	{
		using namespace std::chrono_literals;
		const auto *time = &m_first_reading_time;

		parser_t parser;
		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] = {0};
		for(;;)
		{
			try {
				auto var = co_await (
					socket.async_read_some(buffer(buf, buf_size), use_awaitable) or
					co_sleep_for(*time)
				);
				if( var.index() == 1 )
					break;

				auto size = std::get<0>(var);
				if( size == 0 )
					break;

				error_code error;
				parser.append({buf, size}, error);
				if( error )
				{
					spdlog::warn("libgs::http::server: {}.", error);
					break;
				}
			}
			catch(std::system_error &ex)
			{
				auto eno = ex.code().value();
				if( eno == errc::bad_descriptor or eno == errc::eof or eno == errc::timed_out )
					break;
				call_on_system_error(ex.code());
			}
			context_t context(std::move(socket), parser, m_sss);
			co_await call_on_request(context);

			if( not context.response().headers_writed() )
				co_await call_on_default(context);

			if( not context.request().keep_alive() )
				break;
			time = &keepalive_time;
			if( *time == 0ms )
				break;

			parser.reset();
			socket = std::move(context.request().next_layer());
		}
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> call_on_request(context_t &context)
	{
		tk_handler_ptr handler {};
		int32_t weight = std::numeric_limits<int32_t>::max();
		size_t path_length = std::numeric_limits<size_t>::min();

		for(auto &[rule, _handler] : m_request_handler_map)
		{
			auto _path_length = context.request().path().length();
			auto _weight = context.request().path_match(rule);

			if( _weight == 0 )
			{
				handler = _handler;
				weight = _weight;
				break;
			}
			else if( _weight > 0 and (_weight < weight or (_weight == weight and _path_length > path_length)) )
			{
				handler = _handler;
				weight = _weight;
			}
		}
		if( not handler )
		{
			context.response().set_status(status::not_found);
			co_return ;
		}
		auto method = context.request().method();
		if( (handler->method & method) == 0 )
		{
			if( method == http::method::HEAD )
			{
				co_await context.response()
					.set_header(header::content_type,"text/plain")
					.write(use_awaitable);
			}
			if( method == http::method::OPTIONS )
			{
				co_await context.response()
					.set_header(header::content_type,"text/plain")
					.write(options_response_body(handler->method), use_awaitable);
			}
			else
				context.response().set_status(status::method_not_allowed);
			co_return ;
		}
		try
		{
			if( co_await handler->aop->before(context) )
				co_return ;

			co_await handler->aop->service(context);
			co_await handler->aop->after(context);
		}
		catch(const std::exception &ex)
		{
			if( handler->aop->exception(context, ex) )
				co_return ;
			call_on_exception(context, ex);
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> call_on_default(context_t &context)
	{
		try {
			if( m_default_handler )
			{
				co_await m_default_handler(context);
				if( context.response().headers_writed() )
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
			if( context.response().status() == status::ok )
				data = std::format(def_html, "Welcome to LIBGS", "");
			else
			{
				auto status = std::format (
					"<h2>{} ({})</h2>",
					status_description(context.response().status()),
					context.response().status()
				);
				data = std::format(def_html, "LIBGS", status);
			}
			if constexpr( is_char_v<CharT> )
				context.response().set_header(header::content_type, "text/html");
			else
				context.response().set_header(wheader::content_type, L"text/html");

			co_await context.response().write(data, use_awaitable);
		}
		catch(const std::exception &ex) {
			call_on_exception(context, ex);
		}
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

	void call_on_exception(context_t &context, const std::exception &ex)
	{
		context.response().set_status(status::internal_server_error);
		if( m_exception_handler )
		{
			if( m_exception_handler(context, ex) )
				return ;
		}
		throw ex;
	}

	std::string options_response_body(http::methods method)
	{
		std::string sum;
		for(int i=static_cast<int>(http::method::begin); i<=static_cast<int>(http::method::end); i<<=1)
		{
			if( method & i )
				sum += method_string(static_cast<http::method>(i)) + ";";
		}
		if( not sum.empty() )
			sum.pop_back();
		return sum;
	}

public:
	class multi_ctrlr_aop : public ctrlr_aop_t
	{
	public:
		template <typename Func, typename...AopPtrs>
		multi_ctrlr_aop(Func &&func, AopPtrs&&...aops) :
			m_aops{aop_ptr_t(std::forward<AopPtrs>(aops))...},
			m_func(std::forward<Func>(func))
		{
			assert(m_func);
		}

	public:
		[[nodiscard]] awaitable<bool> before(context_t &context) override
		{
			for(auto &aop : m_aops)
			{
				if( co_await aop->before(context) )
					co_return true;
			}
			co_return false;
		}

		[[nodiscard]] awaitable<bool> after(context_t &context) override
		{
			for(auto &aop : m_aops)
			{
				if( co_await aop->after(context) )
					co_return true;
			}
			co_return false;
		}

		[[nodiscard]] bool exception(context_t &context, const std::exception &ex) override
		{
			for(auto &aop : m_aops)
			{
				if( aop->exception(context, ex) )
					return true;
			}
			return false;
		}

	public:
		[[nodiscard]] awaitable<void> service(context_t &context) override {
			co_return co_await m_func(context);
		}

	private:
		std::vector<aop_ptr_t> m_aops {};
		request_handler_t m_func {};
	};

	struct tk_handler
	{
		explicit tk_handler(ctrlr_aop_ptr_t aop) : aop(std::move(aop)) {}

		template <method...Method>
		tk_handler &bind_method()
		{
			if constexpr( sizeof...(Method) == 0 )
				method = method::all;
			else
			{
				(void) std::initializer_list<int> {
					(method |= Method, 0) ...
				};
			}
			return *this;
		}
		methods method {};
		ctrlr_aop_ptr_t aop {};
	};
	using tk_handler_ptr = std::shared_ptr<tk_handler>;

public:
	next_layer_t m_next_layer;
	service_exec_t m_service_exec;

	std::map<string_t, tk_handler_ptr> m_request_handler_map;
	session_set m_sss;

	request_handler_t m_default_handler {};
	system_error_handler_t m_system_error_handler {};
	exception_handler_t m_exception_handler {};

	std::chrono::milliseconds m_first_reading_time {1500};
	std::chrono::milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <core_concepts::execution Exec0>
basic_server<CharT,Stream,Exec>::basic_server
(basic_acceptor_wrap<socket_t> &&next_layer, const Exec0 &service_exec) :
	m_impl(new impl(std::move(next_layer), service_exec))
{

}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec>::basic_server
(basic_acceptor_wrap<socket_t> &&next_layer, core_concepts::execution_context auto &service_exec) :
	m_impl(new impl(std::move(next_layer), service_exec.get_executor()))
{

}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec>::~basic_server()
{
	delete m_impl;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <typename Stream0, typename Exec0>
basic_server<CharT,Stream,Exec>::basic_server(basic_server<CharT,Stream0,Exec0> &&other) noexcept
	requires core_concepts::constructible<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
			 core_concepts::constructible<service_exec_t,typename Stream::executor_type> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <typename Stream0, typename Exec0>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::operator=
(basic_server<CharT,Stream0,Exec0> &&other) noexcept
	requires core_concepts::assignable<next_layer_t,asio::basic_socket_acceptor<asio::ip::tcp,executor_t>&&> and
			 core_concepts::assignable<service_exec_t,typename Stream::executor_type>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::bind(endpoint_t ep)
{
	error_code error;
	bind(std::move(ep), error);
	if( error )
		throw system_error(error, "libgs::http::basic_server::bind");
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
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

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start(size_t max)
{
	error_code error;
	start(max, error);
	if( error )
		throw system_error(error, "libgs::http::basic_server::start");
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start
(size_t max, error_code &error) noexcept
{
	m_impl->async_start(max, error);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::start
(error_code &error) noexcept
{
	return start(asio::socket_base::max_listen_connections, error);
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <http::method...method, typename Func, typename...AopPtrs>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_request
(const path_opt_token_t &path_rules, Func &&func, AopPtrs&&...aops) requires
	detail::concepts::request_handler<Func,socket_t,CharT> and
	detail::concepts::aop_ptr_list<socket_t,CharT,AopPtrs...>
{
	for(auto &path_rule : path_rules.paths)
	{
		if( path_rule.empty() )
			throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

		string_t rule(path_rule.data(), path_rule.size());
		m_impl->rule_path_check(rule);
		auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

		if( not res )
			throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

		auto aop = new typename impl::multi_ctrlr_aop(func, aops...);
		it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr_t(aop));
		it->second->template bind_method<method...>();
	}
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <http::method...method>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_request
(const path_opt_token_t &path_rules, ctrlr_aop_ptr_t ctrlr)
{
	for(auto &path_rule : path_rules.paths)
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
	}
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <http::method...method>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_request
(const path_opt_token_t &path_rules, ctrlr_aop_t *ctrlr)
{
	for(auto &path_rule : path_rules.paths)
	{
		if( path_rule.empty() )
			throw runtime_error("libgs::http::server::on_request: path_rule is empty.");

		string_t rule(path_rule.data(), path_rule.size());
		m_impl->rule_path_check(rule);
		auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);

		if( not res )
			throw runtime_error("libgs::http::server::on_request: path_rule duplication.");

		it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr_t(ctrlr));
		it->second->template bind_method<method...>();
	}
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <typename Func>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::on_default(Func &&func)
	requires detail::concepts::request_handler<Func,socket_t,CharT>
{
	m_impl->m_default_handler = std::forward<Func>(func);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::on_system_error(system_error_handler_t func)
{
	m_impl->m_system_error_handler = std::move(func);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::on_exception(exception_handler_t func)
{
	m_impl->m_exception_handler = std::move(func);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::unbound_request(string_view_t path_rule)
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::unbound_request: path_rule is empty.");
	m_impl->m_request_handler_map.erase({path_rule.data(), path_rule.size()});
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::unbound_system_error()
{
	m_impl->m_system_error_handler = {};
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::unbound_exception()
{
	m_impl->m_exception_handler = {};
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <typename Rep, typename Period>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::set_first_reading_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_first_reading_time = duration_cast<milliseconds>(d);
	if( m_impl->m_first_reading_time == 0ms )
		m_impl->m_first_reading_time = 1ms;
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
template <typename Rep, typename Period>
basic_server<CharT,Stream,Exec>&
basic_server<CharT,Stream,Exec>::set_keepalive_time(const std::chrono::duration<Rep,Period> &d)
{
	using namespace std::chrono;
	m_impl->m_keepalive_timeout = duration_cast<milliseconds>(d);
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
awaitable<void> basic_server<CharT,Stream,Exec>::co_stop() noexcept
{
	m_impl->m_is_start = false;
	co_return co_await m_impl->m_next_layer.acceptor().co_stop();
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
const typename basic_server<CharT,Stream,Exec>::executor_t&
basic_server<CharT,Stream,Exec>::get_executor() noexcept
{
	return m_impl->m_next_layer.acceptor().get_executor();
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::stop() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_next_layer.acceptor().stop();
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
basic_server<CharT,Stream,Exec> &basic_server<CharT,Stream,Exec>::cancel() noexcept
{
	m_impl->m_is_start = false;
	m_impl->m_next_layer.acceptor().cancel();
	return *this;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
const typename basic_server<CharT,Stream,Exec>::next_layer_t&
basic_server<CharT,Stream,Exec>::next_layer() const
{
	return m_impl->m_next_layer;
}

template <core_concepts::char_type CharT, concepts::stream_requires Stream, core_concepts::execution Exec>
typename basic_server<CharT,Stream,Exec>::next_layer_t&
basic_server<CharT,Stream,Exec>::next_layer()
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
