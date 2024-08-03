
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

#include <libgs/http/basic/socket_operation_helper.h>
#include <libgs/core/string_list.h>
#include <libgs/core/app_utls.h>
#include <filesystem>
#include <fstream>

namespace libgs::http
{

template <concept_tcp_stream Stream, concept_char_type CharT>
class basic_server_request<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	template <typename Native>
	impl(Native &&next_layer, parser_t &parser) :
		m_next_layer(std::forward<Native>(next_layer)), m_parser(&parser) {}

	template <typename Stream0>
	impl(typename basic_server_request<Stream0,CharT>::impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)), m_parser(other.m_parser) {}

	impl(impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)), m_parser(other.m_parser) {}

	template <typename Stream0>
	impl &operator=(typename basic_server_request<Stream0,CharT>::impl &&other) noexcept
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

	~impl() {
		socket_operation_helper<next_layer_t>::close(m_next_layer);
	}

public:
	template <typename Token>
	void token_reset(Token &token)
	{
		if constexpr( is_function_v<Token> )
			return ;
		else if constexpr( detail::concept_has_ec_<Token> )
			token.ec_ = error_code();
		else if constexpr( detail::concept_has_get<Token> )
		{
			if constexpr( detail::concept_has_ec_<decltype(token.get())> )
				token.get().ec_ = error_code();
		}
	}

	template <typename Token>
	void token_handle(Token &token, const error_code &error, const char *ehead)
	{
		if constexpr( is_function_v<Token> )
			return ;
		else if constexpr( detail::concept_has_ec_<Token> )
		{
			token.ec_ = error;
			return ;
		}
		else if constexpr( detail::concept_has_get<Token> )
		{
			if constexpr( detail::concept_has_ec_<decltype(token.get())> )
			{
				token.get().ec_ = error;
				return ;
			}
		}
		throw system_error(error, ehead);
	}

public:
	next_layer_t m_next_layer;
	parser_t *m_parser;
};

template <concept_tcp_stream Stream, concept_char_type CharT>
template <typename NextLayer>
basic_server_request<Stream,CharT>::basic_server_request(NextLayer &&next_layer, parser_t &parser)
	requires concept_constructible<next_layer_t,NextLayer&&> :
	m_impl(new impl(std::move(next_layer), parser))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_request<Stream,CharT>::~basic_server_request()
{
	delete m_impl;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_request<Stream,CharT>::basic_server_request(basic_server_request &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::operator=(basic_server_request &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <typename Stream0>
basic_server_request<Stream,CharT>::basic_server_request(basic_server_request<Stream0,CharT> &&other) noexcept
	requires concept_constructible<Stream,Stream0&&> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <typename Stream0>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::operator=
(basic_server_request<Stream0,CharT> &&other) noexcept requires concept_assignable<Stream,Stream0&&>
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
http::method basic_server_request<Stream,CharT>::method() const noexcept
{
	return m_impl->m_parser->method();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
std::basic_string_view<CharT> basic_server_request<Stream,CharT>::version() const noexcept
{
	return m_impl->m_parser->version();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
std::basic_string_view<CharT> basic_server_request<Stream,CharT>::path() const noexcept
{
	return m_impl->m_parser->path();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::parameters_t&
basic_server_request<Stream,CharT>::parameters() const noexcept
{
	return m_impl->m_parser->parameters();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::path_args_t&
basic_server_request<Stream,CharT>::path_args() const noexcept
{
	return m_impl->m_parser->path_args();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::headers_t&
basic_server_request<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_parser->headers();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::cookies_t&
basic_server_request<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_parser->cookies();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::value_t&
basic_server_request<Stream,CharT>::parameter(string_view_t key) const
{
	auto &map = m_impl->m_parser->parameters();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::server_request::parameter: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::value_t&
basic_server_request<Stream,CharT>::header(string_view_t key) const
{
	auto &map = m_impl->m_parser->headers();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::server_request::header: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::value_t&
basic_server_request<Stream,CharT>::cookie(string_view_t key) const
{
	auto &map = m_impl->m_parser->cookies();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::server_request::cookie: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::parameter_or(string_view_t key, value_t def_value) const noexcept
{
	auto &map = m_impl->m_parser->parameters();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::header_or(string_view_t key, value_t def_value) const noexcept
{
	auto &map = m_impl->m_parser->headers();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::cookie_or(string_view_t key, value_t def_value) const noexcept
{
	auto &map = m_impl->m_parser->cookies();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::path_arg(size_t index) const
{
	auto &vector = m_impl->m_parser->path_args();
	if( index >= vector.size() )
		throw runtime_error("libgs::http::server_request::path_arg: Index '{}' out-of-bounds access.", index);
	return vector[index].second;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::path_arg(string_view_t key) const
{
	auto &vector = m_impl->m_parser->path_args();
	for(const auto &[_key,value] : vector)
	{
		if( _key == key )
			return value;
	}
	throw runtime_error("libgs::http::server_request::path_arg: key '{}' not exists.", xxtombs<CharT>(key));
//	return {};
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::value_t
basic_server_request<Stream,CharT>::path_arg_or(string_view_t key, value_t def_value) const noexcept
{
	auto &vector = m_impl->m_parser->path_args();
	for(const auto &[_key,value] : vector)
	{
		if( _key == key )
			return value;
	}
	return def_value;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
int32_t basic_server_request<Stream,CharT>::path_match(string_view_t rule)
{
	return m_impl->m_parser->path_match(rule);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::read(const mutable_buffer &buf)
{
	error_code error;
	auto res = read(buf, error);
	if( error )
		throw system_error(error, "libgs::http::server_request::read");
	return res;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::read(const mutable_buffer &buf, error_code &error) noexcept
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
	asio::socket_base::receive_buffer_size op;
	socket_operation_helper<next_layer_t>::get_option(m_impl->m_next_layer, op, error);
	if( error )
		return sum;

	auto dst_buf = reinterpret_cast<char*>(buf.data());
	do {
		auto body = m_impl->m_parser->take_partial_body(buf_size);
		sum += body.size();

		memcpy(dst_buf + sum, body.c_str(), body.size());
		if( sum == buf_size or not m_impl->m_parser->can_read_from_device() )
			break;

		body = std::string(op.value(),'\0');
		auto tmp_buf = const_cast<char*>(body.c_str());
		size_t tmp_sum = 0;
		do {
			tmp_sum += m_impl->m_next_layer.read_some(buffer(tmp_buf + tmp_sum, body.size() - tmp_sum), error);
			sum += tmp_sum;
			if( error and error.value() == errc::interrupted )
				continue;
		}
		while(false);
		if( tmp_sum == 0 or not m_impl->m_parser->append({body.c_str(), tmp_sum}, error) )
			break;
	}
	while(true);
	return sum;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_request<Stream,CharT>::async_read(const mutable_buffer &buf, Token &&token)
{
	if constexpr( is_function_v<Token> )
	{
		co_spawn_detached([this, buf, callback = std::move(token)]() -> awaitable<void>
		{
			error_code error;
			auto size = co_await async_read(buf, use_awaitable|error);
			callback(size, error);
			co_return ;
		},
		get_executor());
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if( std::is_same_v<std::decay_t<Token>, yield_context> )
	{
		error_code error;
		size_t sum = 0;

		if( token.ec_ )
			*token.ec_ = error;
		do {
			error.assign(0, std::system_category());
			const size_t buf_size = buf.size();

			if( buf_size == 0 )
				break;
			else if( is_eof() )
			{
				error = std::make_error_code(static_cast<std::errc>(errc::eof));
				break;
			}
			asio::socket_base::receive_buffer_size op;
			socket_operation_helper<next_layer_t>::get_option(m_impl->m_next_layer, op, error);
			if( error )
				break;

			auto dst_buf = reinterpret_cast<char*>(buf.data());
			do {
				auto body = m_impl->m_parser->take_partial_body(buf_size);
				sum += body.size();

				memcpy(dst_buf + sum, body.c_str(), body.size());
				if( sum == buf_size or not m_impl->m_parser->can_read_body() )
					break;

				body = std::string(op.value(),'\0');
				auto tmp_buf = const_cast<char*>(body.c_str());
				size_t tmp_sum = 0;
				do {
					tmp_sum += m_impl->m_next_layer.async_read_some
							(buffer(tmp_buf + tmp_sum, body.size() - tmp_sum), token[error]);
					sum += tmp_sum;
					if( error and error.value() == errc::interrupted )
						continue;
				}
				while(false);
				if( tmp_sum == 0 or not m_impl->m_parser->append({body.c_str(), tmp_sum}, error) )
					break;
			}
			while(true);
		}
		while(false);

		if( not error )
			return sum;
		else if( token.ec_ )
			*token.ec_ = error;
		else
			throw system_error(error, "libgs::http::server_request::async_read");
		return sum;
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		return [](basic_server_request *self, mutable_buffer buf, Token token) mutable -> awaitable<size_t>
		{
			self->m_impl->token_reset(token);
			const size_t buf_size = buf.size();
			error_code error;
			size_t sum = 0;
			do {
				if( buf_size == 0 )
					co_return sum;
				else if( self->is_eof() )
				{
					error = std::make_error_code(static_cast<std::errc>(errc::eof));
					break;
				}
				asio::socket_base::receive_buffer_size op;
				socket_operation_helper<next_layer_t>::get_option(self->m_impl->m_next_layer, op, error);
				if( error )
					break;

				auto dst_buf = reinterpret_cast<char*>(buf.data());
				do {
					auto body = self->m_impl->m_parser->take_partial_body(buf_size);
					sum += body.size();

					memcpy(dst_buf + sum, body.c_str(), body.size());
					if( sum == buf_size or not self->m_impl->m_parser->can_read_from_device() )
						break;

					body = std::string(op.value(),'\0');
					auto tmp_buf = const_cast<char*>(body.c_str());
					size_t tmp_sum = 0;
					do {
						auto read_buf = buffer(tmp_buf + tmp_sum, body.size() - tmp_sum);
						if constexpr( detail::concept_has_get<Token> )
						{
							tmp_sum += co_await self->m_impl->m_next_layer.async_read_some
									(read_buf, use_awaitable|error|token.get_cancellation_slot());
						}
						else
						{
							tmp_sum += co_await self->m_impl->m_next_layer.async_read_some
									(read_buf, use_awaitable|error);
						}
						sum += tmp_sum;
						if( error and error.value() == errc::interrupted )
							continue;
					}
					while(false);
					if( tmp_sum == 0 or not self->m_impl->m_parser->append({body.c_str(), tmp_sum}, error) )
						break;
				}
				while(true);
			}
			while(false);
			self->m_impl->token_handle(token, error, "libgs::http::server_request::async_read");
			co_return sum;
		}
		(this, buf, token);
	}
}

template <concept_tcp_stream Stream, concept_char_type CharT>
std::string basic_server_request<Stream,CharT>::read_all()
{
	error_code error;
	auto buf = read_all(error);
	if( error )
		throw system_error(error, "libgs::http::server_request::read_all");
	return buf;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
std::string basic_server_request<Stream,CharT>::read_all(error_code &error) noexcept
{
	std::string sum;
	do {
		constexpr size_t buf_size = 0xFFFF;
		char buf[buf_size] {0};

		auto res = read(buffer(buf,buf_size), error);
		sum += std::string(buf,res);

		if( error )
			break;
	}
	while( can_read_body() );
	return sum;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <asio::completion_token_for<void(std::string_view,error_code)> Token>
auto basic_server_request<Stream,CharT>::async_read_all(Token &&token)
{
	if constexpr( is_function_v<Token> )
	{
		co_spawn_detached([this, callback = std::move(token)]() -> awaitable<void>
		{
			error_code error;
			auto buf = co_await async_read_all(use_awaitable|error);
			callback(buf, error);
			co_return ;
		},
		get_executor());
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if( std::is_same_v<std::decay_t<Token>, yield_context> )
	{
		error_code error;
		std::string sum;

		if( token.ec_ )
			*token.ec_ = error;
		do {
			constexpr size_t buf_size = 0xFFFF;
			char buf[buf_size] {0};

			auto res = async_read(buffer(buf,buf_size), token[error]);
			sum += std::string(buf,res);
			if( error )
				break;
		}
		while( can_read_body() );

		if( not error )
			return sum;
		else if( token.ec_ )
			*token.ec_ = error;
		else
			throw system_error(error, "libgs::http::server_request::async_read_all");
		return sum;
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		return [](basic_server_request *self, Token token) mutable -> awaitable<std::string>
		{
			std::string sum;
			error_code error;
			do {
				constexpr size_t buf_size = 0xFFFF;
				char buf[buf_size] {0};

				size_t res = 0;
				if constexpr( detail::concept_has_get<Token> )
					res = co_await self->async_read(buffer(buf,buf_size), use_awaitable|error|token.get_cancellation_slot());
				else
					res = co_await self->async_read(buffer(buf,buf_size), use_awaitable|error);

				sum += std::string(buf,res);
				if( error )
					break;
			}
			while( self->can_read_body() );
			self->m_impl->token_handle(token, error, "libgs::http::server_request::async_read_all");
			co_return sum;
		}
		(this, token);
	}
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::save_file(std::string_view file_name, const req_range &range)
{
	error_code error;
	auto buf = save_file(file_name, range, error);
	if( error )
		throw system_error(error, "libgs::http::server_request::save_file");
	return buf;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::save_file(std::string_view file_name, const req_range &range, error_code &error) noexcept
{
	std::size_t sum = 0;
	if( file_name.empty() )
	{
		error = std::make_error_code(std::errc::invalid_argument);
		return sum;
	}
	auto _file_name = app::absolute_path(file_name);
	using namespace std::chrono_literals;
	namespace fs = std::filesystem;

	if( range.begin > 0 and not fs::exists(std::string(file_name.data(), file_name.size())) )
	{
		error = std::make_error_code(std::errc::no_such_file_or_directory);
		return sum;
	}
	std::ofstream file(_file_name, std::ios_base::out | std::ios_base::binary);
	if( not file.is_open() )
	{
		error = std::make_error_code(static_cast<std::errc>(errno));
		return sum;
	}
	using pos_type = std::ifstream::pos_type;
	file.seekp(static_cast<pos_type>(range.begin));

	constexpr size_t tcp_buf_size = 0xFFFF;
	char buf[tcp_buf_size] {0};

	auto total = range.total;
	size_t size = 0;

	while( can_read_body() )
	{
		size = read(buffer(buf, tcp_buf_size), error);
		if( error )
			break;

		sum += size;
		if( total == 0 )
			file.write(buf, static_cast<pos_type>(size));

		else if( size > total )
		{
			file.write(buf, static_cast<pos_type>(total));
			break;
		}
		else
		{
			file.write(buf, static_cast<pos_type>(size));
			total -= size;
		}
		sleep_for(512us);
	}
	file.close();
	return sum;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::save_file(std::string_view file_name, error_code &error) noexcept
{
	return save_file(file_name, {}, error);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_request<Stream,CharT>::async_save_file(std::string_view file_name, const req_range &range, Token &&token)
{
	if constexpr( is_function_v<Token> )
	{
		auto _file_name = std::string(file_name.data(), file_name.size());
		co_spawn_detached([this, _file_name = std::move(_file_name), range, callback = std::move(token)]() -> awaitable<void>
		{
			error_code error;
			auto res = co_await async_save_file(_file_name, range, use_awaitable|error);
			callback(res, error);
			co_return ;
		},
		get_executor());
	}
#ifdef LIBGS_USING_BOOST_ASIO
	else if( std::is_same_v<std::decay_t<Token>, yield_context> )
	{
		std::size_t sum = 0;
		error_code error;

		if( token.ec_ )
			*token.ec_ = error;
		do {
			if( file_name.empty() )
			{
				error = std::make_error_code(std::errc::invalid_argument);
				break;
			}
			auto _file_name = app::absolute_path(file_name);
			using namespace std::chrono_literals;
			namespace fs = std::filesystem;

			if( range.begin > 0 and not fs::exists(file_name) )
			{
				error = std::make_error_code(std::errc::no_such_file_or_directory);
				break;
			}
			std::ofstream file(_file_name, std::ios_base::out | std::ios_base::binary);
			if( not file.is_open() )
			{
				error = std::make_error_code(static_cast<std::errc>(errno));
				break;
			}
			using pos_type = std::ifstream::pos_type;
			file.seekp(static_cast<pos_type>(range.begin));

			constexpr size_t tcp_buf_size = 0xFFFF;
			char buf[tcp_buf_size] {0};

			auto total = range.total;
			size_t size = 0;

			while( can_read_body() )
			{
				size = async_read(buffer(buf, tcp_buf_size), token[error]);
				if( error )
					break;

				sum += size;
				if( total == 0 )
					file.write(buf, static_cast<pos_type>(size));

				else if( size > total )
				{
					file.write(buf, static_cast<pos_type>(total));
					break;
				}
				else
				{
					file.write(buf, static_cast<pos_type>(size));
					total -= size;
				}
				sleep_for(512us);
			}
			file.close();
		}
		while(false);

		if( not error )
			return sum;
		else if( token.ec_ )
			*token.ec_ = error;
		else
			throw system_error(error, "libgs::http::server_request::async_save_file");
		return sum;
	}
#endif //LIBGS_USING_BOOST_ASIO
	else
	{
		return [](basic_server_request *self, std::string file_name, req_range range, Token token) mutable -> awaitable<size_t>
		{
			self->m_impl->token_reset(token);
			error_code error;
			size_t sum = 0;
			do {
				if( file_name.empty() )
				{
					error = std::make_error_code(std::errc::invalid_argument);
					break;
				}
				auto _file_name = app::absolute_path(file_name);
				using namespace std::chrono_literals;
				namespace fs = std::filesystem;

				if( range.begin > 0 and not fs::exists(file_name) )
				{
					error = std::make_error_code(std::errc::no_such_file_or_directory);
					break;
				}
				std::ofstream file(_file_name, std::ios_base::out | std::ios_base::binary);
				if( not file.is_open() )
				{
					error = std::make_error_code(static_cast<std::errc>(errno));
					break;
				}
				using pos_type = std::ifstream::pos_type;
				file.seekp(static_cast<pos_type>(range.begin));

				constexpr size_t tcp_buf_size = 0xFFFF;
				char buf[tcp_buf_size] {0};

				auto total = range.total;
				size_t size = 0;

				while( self->can_read_body() )
				{
					if constexpr( detail::concept_has_get<Token> )
						size = co_await self->async_read(buffer(buf, tcp_buf_size), use_awaitable|error|token.get_cancellation_slot());
					else
						size = co_await self->async_read(buffer(buf, tcp_buf_size), use_awaitable|error);
					if( error )
						break;

					sum += size;
					if( total == 0 )
						file.write(buf, static_cast<pos_type>(size));

					else if( size > total )
					{
						file.write(buf, static_cast<pos_type>(total));
						break;
					}
					else
					{
						file.write(buf, static_cast<pos_type>(size));
						total -= size;
					}
					sleep_for(512us);
				}
				file.close();
			}
			while(false);
			co_return sum;
		}
		(this, {file_name.data(), file_name.size()}, range, token);
	}
}

template <concept_tcp_stream Stream, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_request<Stream,CharT>::async_save_file(std::string_view file_name, Token &&token)
{
	return async_save_file(file_name, {}, std::forward<Token>(token));
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::keep_alive() const noexcept
{
	return m_impl->m_parser->keep_alive();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::support_gzip() const noexcept
{
	return m_impl->m_parser->support_gzip();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::is_chunked() const noexcept
{
	if( stoi32(version()) < 1.1 )
		return false;
	auto it = m_impl->m_headers.find(header_t::transfer_encoding);
	return it != m_impl->m_headers.end() and str_to_lower(it->second) == detail::_key_static_string<CharT>::chunked;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::can_read_body() const noexcept
{
	return not is_eof();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::is_eof() const noexcept
{
	return m_impl->m_parser->is_eof();
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::endpoint_t basic_server_request<Stream,CharT>::remote_endpoint() const
{
	return socket_operation_helper<next_layer_t>::remote_endpoint(m_impl->m_next_layer);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::endpoint_t basic_server_request<Stream,CharT>::local_endpoint() const
{
	return socket_operation_helper<next_layer_t>::local_endpoint(m_impl->m_next_layer);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::executor_t &basic_server_request<Stream,CharT>::get_executor() noexcept
{
	return socket_operation_helper<next_layer_t>::get_executor(m_impl->m_next_layer);
}

template <concept_tcp_stream Stream, concept_char_type CharT>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::cancel() noexcept
{
	m_impl->m_next_layer.cancel();
	return *this;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::next_layer_t&
basic_server_request<Stream,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <concept_tcp_stream Stream, concept_char_type CharT>
typename basic_server_request<Stream,CharT>::next_layer_t&
basic_server_request<Stream,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
