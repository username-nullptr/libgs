
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

#include <libgs/core/string_list.h>
#include <libgs/core/app_utls.h>
#include <filesystem>
#include <fstream>

namespace libgs::http
{

template <typename Stream, concept_char_type CharT>
class basic_server_request<Stream,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

public:
	template <typename Native>
	impl(Native &&next_layer, parser_t &parser) :
		m_next_layer(std::forward<Native>(next_layer)), m_parser(parser) {}

	impl(impl &&other) noexcept :
		m_next_layer(std::move(other.m_next_layer)), m_parser(other.m_parser) {}

	impl &operator=(impl &&other) noexcept
	{
		m_next_layer = std::move(other.m_next_layer);
		return *this;
	}

public:
	next_layer_t m_next_layer;
	parser_t &m_parser;
};

template <typename Stream, concept_char_type CharT>
template <typename NextLayer>
basic_server_request<Stream,CharT>::basic_server_request(NextLayer &&next_layer, parser_t &parser)
	requires concept_constructible<next_layer_t, NextLayer> :
	m_impl(new impl(std::move(next_layer), parser))
{

}

template <typename Stream, concept_char_type CharT>
basic_server_request<Stream,CharT>::~basic_server_request()
{
	delete m_impl;
}

template <typename Stream, concept_char_type CharT>
basic_server_request<Stream,CharT>::basic_server_request(basic_server_request &&other) noexcept :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <typename Stream, concept_char_type CharT>
basic_server_request<Stream,CharT> &basic_server_request<Stream,CharT>::operator=(basic_server_request &&other) noexcept
{
	*m_impl = std::move(*other.m_impl);
	return *this;
}

template <typename Stream, concept_char_type CharT>
http::method basic_server_request<Stream,CharT>::method() const noexcept
{
	return m_impl->m_parser.method();
}

template <typename Stream, concept_char_type CharT>
std::basic_string_view<CharT> basic_server_request<Stream,CharT>::version() const noexcept
{
	return m_impl->m_parser.version();
}

template <typename Stream, concept_char_type CharT>
std::basic_string_view<CharT> basic_server_request<Stream,CharT>::path() const noexcept
{
	return m_impl->m_parser.path();
}

template <typename Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::parameters_t&
basic_server_request<Stream,CharT>::parameters() const noexcept
{
	return m_impl->m_parser.parameters();
}

template <typename Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::headers_t&
basic_server_request<Stream,CharT>::headers() const noexcept
{
	return m_impl->m_parser.headers();
}

template <typename Stream, concept_char_type CharT>
const typename basic_server_request<Stream,CharT>::cookies_t&
basic_server_request<Stream,CharT>::cookies() const noexcept
{
	return m_impl->m_parser.cookies();
}

template <typename Stream, concept_char_type CharT>
const basic_value<CharT> &basic_server_request<Stream,CharT>::parameter(string_view_t key) const
{
	auto map = m_impl->m_parser.parameters();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::request::parameter: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <typename Stream, concept_char_type CharT>
const basic_value<CharT> &basic_server_request<Stream,CharT>::header(string_view_t key) const
{
	auto map = m_impl->m_parser.headers();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::request::header: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <typename Stream, concept_char_type CharT>
const basic_value<CharT> &basic_server_request<Stream,CharT>::cookie(string_view_t key) const
{
	auto map = m_impl->m_parser.cookies();
	auto it = map.find({key.data(), key.size()});
	if( it == map.end() )
		throw runtime_error("libgs::http::request::cookie: key '{}' not exists.", xxtombs<CharT>(key));
	return it->second;
}

template <typename Stream, concept_char_type CharT>
basic_value<CharT> basic_server_request<Stream,CharT>::parameter_or(string_view_t key, value_t def_value) const noexcept
{
	auto map = m_impl->m_parser.parameters();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <typename Stream, concept_char_type CharT>
basic_value<CharT> basic_server_request<Stream,CharT>::header_or(string_view_t key, value_t def_value) const noexcept
{
	auto map = m_impl->m_parser.headers();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <typename Stream, concept_char_type CharT>
basic_value<CharT> basic_server_request<Stream,CharT>::cookie_or(string_view_t key, value_t def_value) const noexcept
{
	auto map = m_impl->m_parser.cookies();
	auto it = map.find({key.data(), key.size()});
	return it == map.end() ? def_value : it->second;
}

template <typename Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::read(const mutable_buffer &buf)
{
	error_code error;
	auto res = read(buf, error);
	if( error )
		throw system_error(error, "libgs::http::request::read");
	return res;
}

template <typename Stream, concept_char_type CharT>
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
	m_impl->m_next_layer.get_option(op, error);
	if( error )
		return sum;

	auto dst_buf = reinterpret_cast<char*>(buf.data());
	do {
		auto body = m_impl->m_parser.take_partial_body(buf_size);
		sum += body.size();

		memcpy(dst_buf + sum, body.c_str(), body.size());
		if( sum == buf_size or not m_impl->m_parser.can_read_body() )
			break;

		body = std::string(op.value(),'\0');
		size_t tmp_sum = 0;
		do {
			tmp_sum += m_impl->m_next_layer.read_some(buffer(body.c_str() + tmp_sum, body.size() - tmp_sum), error);
			sum += tmp_sum;
			if( error )
			{
				if( error == errc::interrupted )
					continue;
			}
		}
		while(false);
		if( tmp_sum == 0 or not m_impl->m_parser.append({body.c_str(), tmp_sum}, error) )
			break;
	}
	while(true);
	return sum;
}

template <typename Stream, concept_char_type CharT>
template <concept_callable<std::string,error_code> Func>
void basic_server_request<Stream,CharT>::async_read(const mutable_buffer &buf, Func &&callback) noexcept
{
	co_spawn_detached([this, buf, callback = std::move(callback)]() -> awaitable<void>
	{
		error_code error;
		auto size = co_await async_read(buf, use_awaitable|error);
		callback(size, error);
		co_return ;
	},
	get_executor());
}

template <typename Stream, concept_char_type CharT>
template <asio::completion_token_for<void> Token>
awaitable<size_t> basic_server_request<Stream,CharT>::async_read(const mutable_buffer &buf, Token &&token)
{
	token.ec_.assign(0, std::system_category());
	const size_t buf_size = buf.size();

	size_t sum = 0;
	if( buf_size == 0 )
		co_return sum;
	else if( is_eof() )
	{
		token.ec_ = std::make_error_code(static_cast<std::errc>(errc::eof));
		co_return sum;
	}
	asio::socket_base::receive_buffer_size op;
	m_impl->m_next_layer.get_option(op, token.ec_);
	if( token.ec_ )
		co_return sum;

	auto dst_buf = reinterpret_cast<char*>(buf.data());
	do {
		auto body = m_impl->m_parser.take_partial_body(buf_size);
		sum += body.size();

		memcpy(dst_buf + sum, body.c_str(), body.size());
		if( sum == buf_size or not m_impl->m_parser.can_read_body() )
			break;

		body = std::string(op.value(),'\0');
		size_t tmp_sum = 0;
		do {
			tmp_sum += co_await m_impl->m_next_layer.async_read_some
					(buffer(body.c_str() + tmp_sum, body.size() - tmp_sum), use_awaitable|token.ec_);
			sum += tmp_sum;
			if( token.ec_error )
			{
				if( token.ec_error == errc::interrupted )
					continue;
			}
		}
		while(false);
		if( tmp_sum == 0 or not m_impl->m_parser.append({body.c_str(), tmp_sum}, token.error) )
			break;
	}
	while(true);
	co_return sum;
}

template <typename Stream, concept_char_type CharT>
size_t basic_server_request<Stream,CharT>::async_read(const mutable_buffer &buf, asio::yield_context &yc)
{
	auto pre_error = yc.ec_;
	error_code error;
	size_t sum = 0;
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
		m_impl->m_next_layer.get_option(op, error);
		if( error )
			break;

		auto dst_buf = reinterpret_cast<char*>(buf.data());
		do {
			auto body = m_impl->m_parser.take_partial_body(buf_size);
			sum += body.size();

			memcpy(dst_buf + sum, body.c_str(), body.size());
			if( sum == buf_size or not m_impl->m_parser.can_read_body() )
				break;

			body = std::string(op.value(),'\0');
			size_t tmp_sum = 0;
			do {
				tmp_sum += m_impl->m_next_layer.async_read_some(buffer(body.c_str() + tmp_sum, body.size() - tmp_sum), yc[error]);
				sum += tmp_sum;
				if( error )
				{
					if( error == errc::interrupted )
						continue;
				}
			}
			while(false);
			if( tmp_sum == 0 or not m_impl->m_parser.append({body.c_str(), tmp_sum}, error) )
				break;
		}
		while(true);
	}
	while(false);
	yc.ec_ = nullptr;

	if( not error )
		return sum;
	else if( pre_error )
		*pre_error = error;
	else
		throw system_error(error, "libgs::http::request::async_read");
	return sum;
}

template <typename Stream, concept_char_type CharT>
std::string basic_server_request<Stream,CharT>::read_all()
{

}

template <typename Stream, concept_char_type CharT>
std::string basic_server_request<Stream,CharT>::read_all(error_code &error) noexcept
{

}

template <typename Stream, concept_char_type CharT>
template <concept_callable<size_t,error_code> Func>
void basic_server_request<Stream,CharT>::async_read_all(Func &&callback) noexcept
{

}

template <typename Stream, concept_char_type CharT>
template <asio::completion_token_for<void> Token>
awaitable<std::string> basic_server_request<Stream,CharT>::async_read_all(Token &&token)
{

}

template <typename Stream, concept_char_type CharT>
std::string basic_server_request<Stream,CharT>::async_read_all(asio::yield_context &yc)
{

}

template <typename Stream, concept_char_type CharT>
awaitable<size_t> basic_server_request<Stream,CharT>::read(buffer<void*> buf, opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return 0;
	}
	if( tk.error )
		*tk.error = std::error_code();

	size_t sum = 0;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		auto dst_buf = reinterpret_cast<char*>(buf.data);
		auto buf_size = m_impl->m_next_layer.read_buffer_size();
		do {
			auto body = m_impl->m_parser.take_partial_body(buf.size);
			sum += body.size();

			memcpy(dst_buf + sum, body.c_str(), body.size());
			if( sum == buf.size )
				break;

			if( tk.cnl_sig )
				sum += co_await m_impl->m_next_layer.read_some({body, buf_size}, {*tk.cnl_sig, error});
			else
				sum += co_await m_impl->m_next_layer.read_some({body, buf_size}, error);

			io::detail::check_error(tk.error, error, "libgs::http::request::read");
			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return sum;
}

template <typename Stream, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_request<Stream,CharT,Derived>::read(buffer<std::string&> buf, opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return 0;
	}
	if( tk.error )
		*tk.error = std::error_code();

	size_t sum = 0;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		do {
			auto body = m_impl->m_parser.take_partial_body(buf.size);
			sum += body.size();

			buf.data += std::string(body.c_str(), body.size());
			if( sum == buf.size )
				break;

			body.clear();
			if( tk.cnl_sig )
				sum += co_await m_impl->m_next_layer.read_some(body, {*tk.cnl_sig, error});
			else
				sum += co_await m_impl->m_next_layer.read_some(body, error);

			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return sum;
}

template <typename Stream, concept_char_type CharT, typename Derived>
awaitable<std::string> basic_server_request<Stream,CharT,Derived>::read_all(opt_token<error_code&> tk)
{
	error_code error;
	if( not can_read_body() )
	{
		error = std::make_error_code(static_cast<std::errc>(errc::eof));
		if( not io::detail::check_error(tk.error, error, "libgs::http::request::read") )
			co_return "";
	}
	if( tk.error )
		*tk.error = std::error_code();

	auto &headers = m_impl->m_parser.headers();
	auto it = headers.find(header_t::content_length);

	size_t buf_size = 0;
	if( it != headers.end() )
		buf_size = it->second.template get<size_t>();
	else
		buf_size = m_impl->m_next_layer.read_buffer_size();

	std::string buf;
	auto lambda_read = [&]() mutable -> awaitable<size_t>
	{
		do {
			auto body = m_impl->m_parser.take_partial_body(buf_size);
			buf += std::string(body.c_str(), body.size());

			body.clear();
			if( tk.cnl_sig )
				co_await m_impl->m_next_layer.read_some(body, {*tk.cnl_sig, error});
			else
				co_await m_impl->m_next_layer.read_some(body, error);

			if( body.empty() or not m_impl->m_parser.append(body) )
				break;
		}
		while(true);
		co_return 0;
	};
	using namespace std::chrono_literals;
	if( tk.rtime == 0ns )
		co_await lambda_read();

	auto var = co_await (lambda_read() or co_sleep_for(tk.rtime));
	if( var.index() == 1 )
		error = std::make_error_code(static_cast<std::errc>(errc::timed_out));

	io::detail::check_error(tk.error, error, "libgs::http::request::read");
	co_return buf;
}

template <typename Stream, concept_char_type CharT, typename Derived>
awaitable<size_t> basic_server_request<Stream,CharT,Derived>::save_file(const std::string &file_name, opt_token<size_t,size_t,error_code&> tk)
{
	if( file_name.empty() )
	{
		io::detail::check_error(tk.error, std::make_error_code(std::errc::invalid_argument), "libgs::http::request::save_file");
		co_return 0;
	}
	auto _file_name = app::absolute_path(file_name);
	using namespace std::chrono_literals;
	namespace fs = std::filesystem;

	if( tk.begin > 0 and not fs::exists(file_name) )
	{
		io::detail::check_error(tk.error, std::make_error_code(std::errc::no_such_file_or_directory), "libgs::http::request::save_file");
		co_return 0;
	}
	std::ofstream file(_file_name, std::ios_base::out | std::ios_base::binary);
	if( not file.is_open() )
	{
		io::detail::check_error(tk.error, std::make_error_code(static_cast<std::errc>(errno)), "libgs::http::request::save_file");
		co_return 0;
	}
	using pos_type = std::ifstream::pos_type;
	file.seekp(static_cast<pos_type>(tk.begin));

	auto tcp_buf_size = m_impl->tcp_ip_buffer_size();
	auto buf = new char[tcp_buf_size] {0};

	std::size_t sum = 0;
	error_code error;
	size_t size;

	while( can_read_body() )
	{
		if( tk.cnl_sig )
			size = read({buf, tcp_buf_size}, {tk.rtime, *tk.cnl_sig, error});
		else
			size = read({buf, tcp_buf_size}, {tk.rtime, error});

		if( error )
		{
			if( tk.error )
				*tk.error = error;
			else if( error != errc::operation_aborted )
			{
				file.close();
				delete[] buf;
				throw system_error(error, "libgs::http::request::save_file");
			}
		}
		sum += size;
		if( tk.total == 0 )
			file.write(buf, static_cast<pos_type>(size));

		else if( size > tk.total )
		{
			file.write(buf, static_cast<pos_type>(tk.total));
			break;
		}
		else
		{
			file.write(buf, static_cast<pos_type>(size));
			tk.total -= size;
		}
		co_await co_sleep_for(512us);
	}
	file.close();
	delete[] buf;
	co_return sum;
}

template <typename Stream, concept_char_type CharT, typename Derived>
bool basic_server_request<Stream,CharT,Derived>::keep_alive() const
{
	return m_impl->m_parser.keep_alive();
}

template <typename Stream, concept_char_type CharT, typename Derived>
bool basic_server_request<Stream,CharT,Derived>::support_gzip() const
{
	return m_impl->m_parser.support_gzip();
}

template <typename Stream, concept_char_type CharT, typename Derived>
bool basic_server_request<Stream,CharT,Derived>::is_chunked() const
{
	if( stoi32(version()) < 1.1 )
		return false;
	auto it = m_impl->m_headers.find(header_t::transfer_encoding);
	return it != m_impl->m_headers.end() and str_to_lower(it->second) == detail::_key_static_string<CharT>::chunked;
}

template <typename Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::can_read_body() const noexcept
{
	return not is_eof();
}

template <typename Stream, concept_char_type CharT>
bool basic_server_request<Stream,CharT>::is_eof() const noexcept
{
	return m_impl->m_parser.is_eof();
}

template <typename Stream, concept_char_type CharT, typename Derived>
io::ip_endpoint basic_server_request<Stream,CharT,Derived>::remote_endpoint() const
{
	return m_impl->m_next_layer.remote_endpoint();
}

template <typename Stream, concept_char_type CharT, typename Derived>
io::ip_endpoint basic_server_request<Stream,CharT,Derived>::local_endpoint() const
{
	return m_impl->m_next_layer.local_endpoint();
}

template <typename Stream, concept_char_type CharT, typename Derived>
typename basic_server_request<Stream,CharT,Derived>::derived_t&
basic_server_request<Stream,CharT,Derived>::cancel() noexcept
{
	m_impl->m_next_layer.cancel();
	return this->derived();
}

template <typename Stream, concept_char_type CharT, typename Derived>
const typename basic_server_request<Stream,CharT,Derived>::next_layer_t&
basic_server_request<Stream,CharT,Derived>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <typename Stream, concept_char_type CharT, typename Derived>
typename basic_server_request<Stream,CharT,Derived>::next_layer_t&
basic_server_request<Stream,CharT,Derived>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_REQUEST_H
