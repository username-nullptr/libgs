
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
#define LIBGS_HTTP_SERVER_DETAIL_REQUEST_H

#include <regex>

namespace libgs::http
{

template <concepts::stream Stream, core_concepts::char_type CharT>
class basic_server_request<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)
	using sock_helper_t = socket_operation_helper<next_layer_t>;

public:
	template <typename Native>
	impl(Native &&next_layer, parser_t &parser) :
		m_next_layer(std::forward<Native>(next_layer)), m_parser(&parser) {}

	template <typename Stream0>
	impl(typename basic_server_request<Stream0,char_t>::impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)), m_parser(other.m_parser) {}

	impl(impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)), m_parser(other.m_parser) {}

	template <typename Stream0>
	impl &operator=(typename basic_server_request<Stream0,char_t>::impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		m_parser = other.m_parser;
		return *this;
	}

	impl &operator=(impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		m_parser = other.m_parser;
		return *this;
	}

	~impl()
	{
		if( m_parser->version() == version::v10 )
			socket_operation_helper<next_layer_t>(m_next_layer).close();
	}

public:
	[[nodiscard]] size_t read(const mutable_buffer &buf, error_code &error) noexcept
	{
		error.assign(0, std::system_category());
		const size_t buf_size = buf.size();

		size_t sum = 0;
		if( buf_size == 0 )
			return sum;
		else if( is_eof() )
		{
			error = std::make_error_code(static_cast<std::errc>(errc::eof));
			return sum;
		}
		sock_helper_t sock_helper(m_next_layer);

		asio::socket_base::receive_buffer_size op;
		sock_helper.get_option(op, error);
		if( error )
			return sum;

		auto dst_buf = reinterpret_cast<char*>(buf.data());
		do {
			auto body = m_parser->take_partial_body(buf_size);
			sum += body.size();

			memcpy(dst_buf + sum, body.c_str(), body.size());
			if( sum == buf_size or not m_parser->can_read_from_device() )
				break;

			body = std::string(op.value(),'\0');
			for(;;)
			{
				auto tmp_sum = sock_helper.read (
					{body.data(), body.size()}, error
				);
				if( error )
					return sum;

				bool res = m_parser->append({body.data(), tmp_sum}, error);
				if( error )
					return sum;
				else if( res )
					break;
			}
		}
		while(true);
		return sum;
	}

	[[nodiscard]] awaitable<size_t> co_read(const mutable_buffer &buf, error_code &error) noexcept
	{
		using namespace std::chrono_literals;
		using namespace libgs::operators;

		error = error_code();
		const size_t buf_size = buf.size();
		size_t sum = 0;

		if( buf_size == 0 )
			co_return sum;

		else if( is_eof() )
		{
			error = std::make_error_code(static_cast<std::errc>(errc::eof));
			co_return sum;
		}
		sock_helper_t sock_helper(m_next_layer);

		asio::socket_base::receive_buffer_size op;
		sock_helper.get_option(op, error);
		if( error )
			co_return sum;

		auto read_task = [&,this]() mutable -> awaitable<size_t>
		{
			auto dst_buf = reinterpret_cast<char*>(buf.data());
			do {
				auto body = m_parser->take_partial_body(buf_size);
				sum += body.size();

				memcpy(dst_buf + sum, body.c_str(), body.size());
				if( sum == buf_size or not m_parser->can_read_from_device() )
					break;

				body = std::string(op.value(),'\0');
				for(;;)
				{
					auto tmp_sum = co_await sock_helper.read (
						{body.data(), body.size()}, use_awaitable | error
					);
					if( error )
						co_return sum;

					bool res = m_parser->append({body.data(), tmp_sum}, error);
					if( error )
						co_return sum;
					else if( res )
						break;
				}
			}
			while(true);
			co_return sum;
		};
		auto var = co_await (read_task() or sleep_for(get_executor(), 30s));
		if( var.index() == 1 and sum == 0 )
			error = make_error_code(errc::timed_out);
		co_return sum;
	}

public:
	template <typename Opt>
	[[nodiscard]] size_t save_file(Opt &&opt, error_code &error) noexcept
	{
		std::size_t sum = 0;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), error);
		if( error )
			return sum;

		constexpr size_t tcp_buf_size = 0xFFFF;
		char buf[tcp_buf_size] {0};

		using pos_t = typename Opt::pos_t;
		if( token.range->total == 0 )
		{
			while( can_read_body() )
			{
				auto size = read(buffer(buf, tcp_buf_size), error);
				if( error )
					break;

				sum += size;
				token.stream->write(buf, static_cast<pos_t>(size));
				// sleep_for(512us);
			}
		}
		else
		{
			auto total = token.range->total;
			while( can_read_body() )
			{
				auto size = read(buffer(buf, std::min(tcp_buf_size, total)), error);
				if( error )
					break;

				sum += size;
				token.stream->write(buf, static_cast<pos_t>(size));

				total -= size;
				if( total == 0 )
					break;
				// sleep_for(512us);
			}
		}
		return sum;
	}

	template <typename Opt>
	[[nodiscard]] awaitable<size_t> co_save_file(Opt &&opt, error_code &error) noexcept
	{
		std::size_t sum = 0;
		auto token = file_opt_token_helper(std::forward<Opt>(opt), error);
		if( error )
			co_return sum;

		constexpr size_t tcp_buf_size = 0xFFFF;
		char buf[tcp_buf_size] {0};

		using pos_t = typename Opt::pos_t;
		if( token.range->total == 0 )
		{
			while( can_read_body() )
			{
				auto size = co_await co_read(buffer(buf, tcp_buf_size), error);
				if( error )
					break;

				sum += size;
				token.stream->write(buf, static_cast<pos_t>(size));
				// sleep_for(512us);
			}
		}
		else
		{
			auto total = token.range->total;
			while( can_read_body() )
			{
				auto size = co_await co_read(buffer(buf, std::min(tcp_buf_size, total)), error);
				if( error )
					break;

				sum += size;
				token.stream->write(buf, static_cast<pos_t>(size));

				total -= size;
				if( total == 0 )
					break;
				// co_await sleep_for(get_executor(), 512us);
			}
		}
		co_return sum;
	}

public:
	[[nodiscard]] bool is_eof() const noexcept {
		return m_parser->is_eof();
	}
	[[nodiscard]] bool can_read_body() const noexcept {
		return not is_eof();
	}
	[[nodiscard]] executor_t get_executor() noexcept {
		return socket_operation_helper<next_layer_t>(m_next_layer).get_executor();
	}

private:
	template <typename Opt>
	[[nodiscard]] auto file_opt_token_helper(Opt &&opt, error_code &error)
	{
		if constexpr( is_char_v<Opt> or is_char_string_v<Opt> or
					  is_fstream_v<Opt> or is_ofstream_v<Opt> )
		{
			auto token = make_file_opt_token(std::forward<Opt>(opt));
			error = token.init(std::ios::out | std::ios::binary);
			if( error )
				return token;

			auto size = file_size(opt, io_permission::write);
			if( not size )
			{
				error = make_error_code(std::errc::permission_denied);
				return token;
			}
			token.stream->seekp(0, std::ios::beg);
			return token;
		}
		else
		{
			error = opt.init(std::ios::out | std::ios::binary);
			if( error )
				return std::forward<Opt>(opt);

			auto size = file_size(opt, io_permission::write);
			if( not size )
			{
				error = make_error_code(std::errc::permission_denied);
				return std::forward<Opt>(opt);
			}
			else if( not opt.range )
				opt.range = file_range(0,0);

			else if( opt.range->begin > *size or opt.range.total == 0 )
			{
				error = make_error_code(std::errc::invalid_seek);
				return std::forward<Opt>(opt);
			}
			opt.stream->seekp(opt.range->begin, std::ios::beg);
			return std::forward<Opt>(opt);
		}
	}

public:
	void set_blocking(error_code &error) {
		socket_operation_helper<next_layer_t>(m_next_layer).non_blocking(false, error);
	}

public:
	next_layer_t m_next_layer;
	parser_t *m_parser = nullptr;
};

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename NextLayer>
basic_server_request<Stream,CharT>::basic_server_request(NextLayer &&next_layer, parser_t &parser)
	requires core_concepts::constructible<next_layer_t,NextLayer&&> :
	m_impl(new impl(std::forward<NextLayer>(next_layer), parser))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_request<Stream,CharT>::~basic_server_request()
{
	delete m_impl;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_request<Stream,CharT>::basic_server_request(basic_server_request &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::operator=(basic_server_request &&other) noexcept
{
	if( this != &other )
		*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename Stream0>
basic_server_request<Stream,CharT>::basic_server_request(basic_server_request<Stream0,char_t> &&other) noexcept
	requires core_concepts::constructible<Stream,Stream0&&> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <typename Stream0>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::operator=
(basic_server_request<Stream0,char_t> &&other) noexcept requires core_concepts::assignable<Stream,Stream0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
method_t basic_server_request<Stream,CharT>::method() const noexcept
{
	return m_impl->m_parser->method();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
version_t basic_server_request<Stream,CharT>::version() const noexcept
{
	return m_impl->m_parser->version();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
std::basic_string_view<CharT> basic_server_request<Stream,CharT>::path() const noexcept
{
	return m_impl->m_parser->path();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_request<Stream,CharT>::parameters_t&
basic_server_request<Stream,CharT>::parameters() const noexcept
{
	return m_impl->m_parser->parameters();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_request<Stream,CharT>::path_args_t&
basic_server_request<Stream,CharT>::path_args() const noexcept
{
	return m_impl->m_parser->path_args();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_request<Stream,CharT>::headers_t&
basic_server_request<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_parser->headers();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_request<Stream,CharT>::cookies_t&
basic_server_request<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_parser->cookies();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::parameter
(core_concepts::basic_string_type<char_t> auto &&key) const
{
	return get_map_value(m_impl->m_parser->parameters(),
		std::forward<decltype(key)>(key)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::header
(core_concepts::basic_string_type<char_t> auto &&key) const
{
	return get_map_value(m_impl->m_parser->headers(),
		std::forward<decltype(key)>(key)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::cookie
(core_concepts::basic_string_type<char_t> auto &&key) const
{
	return get_map_value(m_impl->m_parser->cookies(),
		std::forward<decltype(key)>(key)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::parameter_or
(core_concepts::basic_string_type<char_t> auto &&key, T &&def_value) const noexcept
{
	return get_map_value_or(m_impl->m_parser->parameters(),
		std::forward<decltype(key)>(key), std::forward<T>(def_value)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::header_or
(core_concepts::basic_string_type<char_t> auto &&key, T &&def_value) const noexcept
{
	return get_map_value_or(m_impl->m_parser->headers(),
		std::forward<decltype(key)>(key), std::forward<T>(def_value)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::cookie_or
(core_concepts::basic_string_type<char_t> auto &&key, T &&def_value) const noexcept
{
	return get_map_value_or(m_impl->m_parser->cookies(),
		std::forward<decltype(key)>(key), std::forward<T>(def_value)
	);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
int32_t basic_server_request<Stream,CharT>::path_match(string_view_t rule)
{
	return m_impl->m_parser->path_match(rule);
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::path_arg
(core_concepts::basic_string_type<char_t> auto &&key) const
{
	auto &vector = m_impl->m_parser->path_args();
	for(const auto &[_key,value] : vector)
	{
		if( _key != key )
			continue;

		using def_t = std::remove_cvref_t<T>;
		if constexpr( std::is_same_v<def_t, value_t> )
			return as_const(value);
		else
			return as_const(value.template get<def_t>());
	}
	throw runtime_error (
		"libgs::http::server_request::path_arg: key '{}' not exists.",
		xxtombs(key)
	);
//	return {};
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::path_arg(size_t index) const
{
	auto &vector = m_impl->m_parser->path_args();
	if( index >= vector.size() )
		throw runtime_error("libgs::http::server_request::path_arg: Index '{}' out-of-bounds access.", index);

	auto &value = vector[index].second;
	using def_t = std::remove_cvref_t<T>;

	if constexpr( std::is_same_v<def_t, value_t> )
		return as_const(value);
	else
		return as_const(value.template get<def_t>());
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::path_arg_or
(core_concepts::basic_string_type<char_t> auto &&key, T &&def_value) const noexcept
{
	using value_t = basic_value<CharT>;
	using def_t = std::remove_cvref_t<T>;

	auto &vector = m_impl->m_parser->path_args();
	for(const auto &[_key,value] : vector)
	{
		if( _key != key )
			continue;

		if constexpr( std::is_same_v<def_t, value_t> )
			return value;
		else
			return value.template get<def_t>();
	}
	if constexpr( std::is_same_v<def_t, value_t> )
		return value_t(std::forward<T>(def_value));
	else
		return value_t(std::forward<T>(def_value)).template get<def_t>();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::basic_text_arg<CharT> T>
decltype(auto) basic_server_request<Stream,CharT>::path_arg_or(size_t index, T &&def_value) const noexcept
{
	using def_t = std::remove_cvref_t<T>;
	auto &vector = m_impl->m_parser->path_args();

	if( index >= vector.size() )
	{
		if constexpr( std::is_same_v<def_t, value_t> )
			return value_t(std::forward<T>(def_value));
		else
			return value_t(std::forward<T>(def_value)).template get<def_t>();
	}
	auto &value = vector[index].second;

	if constexpr( std::is_same_v<def_t, value_t> )
		return as_const(value);
	else
		return as_const(value.template get<def_t>());
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_request<Stream,CharT>::read(const mutable_buffer &buf, Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
	{
		m_impl->set_blocking(token);
		return token ? 0 : m_impl->read(buf, token);
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = read(buf, error);
		if( error )
			throw system_error(error, "libgs::http::server_request::read");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(),
		[this, buf, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_read(buf, error) or
				sleep_for(get_executor(), timeout)
			);
			size_t res = 0;
			if( var.index() == 0 )
				res = std::get<0>(var);
			else if( not std::get<1>(var) )
				error = make_error_code(std::errc::timed_out);

			check_error(remove_const(ntoken), error, "libgs::http::server_request::read");
			co_return res;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(), [this, buf, token]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto res = co_await m_impl->co_read(buf, error);
			check_error(remove_const(token), error, "libgs::http::server_request::read");
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_request<Stream,CharT>::read(Token &&token)
{
	using token_t = std::remove_cvref_t<Token>;
	if constexpr( std::is_same_v<token_t, error_code> )
	{
		std::string sum;
		do {
			constexpr size_t buf_size = 0xFFFF;
			char buf[buf_size] {0};

			auto res = read(buffer(buf,buf_size), token);
			sum += std::string(buf,res);

			if( token )
				break;
		}
		while( can_read_body() );
		return sum;
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto buf = read(error);
		if( error )
			throw system_error(error, "libgs::http::server_request::read");
		return buf;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(),
		[this, ntoken, timeout = get_associated_redirect_time(token)]() mutable -> awaitable<std::string>
		{
			error_code error;
			std::string sum;

			auto read_task = [&]() mutable -> awaitable<void>
			{
				do {
					constexpr size_t buf_size = 0xFFFF;
					char buf[buf_size] {0};

					auto res = co_await m_impl->co_read(buffer(buf,buf_size), error);
					sum += std::string(buf,res);
					if( error )
						break;
				}
				while( can_read_body() );
				co_return ;
			};
			auto var = co_await (
				read_task() or sleep_for(get_executor(), timeout)
			);
			if( var.index() == 1 and not std::get<1>(var) )
				error = make_error_code(std::errc::timed_out);

			check_error(remove_const(ntoken), error, "libgs::http::server_request::read");
			co_return sum;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(), [this, token]() mutable -> awaitable<std::string>
		{
			error_code error;
			std::string sum;
			do {
				constexpr size_t buf_size = 0xFFFF;
				char buf[buf_size] {0};

				auto res = co_await m_impl->co_read(buffer(buf,buf_size), error);
				sum += std::string(buf,res);
				if( error )
					break;
			}
			while( can_read_body() );
			check_error(remove_const(token), error, "libgs::http::server_request::read");
			co_return sum;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
template <core_concepts::dis_func_tf_opt_token Token>
auto basic_server_request<Stream,CharT>::save_file
(concepts::char_file_opt_token_arg<file_optype::single, io_permission::write> auto &&opt, Token &&token)
{
	using opt_t = decltype(opt);
	using token_t = std::remove_cvref_t<Token>;

	if constexpr( std::is_same_v<token_t, error_code> )
	{
		m_impl->set_blocking(token);
		return token ? 0 : m_impl->save_file(std::forward<opt_t>(opt), token);
	}
	else if constexpr( is_sync_opt_token_v<token_t> )
	{
		error_code error;
		auto res = save_file(std::forward<opt_t>(opt), error);
		if( error )
			throw system_error(error, "libgs::http::server_request::save_file");
		return res;
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if constexpr( is_yield_context_v<token_t> )
	{
		// TODO ... ...
	}
#endif //LIBGS_USING_BOOST_ASIO
	else if constexpr( is_redirect_time_v<std::remove_cvref_t<Token>> )
	{
		auto ntoken = unbound_redirect_time(token);
		return asio::co_spawn(get_executor(), [
			this, opt = std::forward<opt_t>(opt), ntoken, timeout = get_associated_redirect_time(token)
		]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto var = co_await (
				m_impl->co_save_file(std::move(opt), error) or
				sleep_for(get_executor(), timeout)
			);
			size_t res = 0;
			if( var.index() == 0 )
				res = std::get<0>(var);
			else if( not std::get<1>(var) )
				error = make_error_code(std::errc::timed_out);

			check_error(remove_const(ntoken), error, "libgs::http::server_request::save_file");
			co_return res;
		},
		ntoken);
	}
	else
	{
		return asio::co_spawn(get_executor(),
		[this, opt = std::forward<opt_t>(opt), token]() mutable -> awaitable<size_t>
		{
			error_code error;
			auto res = co_await m_impl->co_save_file(std::move(opt), error) or
			check_error(remove_const(token), error, "libgs::http::server_request::save_file");
			co_return res;
		},
		token);
	}
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_request<Stream,CharT>::keep_alive() const noexcept
{
	return m_impl->m_parser->keep_alive();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_request<Stream,CharT>::support_gzip() const noexcept
{
	return m_impl->m_parser->support_gzip();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_request<Stream,CharT>::is_chunked() const noexcept
{
	if( version() < http::version::v11 )
		return false;
	auto it = m_impl->m_headers.find(header_t::transfer_encoding);
	return it != m_impl->m_headers.end() and str_to_lower(it->second) == detail::string_pool<char_t>::chunked;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_request<Stream,CharT>::can_read_body() const noexcept
{
	return not is_eof();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
bool basic_server_request<Stream,CharT>::is_eof() const noexcept
{
	return m_impl->is_eof();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_request<Stream,CharT>::endpoint_t basic_server_request<Stream,CharT>::remote_endpoint() const
{
	return socket_operation_helper<next_layer_t>(m_impl->m_next_layer).remote_endpoint();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_request<Stream,CharT>::endpoint_t basic_server_request<Stream,CharT>::local_endpoint() const
{
	return socket_operation_helper<next_layer_t>(m_impl->m_next_layer).local_endpoint();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_request<Stream,CharT>::executor_t basic_server_request<Stream,CharT>::get_executor() noexcept
{
	return m_impl->get_executor();
}

template <concepts::stream Stream, core_concepts::char_type CharT>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::cancel() noexcept
{
	m_impl->m_next_layer.cancel();
	return *this;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
const typename basic_server_request<Stream,CharT>::next_layer_t&
basic_server_request<Stream,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <concepts::stream Stream, core_concepts::char_type CharT>
typename basic_server_request<Stream,CharT>::next_layer_t&
basic_server_request<Stream,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
