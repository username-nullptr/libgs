
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

#ifndef LIBGS_HTTP_NT_SERVER_DETAIL_SERVER_H
#define LIBGS_HTTP_NT_SERVER_DETAIL_SERVER_H

namespace libgs::http_nt
{

template <concepts::any_exec_stream Stream>
class LIBGS_HTTP_NT_TAPI basic_server<Stream>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using request_handler_t = std::function<awaitable<void>(context_t&)>;

public:
	impl(acceptor_wrap_t &&wrap, core_concepts::sched auto &&service_exec) :
		m_wrap(std::move(wrap)),
		m_service_exec(get_executor_helper (
			std::forward<decltype(service_exec)>(service_exec)
		)) {}

	explicit impl(acceptor_wrap_t &&wrap) :
		m_wrap(std::move(wrap)) {
		m_service_exec = m_wrap.accept().get_executor();
	}

public:
	void async_start(size_t max, error_code &error) noexcept {
		async_start(max, error, m_service_exec);
	}

	void async_start(const executor_t &service_exec, size_t max, error_code &error) noexcept
	{
		if( m_is_start )
			return ;
		m_wrap.acceptor().listen(static_cast<int>(max), error);
		if( error )
			return ;
		m_is_start = true;

		libgs::dispatch(m_wrap.acceptor().get_executor(),
		[self = this->shared_from_this(), service_exec]() mutable -> awaitable<void>
		{
			bool abd = false;
			try {
				co_await self->do_tcp_accept(service_exec);
			}
			catch(...) {
				abd = true;
			}
			self->m_wrap.acceptor().cancel();
			error_code _error; LIBGS_UNUSED(_error);

			self->m_wrap.acceptor().close(_error);
			self->m_is_start = false;

			if( abd )
				forced_termination();
			co_return ;
		});
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
	}

private:
	[[nodiscard]] awaitable<void> do_tcp_accept(const executor_t &service_exec)
	{
		do try {
			auto socket = co_await m_wrap.accept(service_exec);
			if( not socket_operation_helper<socket_t>(socket).is_open() )
				continue;

			libgs::dispatch(service_exec,
			[self = this->shared_from_this(), socket = std::move(socket), ktime = m_keepalive_timeout]
			() mutable -> awaitable<void>
			{
				bool abd = false;
				try {
					co_await self->do_tcp_service(socket, ktime);
				}
				catch(...) {
					abd = true;
				}
				socket_operation_helper<socket_t>(socket).close();
				if( abd )
					forced_termination();
				co_return ;
			});
		}
		catch(std::system_error &ex)
		{
			if( not m_is_start )
				break;
			call_on_server_error(ex.code());
		}
		while(true);
		co_return ;
	}

	[[nodiscard]] awaitable<void> do_tcp_service(socket_t &socket, const milliseconds &keepalive_time)
	{
		// TODO ... ...
		co_return ;
	}

private:
	void call_on_server_error(const error_code &error)
	{
		if( not m_server_error_handler or not m_server_error_handler(error) )
			system_error::loc_throw(error, "libgs::http_nt::server");
	}

	// TODO ... ...

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
		explicit tk_handler(ctrlr_aop_ptr_t aop) :
			aop(std::move(aop)) {}

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

private:
	acceptor_wrap_t m_wrap {};
	asio::any_io_executor m_service_exec {};

	server_error_handler_t m_server_error_handler {};
	service_error_handler_t m_service_error_handler {};
	request_handler_t m_default_handler {};

	std::map<std::string, tk_handler_ptr> m_request_handler_map {};
	session_manager m_session_manager {};

	milliseconds m_keepalive_timeout {5000};
	std::atomic_bool m_is_start {false};
};

template <concepts::any_exec_stream Stream>
basic_server<Stream>::basic_server(acceptor_wrap_t &&wrap, core_concepts::sched auto &&service_exec) :
	m_impl(new impl(std::move(wrap), std::forward<decltype(service_exec)>(service_exec)))
{

}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::basic_server(acceptor_wrap_t &&wrap) :
	m_impl(new impl(std::move(wrap)))
{

}

template <concepts::any_exec_stream Stream>
basic_server<Stream>::~basic_server()
{
	delete m_impl;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::bind(endpoint_wrapper_t ep)
{
	error_code error;
	bind(std::move(ep), error);
	if( error )
	{
		system_error::loc_throw(error,
			"libgs::http_nt::basic_server::bind"
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
		if( ep->address().is_v4())
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
	m_impl->async_start(service_exec, max, error);
	return *this;
}

template <concepts::any_exec_stream Stream>
basic_server<Stream> &basic_server<Stream>::start
(core_concepts::sched auto service_exec, error_code &error) noexcept
{
	return start(service_exec, asio::socket_base::max_listen_connections, error);
}

} //namespace libgs::http_nt


#endif //LIBGS_HTTP_NT_SERVER_DETAIL_SERVER_H
