
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SERVER_H
#define LIBGS_HTTP_SERVER_DETAIL_SERVER_H

namespace libgs::http
{

template <concepts::any_exec_stream Stream>
class LIBGS_HTTP_TAPI basic_server<Stream>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using request_handler_t = std::function<awaitable<void>(context_t&)>;
	using connection_ptr = std::shared_ptr<connection_t>;

public:
	impl(acceptor_wrap_t &&wrap, core_concepts::sched auto &&service_exec) :
		m_wrap(std::move(wrap)),
		m_service_exec(get_executor_helper (
			std::forward<decltype(service_exec)>(service_exec)
		))
	{
		bind_session_error_handler();
	}

	explicit impl(acceptor_wrap_t &&wrap) :
		m_wrap(std::move(wrap))
	{
		m_service_exec = m_wrap.acceptor().get_executor();
		bind_session_error_handler();
	}

public:
	void async_start(size_t max, error_code &error) noexcept
	{
		async_start(m_service_exec, max, error);
	}

	void async_start(const executor_t &service_exec, size_t max, error_code &error) noexcept
	{
		error.clear();
		if( m_is_start )
			return ;

		m_wrap.acceptor().listen(static_cast<int>(max), error);
		if( error )
			return ;
		m_is_start = true;

		libgs::dispatch(m_wrap.acceptor().get_executor(),
		[self = this->shared_from_this(), service_exec]() mutable
		{
			bool abd = false;
			try {
				self->do_tcp_accept(service_exec);
			}
			catch(...) {
				abd = true;
			}
			if( abd )
			{
				error_code ignored {};
				self->m_wrap.acceptor().cancel(ignored);
				self->m_wrap.acceptor().close(ignored);
				self->m_is_start = false;
				forced_termination();
			}
		});
	}

	void bind_session_error_handler()
	{
		m_session_manager.on_error (
		[this](const session_ptr&, const error_code &error)
		{
			call_on_server_error(error);
		});
	}

	void set_config(config_t config) noexcept
	{
		using namespace std::chrono_literals;
		if( config.first_reading_time <= 0ms )
			config.first_reading_time = 1ms;

		if( config.keepalive_time < 0ms )
			config.keepalive_time = 0ms;

		if constexpr( requires { config.tls_handshake_timeout; } )
		{
			if( config.tls_handshake_timeout <= 0ms )
				config.tls_handshake_timeout = 1ms;
		}
		m_config = std::move(config);
	}

	[[nodiscard]] config_t config() const noexcept
	{
		return m_config;
	}

	void rule_path_check(std::string &str)
	{
		auto n_it = std::unique(str.begin(), str.end(), [](char c0, char c1){
			return c0 == c1 and c0 == 0x2F/*/*/;
		});
		if( n_it != str.end() )
			str.erase(n_it, str.end());

		if( not str.starts_with('/') )
			str = "/" + str;

		if( str.size() > 1 and str.ends_with('/') )
			str.pop_back();
	}

private:
	void do_tcp_accept(const executor_t &service_exec)
	{
		auto callback =
		[server_self = this->shared_from_this(), service_exec](connection_ptr accepted_connection) mutable
		{
			if( not accepted_connection )
			{
				if( not server_self->m_is_start )
				{
					error_code ignored {};
					server_self->m_wrap.acceptor().cancel(ignored);
					server_self->m_wrap.acceptor().close(ignored);
				}
				return ;
			}
			if( not server_self->m_is_start or not accepted_connection->is_open() )
			{
				ignore_unused(accepted_connection->close());
				return ;
			}
			tcp_socket_options options {};
			options.no_delay = true;

			auto set_result = accepted_connection->set_options(options);
			if( not set_result )
			{
				ignore_unused(accepted_connection->close());
				server_self->call_on_server_error(set_result.error());
				return ;
			}
			auto config = server_self->m_config;
			libgs::dispatch(service_exec,
			[server = std::move(server_self), client_connection = std::move(accepted_connection), config]
			() mutable -> awaitable<void>
			{
				bool abd = false;
				bool released = false;
				try {
					released = co_await server->do_tcp_service (
						client_connection, config.first_reading_time,
						config.keepalive_time, config.resource_root
					);
				}
				catch(...) {
					abd = true;
				}
				if( not released )
					ignore_unused(client_connection->close());
				if( abd )
					forced_termination();
				co_return ;
			});
		};

		if constexpr( requires { m_config.tls_handshake_timeout; } )
		{
			m_wrap.accept(service_exec, std::move(callback),
				m_config.tls_handshake_timeout
			);
		}
		else
			m_wrap.accept(service_exec, std::move(callback));
	}

	[[nodiscard]] awaitable<bool> do_tcp_service
	(const connection_ptr &client_connection, milliseconds first_reading_time,
		milliseconds keepalive_time, std::filesystem::path resource_root)
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		const auto *time = &first_reading_time;
		std::string pending_data {};
		for(;;)
		{
			server_parser parser {};
			if( not pending_data.empty() )
			{
				auto expected = parser.append(buffer(pending_data));
				if( not expected )
				{
					call_on_server_error(expected.error());
					break;
				}
			}
			context_t context (
				client_connection, std::move(parser), m_session_manager,
				resource_root
			);
			try {
				co_await context.request().wait(use_awaitable | *time);
			}
			catch(std::system_error &ex)
			{
				auto eno = ex.code().value();
				if( eno == errc::bad_descriptor or eno == errc::eof or
					eno == errc::timed_out )
					break;
				call_on_server_error(ex.code());
				break;
			}
			context.response().auto_set(context.request());
			if( auto expectation = context.request().header(header::expect); expectation )
			{
				auto value = strtls::to_lower(strtls::trimmed(**expectation));
				if( context.request().version() < version::v11 or value != "100-continue" )
				{
					context.response()
						.set_status(status::expectation_failed)
						.set_header(header::connection, "close");

					co_await context.response().write({nullptr,0}, use_awaitable);
					break;
				}
			}
			co_await call_on_request(context);

			if( not context.response().is_finished() )
				co_await call_on_default(context);

			if( context.connection_handed_over() )
				co_return true;

			if( not context.request().keep_alive() )
				break;

			if( context.request().can_read_body() )
			{
				if( context.request().header(header::expect) )
					break;
				try
				{
					ignore_unused(
						co_await context.request().read(use_awaitable)
					);
				}
				catch(const std::system_error &ex)
				{
					call_on_server_error(ex.code());
					break;
				}
			}
			pending_data = context.request().take_pending_data();

			time = &keepalive_time;
			if( *time == 0ms )
				break;
		}
		co_return false;
	}

private:
	[[nodiscard]] awaitable<void> call_on_request(context_t &context)
	{
		tk_handler_ptr handler {};
		const std::string *selected_rule = nullptr;

		int32_t weight = std::numeric_limits<int32_t>::max();
		size_t path_length = std::numeric_limits<size_t>::min();

		for(auto &[rule, _handler] : m_request_handler_map)
		{
			auto _path_length = rule.length();
			auto _weight = context.request().path_match(rule);

			if( _weight == 0 )
			{
				handler = _handler;
				selected_rule = &rule;
				weight = _weight;
				break;
			}
			else if( _weight > 0 and (_weight < weight or (_weight == weight and _path_length > path_length)) )
			{
				handler = _handler;
				selected_rule = &rule;
				weight = _weight;
				path_length = _path_length;
			}
		}
		if( not handler )
		{
			context.response().set_status(status::not_found);
			co_return ;
		}
		ignore_unused(context.request().path_match(*selected_rule));
		auto method = context.request().method();

		if( not ( handler->method & method ) )
		{
			if( method == method::head )
			{
				co_await context.response()
					.set_header(header::content_type,"text/plain")
					.write(use_awaitable);
			}
			else if( method == method::options )
			{
				auto body = options_response_body(handler->method);
				co_await context.response()
					.set_header(header::content_type,"text/plain")
					.write(body, use_awaitable);
			}
			else
			{
				context.response().set_status (
					status::method_not_allowed
				);
			}
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
			call_on_service_error(context, ex);
		}
		co_return ;
	}

	static constexpr auto def_html_v =
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

	[[nodiscard]] awaitable<void> call_on_default(context_t &context)
	{
		try {
			if( m_default_handler )
			{
				co_await m_default_handler(context);
				if( context.response().is_finished() )
					co_return ;
			}
			std::string data {};
			if( context.response().status() == status::ok )
				data = std::format(def_html_v, "Welcome to LIBGS", "");
			else
			{
				auto status = std::format (
					"<h2>{} ({})</h2>",
					status::description(context.response().status()),
					context.response().status()
				);
				data = std::format(def_html_v, "LIBGS", status);
			}
			co_await context.response()
				.set_header(header::content_type, "text/html")
				.write(data, use_awaitable);
		}
		catch(const std::exception &ex) {
			call_on_service_error(context, ex);
		}
		co_return ;
	}

private:
	void call_on_server_error(const error_code &error)
	{
		if( not m_server_error_handler or not m_server_error_handler(error) )
			system_error::loc_throw(error, "libgs::http::server");
	}

	void call_on_service_error(context_t &context, const std::exception &ex)
	{
		context.response().set_status(status::internal_server_error);
		if( m_service_error_handler and m_service_error_handler(context, ex) )
			return ;
		throw ;
	}

	[[nodiscard]] static std::string options_response_body(methods method)
	{
		std::string sum {};
		for(uint16_t i=method::get; i<=method::connect; i<<=1)
		{
			if( not ( method & i ) )
				continue;

			sum += std::format("{};", method::string (
				static_cast<method_enum>(i)
			));
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
		explicit multi_ctrlr_aop(Func &&func, AopPtrs&&...aops) :
			m_aops{aop_ptr_t(std::forward<AopPtrs>(aops))...},
			m_func(std::forward<Func>(func))
		{
			assert(m_func);
		}

	public:
		[[nodiscard]] awaitable<bool> before(context_t &context) override
		{
			for(auto &interceptor : m_aops)
			{
				if( co_await interceptor->before(context) )
					co_return true;
			}
			co_return false;
		}

		[[nodiscard]] awaitable<bool> after(context_t &context) override
		{
			for(auto &interceptor : m_aops)
			{
				if( co_await interceptor->after(context) )
					co_return true;
			}
			co_return false;
		}

		[[nodiscard]] bool exception(context_t &context, const std::exception &ex) override
		{
			for(auto &interceptor : m_aops)
			{
				if( interceptor->exception(context, ex) )
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
		explicit tk_handler(ctrlr_aop_ptr_t handler_aop) :
			aop(std::move(handler_aop)) {}

		template <method_enum...Method>
		tk_handler &bind_method()
		{
			if constexpr( sizeof...(Method) == 0 )
			{
#define X_MACRO(e,v,d) method |= method_enum::e;
				LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
			}
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
	acceptor_wrap_t m_wrap {};
	asio::any_io_executor m_service_exec {};

	server_error_handler_t m_server_error_handler {};
	service_error_handler_t m_service_error_handler {};
	request_handler_t m_default_handler {};

	std::map<std::string, tk_handler_ptr> m_request_handler_map {};
	session_manager m_session_manager {};

	config_t m_config {};
	std::atomic_bool m_is_start {false};
};

template <concepts::any_exec_stream Stream>
basic_server<Stream>::basic_server(acceptor_wrap_t &&wrap, core_concepts::sched auto &&service_exec) :
	m_impl(std::make_shared<impl>(std::move(wrap), std::forward<decltype(service_exec)>(service_exec)))
{

}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::basic_server(acceptor_wrap_t &&wrap) :
	m_impl(std::make_shared<impl>(std::move(wrap)))
{

}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::~basic_server() = default;

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::bind(endpoint_wrapper_t ep)
{
	error_code error;
	bind(std::move(ep), error);
	if( error )
	{
		system_error::loc_throw(error,
			"libgs::http::basic_server::bind"
		);
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::bind(endpoint_wrapper_t ep, error_code &error) noexcept
{
	auto &acceptor = m_impl->m_wrap.acceptor();
	if( not acceptor.is_open() )
	{
		if( ep->address().is_v4() )
			acceptor.open(asio::ip::tcp::v4(), error);
		else
			acceptor.open(asio::ip::tcp::v6(), error);
		if( error )
			return *this;
	}
	acceptor.set_option(asio::socket_base::reuse_address(true), error);
	if( error )
		return *this;

	acceptor.bind(std::move(*ep), error);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start(size_t max)
{
	error_code error;
	start(max, error);
	if( error )
	{
		system_error::loc_throw(error,
			"libgs::http::basic_server::start"
		);
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start(size_t max, error_code &error) noexcept
{
	m_impl->async_start(max, error);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start(error_code &error) noexcept
{
	return start(asio::socket_base::max_listen_connections, error);
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start
(core_concepts::sched auto &&service_exec, size_t max)
{
	error_code error;
	start(service_exec, max, error);
	if( error )
	{
		system_error::loc_throw(error,
			"libgs::http::basic_server::start"
		);
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start
(core_concepts::sched auto &&service_exec, size_t max, error_code &error) noexcept
{
	m_impl->async_start (
		get_executor_helper(std::forward<decltype(service_exec)>(service_exec)),
		max, error
	);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start
(core_concepts::sched auto service_exec, error_code &error) noexcept
{
	return start(service_exec, asio::socket_base::max_listen_connections, error);
}

template <concepts::any_exec_stream Stream>
template <method_enum...Method, typename Func, typename...AopPtrs>
basic_server<Stream> &basic_server<Stream>::on_request
(const path_opt_token_t &path_rules, Func &&func, AopPtrs&&...aops) requires
	concepts::request_handler<Func,executor_t> and
	concepts::aop_ptr_list<executor_t,AopPtrs...>
{
	for(auto &path_rule : path_rules.paths)
	{
		if( path_rule.empty() )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule is empty."
			);
		}
		std::string rule(path_rule.data(), path_rule.size());
		m_impl->rule_path_check(rule);

		auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);
		if( not res )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule duplication."
			);
		}
		auto controller_aop = new impl::multi_ctrlr_aop(func, aops...);
		it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr_t(controller_aop));
		it->second->template bind_method<Method...>();
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
template <method_enum...Method>
basic_server<Stream> &basic_server<Stream>::on_request
(const path_opt_token_t &path_rules, ctrlr_aop_ptr_t ctrlr)
{
	for(auto &path_rule : path_rules.paths)
	{
		if( path_rule.empty() )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule is empty."
			);
		}
		std::string rule(path_rule.data(), path_rule.size());
		m_impl->rule_path_check(rule);

		auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);
		if( not res )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule duplication."
			);
		}
		it->second = std::make_shared<typename impl::tk_handler>(std::move(ctrlr));
		it->second->template bind_method<Method...>();
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
template <method_enum...Method>
basic_server<Stream> &basic_server<Stream>::on_request
(const path_opt_token_t &path_rules, ctrlr_aop_t *ctrlr)
{
	for(auto &path_rule : path_rules.paths)
	{
		if( path_rule.empty() )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule is empty."
			);
		}
		std::string rule(path_rule.data(), path_rule.size());
		m_impl->rule_path_check(rule);

		auto [it, res] = m_impl->m_request_handler_map.emplace(rule, nullptr);
		if( not res )
		{
			runtime_error::loc_throw (
				"libgs::http::server::on_request: path_rule duplication."
			);
		}
		it->second = std::make_shared<typename impl::tk_handler>(ctrlr_aop_ptr_t(ctrlr));
		it->second->template bind_method<Method...>();
	}
	return *this;
}

template <concepts::any_exec_stream Stream>
template <typename Func>
basic_server<Stream> &basic_server<Stream>::on_default(Func &&func) requires
	concepts::request_handler<Func,executor_t>
{
	m_impl->m_default_handler = std::forward<Func>(func);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::on_server_error(server_error_handler_t func)
{
	m_impl->m_server_error_handler = std::move(func);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::on_service_error(service_error_handler_t func)
{
	m_impl->m_service_error_handler = std::move(func);
	return *this;
}

template <concepts::any_exec_stream Stream>
template <core_concepts::text_p<char> Text>
basic_server<Stream> &basic_server<Stream>::unbound_request(const Text &path_rule)
{
	auto rule = strtls::to_string(path_rule);
	if( rule.empty() )
	{
		runtime_error::loc_throw (
			"libgs::http::server::unbound_request: path_rule is empty."
		);
	}
	m_impl->rule_path_check(rule);
	m_impl->m_request_handler_map.erase(rule);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::unbound_server_error()
{
	m_impl->m_server_error_handler = {};
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::unbound_service_error()
{
	m_impl->m_service_error_handler = {};
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::set_config(const config_t &config)
{
	m_impl->set_config(config);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::config_t basic_server<Stream>::config() const noexcept
{
	return m_impl->config();
}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::executor_t basic_server<Stream>::get_executor() noexcept
{
	return m_impl->m_wrap.acceptor().get_executor();
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::cancel() noexcept
{
	return stop();
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::stop() noexcept
{
	m_impl->m_is_start = false;
	error_code ignored {};
	m_impl->m_wrap.acceptor().cancel(ignored);
	return *this;
}

template <concepts::any_exec_stream Stream>
const basic_server<Stream>::acceptor_wrap_t &basic_server<Stream>::acceptor_wrap() const
{
	return m_impl->m_wrap;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::acceptor_wrap_t &basic_server<Stream>::acceptor_wrap()
{
	return m_impl->m_wrap;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_SERVER_H
