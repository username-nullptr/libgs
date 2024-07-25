
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
#define LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H

namespace libgs::http
{

template <typename Request, concept_char_type CharT>
class basic_server_response<Request,CharT>::impl
{
	LIBGS_DISABLE_COPY(impl)

	using fpos_t = std::ifstream::pos_type;
	using string_list_t = basic_string_list<CharT>;
	using static_string = detail::_response_helper_static_string<CharT>;

public:
	explicit impl(next_layer_t &&next_layer) :
		m_helper(next_layer.version(), next_layer.headers()),
		m_next_layer(std::move(next_layer)) {}

	impl(impl &&other) noexcept = default;
	impl &operator=(impl &&other) noexcept = default;

public:
	[[nodiscard]] awaitable<size_t> send_file(std::string_view file_name, const resp_ranges &ranges,  error_code &error)
	{
		namespace fs = std::filesystem;
		if( not fs::exists(file_name) )
		{
			throw system_error(std::make_error_code(std::errc::no_such_file_or_directory),
							   "libgs::http::response_helper::write('{}')", file_name);
		}
		std::ifstream file(file_name.data(), std::ios_base::in | std::ios_base::binary);
		if( not file.is_open() )
		{
			throw system_error(std::make_error_code(static_cast<std::errc>(errno)),
							   "libgs::http::response_helper::write('{}')", file_name);
		}
		size_t res = 0;
		std::exception *exp = nullptr;
		try {
			if( ranges.empty() )
			{
				auto it = m_request_headers.find(header_t::range);
				if( it != m_request_headers.end() )
					res = co_await range_transfer(file_name, file, it->second.to_string(), error);
				else
					res = co_await default_transfer(file_name, file, error);
			}
			else
			{
				string_t range_text = static_string::bytes_start;
				for(auto &range : ranges)
				{
					if( range.begin >= range.end )
						range_text += std::format(static_string::range_format, range.begin, range.end);
					else
					{
						throw system_error(std::make_error_code(static_cast<std::errc>(errc::invalid_argument)),
										   "gts::response::write_file: range error: begin > end");
					}
				}
				range_text.pop_back();
				res = co_await range_transfer(file_name, file, range_text, error);
			}
		}
		catch(std::exception &ex) {
			exp = &ex;
		}
		file.close();
		if( exp )
			throw *exp;
		co_return res;
	}

private:
	awaitable<size_t> default_transfer(std::string_view file_name, std::ifstream &file, error_code &error)
	{
		using namespace std::chrono_literals;
		namespace fs = std::filesystem;

		auto mime_type = get_mime_type(file_name);
		spdlog::debug("resource mime-type: {}.", mime_type);

		auto file_size = fs::file_size(file_name);
		if( file_size == 0 )
			co_return 0;

		q_ptr->set_header(header_t::content_type, mbstoxx<CharT>(mime_type));
		auto sum = co_await write_header(file_size, error);
		if( error )
			co_return sum;

		fpos_t buf_size = m_get_write_buffer_size ? m_get_write_buffer_size() : 65536;
		char *fr_buf = new char[buf_size] {0};

		while( not file.eof() )
		{
			file.read(fr_buf, buf_size);
			auto size = file.gcount();
			if( size == 0 )
				break;

			co_await write_body(fr_buf, size, error);
			if( error )
				break;

			co_await co_sleep_for(512us);
		}
		delete[] fr_buf;
		co_return sum;
	}

private:
	struct range_value : resp_range
	{
		std::string cr_line {};
		std::size_t size = 0;
	};

	bool range_parsing(std::string_view file_name, string_view_t range_str, range_value &rv)
	{
		rv.size = 0;
		auto list = string_list_t::from_string(range_str, '-', false);

		if( list.size() > 2 )
		{
			spdlog::debug("Range format error: {}.", range_str);
			return false;
		}
		namespace fs = std::filesystem;
		auto file_size = fs::file_size(file_name);

		if( list[0].empty() )
		{
			if( list[1].empty() )
			{
				spdlog::debug("Range format error.");
				return false;
			}
			rv.size = ston_or<size_t>(list[1]);

			if( rv.size == 0 or rv.size > file_size )
			{
				spdlog::debug("Range size is invalid.");
				return false;
			}
			rv.end = file_size - 1;
			rv.begin = file_size - rv.size;
		}
		else if( list[1].empty() )
		{
			if( list[0].empty() )
			{
				spdlog::debug("Range format error.");
				return false;
			}
			rv.begin = ston_or<size_t>(list[0]);
			rv.end = file_size - 1;

			if( rv.begin > rv.end )
			{
				spdlog::debug("Range is invalid.");
				return false;
			}
			rv.size = file_size - rv.begin;
		}
		else
		{
			rv.begin = ston_or<size_t>(list[0]);
			rv.end = ston_or<size_t>(list[1]);

			if( rv.begin > rv.end or rv.end >= file_size )
			{
				spdlog::debug("Range is invalid.");
				return false;
			}
			rv.size = rv.end - rv.begin + 1;
		}
		return true;
	}

	awaitable<size_t> send_range(std::ifstream &file,
								 std::string_view boundary,
								 std::string_view ct_line,
								 std::list<range_value> &range_value_queue,
								 error_code &error)
	{
		using namespace std::chrono_literals;

		assert(not range_value_queue.empty());
		auto sum = co_await write_header(0,error);

		fpos_t buf_size = m_get_write_buffer_size ? m_get_write_buffer_size() : 65536;
		if( range_value_queue.size() == 1 )
		{
			auto &value = range_value_queue.back();
			file.seekg(value.begin, std::ios_base::beg);

			auto buf = new char[buf_size] {0};
			while( not file.eof() )
			{
				if( value.size <= static_cast<size_t>(buf_size) )
				{
					file.read(buf, value.size);
					auto size = file.gcount();

					sum += co_await write_body(buf, size, error);
					if( error )
						break;

					co_await co_sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += co_await write_body(buf, size, error);
				if( error )
					break;

				value.size -= buf_size;
				co_await co_sleep_for(512us);
			}
			delete[] buf;
			co_return sum;
		}
		else if( range_value_queue.empty() )
			co_return 0;

		auto buf = new char[buf_size] {0};
		auto sp_buf = std::shared_ptr<char>(buf); LIBGS_UNUSED(sp_buf);

		for(auto &value : range_value_queue)
		{
			std::string body;
			body.reserve(2 + boundary.size() + 2 +
						 ct_line.size() + 2 +
						 value.cr_line.size() + 2 +
						 2);

			body.append("--").append(boundary).append("\r\n")
					.append(ct_line).append("\r\n")
					.append(value.cr_line).append("\r\n"
												  "\r\n");

			sum += co_await write_body(body.c_str(), body.size(), error);
			if( error )
				co_return sum;

			file.seekg(value.begin, std::ios_base::beg);
			while( not file.eof() )
			{
				if( value.size <= static_cast<size_t>(buf_size) )
				{
					file.read(buf, value.size);
					auto size = file.gcount();

					if( size == 0 )
						break;

					buf[size + 0] = '\r';
					buf[size + 1] = '\n';

					sum += co_await write_body(buf, size + 2, error);
					if( error )
						co_return sum;

					co_await co_sleep_for(512us);
					break;
				}
				file.read(buf, buf_size);
				auto size = file.gcount();

				sum += co_await write_body(buf, size, error);
				if( error )
					co_return sum;

				value.size -= buf_size;
				co_await co_sleep_for(512us);
			}
		}
		auto abuf = "--" + std::string(boundary.data(), boundary.size()) + "--\r\n";
		sum += co_await write_body(abuf.c_str(), abuf.size(), error);
		co_return sum;
	}

	awaitable<size_t> range_transfer(std::string_view file_name, std::ifstream &file, string_view_t _range_str, error_code &error)
	{
		string_t range_str(_range_str.data(), _range_str.size());
		for(auto i=range_str.size(); i>0; i--)
		{
			if( range_str[i] == 0x20/*SPACE*/ )
				range_str.erase(i,1);
		}
		if( range_str.empty() )
		{
			q_ptr->set_status(status::bad_request);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

			// bytes=x-y, m-n, i-j ...
		else if( range_str.substr(0,6) != static_string::bytes_start )
		{
			q_ptr->set_status(status::range_not_satisfiable);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

		// x-y, m-n, i-j ...
		auto range_list_str = range_str.substr(6);
		if( range_list_str.empty() )
		{
			q_ptr->set_status(status::range_not_satisfiable);
			co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
		}

		// (x-y) ( m-n) ( i-j) ...
		auto range_list = string_list_t::from_string(range_list_str, 0x2C/*,*/);
		auto mime_type = get_mime_type(file_name);

		q_ptr->set_status(status::partial_content);
		std::list<range_value> range_value_list;

		if( range_list.size() == 1 )
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(file_name, range_list[0], range_value) )
			{
				q_ptr->set_status(status::range_not_satisfiable);
				co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
			}
			q_ptr->set_header(header_t::accept_ranges , static_string::bytes)
					.set_header(header_t::content_type  , mime_type)
					.set_header(header_t::content_length, range_value.size)
					.set_header(header_t::content_range , {static_string::content_range_format, range_value.begin, range_value.end, range_value.size});

			co_return co_await send_range(file, "", "", range_value_list, error);
		} // if( rangeList.size() == 1 )

		using namespace std::chrono;
		auto boundary = std::format("{}_{}", __func__, duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

		q_ptr->set_header(header_t::content_type, static_string::content_type_boundary + boundary);

		auto ct_line = std::format("{}: {}", header::content_type, mime_type);
		std::size_t content_length = 0;

		for(auto &str : range_list)
		{
			range_value_list.emplace_back();
			auto &range_value = range_value_list.back();

			if( not range_parsing(file_name, str_trimmed(str), range_value) )
			{
				q_ptr->set_status(status::range_not_satisfiable);
				co_return co_await m_writer(std::format("{} ({})", to_status_description(m_status), m_status), error);
			}
			namespace fs = std::filesystem;
			range_value.cr_line = std::format("{}: bytes {}-{}/{}", header::content_range,
											  range_value.begin, range_value.end, fs::file_size(file_name));
			/*
				--boundary<CR><LF>
				Content-Type: xxx<CR><LF>
				Content-Range: bytes 3-11/96<CR><LF>
				<CR><LF>
				012345678<CR><LF>
				--boundary<CR><LF>
				Content-Type: xxx<CR><LF>
				Content-Range: bytes 0-7/96<CR><LF>
				<CR><LF>
				01235467<CR><LF>
				--boundary--<CR><LF>
			*/
			content_length += 2 + boundary.size() + 2 +        // --boundary<CR><LF>
							  ct_line.size() + 2 +             // Content-Type: xxx<CR><LF>
							  range_value.cr_line.size() + 2 + // Content-Range: bytes 3-11/96<CR><LF>
							  2 +                              // <CR><LF>
							  range_value.size + 2;            // 012345678<CR><LF>
		}
		content_length += 2 + boundary.size() + 2 + 2;         // --boundary--<CR><LF>

		q_ptr->set_header(header_t::content_length, content_length)
				.set_header(header_t::accept_ranges , static_string::bytes);

		co_return co_await send_range(file, boundary, ct_line, range_value_list, error);
	}

public:
	helper_t m_helper;
	next_layer_t m_next_layer;
};

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::basic_server_response(next_layer_t &&next_layer) :
	m_impl(new impl(std::move(next_layer)))
{

}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::~basic_server_response()
{
	delete m_impl;
}

template <typename Request, concept_char_type CharT>
template <typename Request0>
basic_server_response<Request,CharT>::basic_server_response
(basic_server_response<Request0,CharT> &&other) noexcept
	requires concept_constructible<Request0,Request0&&> :
	m_impl(new impl(std::move(*other.m_impl)))
{

}

template <typename Request, concept_char_type CharT>
template <typename Request0>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::operator=
(basic_server_response<Request0,CharT> &&other) noexcept
	requires concept_constructible<Request0,Request0&&>
{
	*m_impl = std::move(*other.m_impl);
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_status(uint32_t status)
{
	m_impl->m_helper.set_status(status);
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_status(http::status status)
{
	m_impl->m_helper.set_status(status);
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_header(string_view_t key, value_t value)
{
	m_impl->m_helper.set_header(key, std::move(value));
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_cookie(string_view_t key, cookie_t cookie)
{
	m_impl->m_helper.set_cookie(key, std::move(cookie));
	return this->derived();
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::write()
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::write(error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_write(const const_buffer &buf, Token &&token)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, http::redirect redi)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, http::redirect redi, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::redirect(string_view_t url, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_redirect(string_view_t url, http::redirect redi, Token &&token)
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_redirect(string_view_t url, Token &&token)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file(std::string_view file_name, const resp_ranges &ranges)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file
(std::string_view file_name, const resp_ranges &ranges, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::send_file(std::string_view file_name, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_send_file
(std::string_view file_name, const resp_ranges &ranges, Token &&token)
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_send_file(std::string_view file_name, Token &&token)
{

}








template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_chunk_attribute(value_t attribute)
{
	m_impl->m_helper.set_chunk_attribute(std::move(attribute));
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::set_chunk_attributes(value_list_t attributes)
{
	m_impl->m_helper.set_chunk_attributes(std::move(attributes));
	return this->derived();
}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::chunk_end(const headers_t &headers)
{

}

template <typename Request, concept_char_type CharT>
size_t basic_server_response<Request,CharT>::chunk_end(const headers_t &headers, error_code &error) noexcept
{

}

template <typename Request, concept_char_type CharT>
template <asio::completion_token_for<void(size_t,error_code)> Token>
auto basic_server_response<Request,CharT>::async_chunk_end(const headers_t &headers, Token &&token)
{

}




template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::string_view_t
basic_server_response<Request,CharT>::version() const noexcept
{
	return m_impl->m_helper.version();
}

template <typename Request, concept_char_type CharT>
http::status basic_server_response<Request,CharT>::status() const noexcept
{
	return m_impl->m_helper.status();
}

template <typename Request, concept_char_type CharT>
const typename basic_server_response<Request,CharT>::headers_t&
basic_server_response<Request,CharT>::headers() const noexcept
{
	return m_impl->m_helper.headers();
}

template <typename Request, concept_char_type CharT>
const typename basic_server_response<Request,CharT>::cookies_t&
basic_server_response<Request,CharT>::cookies() const noexcept
{
	return m_impl->m_helper.cookies();
}

template <typename Request, concept_char_type CharT>
bool basic_server_response<Request,CharT>::headers_writed() const noexcept
{
	return m_impl->m_helper.headers_writed();
}

template <typename Request, concept_char_type CharT>
bool basic_server_response<Request,CharT>::chunk_end_writed() const noexcept
{
	return m_impl->m_helper.chunk_end_writed();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::cancel() noexcept
{
	m_impl->m_next_layer->m_impl->m_socket->cancel();
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_header(string_view_t key)
{
	m_impl->m_helper.unset_header(key);
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_cookie(string_view_t key)
{
	m_impl->m_helper.unset_cookie(key);
	return this->derived();
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT> &basic_server_response<Request,CharT>::unset_chunk_attribute(const value_t &attribute)
{
	m_impl->m_helper.unset_chunk_attribute(std::move(attribute));
	return this->derived();
}

template <typename Request, concept_char_type CharT>
const basic_server_response<Request,CharT>::next_layer_t&
basic_server_response<Request,CharT>::next_layer() const noexcept
{
	return m_impl->m_next_layer;
}

template <typename Request, concept_char_type CharT>
basic_server_response<Request,CharT>::next_layer_t&
basic_server_response<Request,CharT>::next_layer() noexcept
{
	return m_impl->m_next_layer;
}

} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_DETAIL_RESPONSE_H
