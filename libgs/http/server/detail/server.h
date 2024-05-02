
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
			try { co_await do_tcp_accept(max); }
			catch(...) {}

			co_await m_tcp_server->co_cancel();
			m_is_start = false;
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
		for(;;)
		{
			auto socket = co_await m_tcp_server->accept();
			auto ktime = m_keepalive_timeout;

			libgs::co_spawn_detached([this, socket = std::move(socket), ktime = std::move(ktime)]() -> awaitable<void>
			{
				try { co_await do_tcp_service(std::move(socket), ktime); }
				catch(...) {}

				error_code error;
				socket->close(error);
			},
			m_tcp_server->pool());
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(io::tcp_socket_ptr socket, const std::chrono::milliseconds &keepalive_time)
	{
		basic_server::parser parser;
		error_code error;

		char buf[65536] = {0};
		for(;;)
		{
			auto size = co_await socket->read({buf, 65536}, keepalive_time);
			if( size == 0 )
				break;

			bool parse_res = parser.append({buf, size}, error);
			if( error )
			{
				if( not m_is_start )
					break;
				co_await call_on_error(error);
				continue;
			}
			else if( not parse_res )
				continue;

			auto req = std::make_shared<basic_server::request>(socket, parser);
			basic_server::response resp(req);

			co_await call_on_request(parser, *req, resp);
			if( not resp.headers_writed() )
				co_await call_on_default(*req, resp);
			parser.reset();
		}
		co_return ;
	}

private:
	[[nodiscard]] awaitable<bool> call_on_request(basic_server::parser &parser, basic_server::request &req, basic_server::response &resp)
	{
		for(auto &[rule, handler] : m_request_handler_map)
		{
			if( not wildcard_match(rule, parser.path()) )
				continue;
			else if( (handler->method & parser.method()) == 0 )
			{
				resp.set_status(status::method_not_allowed);
				co_return false;
			}
			else if( handler->aop.index() == 0 )
				co_await for_aop(std::get<0>(handler->aop), req, resp);
			else // index == 1
				co_await ctrlr_aop(std::get<1>(handler->aop), req, resp);
			co_return true;
		}
		resp.set_status(status::not_found);
		co_return false;
	}

	[[nodiscard]] awaitable<void> call_on_default(basic_server::request &req, basic_server::response &resp)
	{
		if( m_default_handler )
		{
			co_await m_default_handler(req, resp);
			if( resp.headers_writed() )
				co_return ;
		}
		co_await resp.write();
		co_return ;
	}

	[[nodiscard]] awaitable<void> call_on_error(const error_code &error)
	{
		if( m_error_callback )
			co_await m_error_callback(error);
		else
			throw system_error(error, "libgs::http::server::impl::do_tcp_service");
		co_return ;
	}

private:
	[[nodiscard]] awaitable<void> for_aop(aop_token &tk, basic_server::request &req, basic_server::response &resp)
	{
		for(auto &aop : tk.aops)
		{
			if( co_await aop->before(req, resp) )
				co_return ;
		}
		if( tk.callback )
			co_await tk.callback(req, resp);
		for(auto &aop : tk.aops)
		{
			if( co_await aop->after(req, resp) )
				break;
		}
		co_return ;
	}

	[[nodiscard]] awaitable<void> ctrlr_aop(ctrlr_aop_ptr &ctrlr, basic_server::request &req, basic_server::response &resp)
	{
		if( co_await ctrlr->before(req, resp) )
			co_return ;

		co_await ctrlr->service(req, resp);
		co_await ctrlr->after(req, resp);
		co_return ;
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

	struct greater_case_insensitive
	{
		bool operator()(const str_type &s1, const str_type &s2) const {
			return s1 > s2;
		}
	};

public:
	tcp_server_ptr m_tcp_server {};
	std::map<str_type, tk_handler_ptr, greater_case_insensitive> m_request_handler_map;

	request_handler m_default_handler {};
	error_handler m_error_callback {};

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
	concept_request_handler<Func,CharT,Exec> and concept_aop_ptr_list<CharT,Exec,AopPtr...> :
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
	requires concept_request_handler<Func,CharT,Exec>
{
	m_impl->m_default_handler = std::forward<Func>(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::on_error(error_handler callback) noexcept
{
	m_impl->m_error_callback = std::move(callback);
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_request(str_view_type path_rule) noexcept
{
	if( path_rule.empty() )
		throw runtime_error("libgs::http::server::unbound_request: path_rule is empty.");
	m_impl->m_request_handler_map.erase({path_rule.data(), path_rule.size()});
	return *this;
}

template <concept_char_type CharT, concept_execution Exec>
basic_server<CharT,Exec> &basic_server<CharT,Exec>::unbound_error() noexcept
{
	m_impl->m_error_callback = {};
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
