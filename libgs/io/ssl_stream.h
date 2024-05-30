
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

#ifndef LIBGS_IO_SSL_STREAM_H
#define LIBGS_IO_SSL_STREAM_H

#ifdef LIBGS_ENABLE_OPENSSL

#include <libgs/io/stream.h>
#include <asio/ssl.hpp>

namespace asio::ssl
{

template <typename Func>
concept concept_verify_callback =
	libgs::is_function_v<Func> and
	libgs::function_traits<Func>::arg_count == 2 and
	std::is_same_v<decltype(std::get<0>(libgs::function_traits<Func>::arg_types)),bool> and
	std::is_same_v<decltype(std::get<1>(libgs::function_traits<Func>::arg_types)),ssl::verify_context&>;

} //namespace asio::ssl

namespace libgs
{

namespace ssl = asio::ssl;

template<typename Stream>
using asio_ssl_stream = ssl::stream<Stream>;

namespace io
{

template <typename Stream, typename Derived = void>
class LIBGS_IO_TAPI ssl_stream :
	public basic_stream<crtp_derived_t<Derived,ssl_stream<Stream,Derived>>, typename Stream::executor_t>
{
	LIBGS_DISABLE_COPY(ssl_stream)

public:
	using executor_t = typename Stream::executor_t;
	using derived_t = crtp_derived_t<Derived,ssl_stream>;
	using base_t = basic_stream<derived_t, typename Stream::executor_t>;

	using next_layer_t = Stream;
	using native_next_layer_t = typename next_layer_t::native_t;

	using native_t = ssl::stream<native_next_layer_t>;
	using handshake_t = ssl::stream_base::handshake_type;

public:
	template <concept_execution_context Context>
	ssl_stream(Context &context, ssl::context &ssl);

	ssl_stream(const executor_t &exec, ssl::context &ssl);
	explicit ssl_stream(ssl::context &ssl);

	ssl_stream(auto next_layer, ssl::context &ssl) requires requires
	{ 
		native_t(std::move(next_layer.native()), ssl);
		next_layer_t(std::move(next_layer)); 
	};
	ssl_stream(native_t &&native);
	~ssl_stream() override;

public:
	ssl_stream(ssl_stream &&other) noexcept = default;
	ssl_stream &operator=(ssl_stream &&other) noexcept = default;
	ssl_stream &operator=(native_t &&native);

public:
	derived_t &handshake(handshake_t type, opt_token<callback_t<error_code>> tk);
	awaitable<void> handshake(handshake_t type, opt_token<error_code&> tk = {});

	derived_t &wave(opt_token<callback_t<error_code>> tk); //close
	awaitable<void> wave(opt_token<error_code&> tk = {}); //close

public:
	derived_t &set_verify_mode(ssl::verify_mode mode, no_time_token tk = {});
	derived_t &set_verify_depth(int depth, no_time_token tk = {});

	template <ssl::concept_verify_callback Func>
	derived_t &set_verify_callback(Func &&func, no_time_token tk = {});

public:
	[[nodiscard]] size_t read_buffer_size() const noexcept;
	[[nodiscard]] size_t write_buffer_size() const noexcept;

	[[nodiscard]] const native_t &native() const noexcept;
	[[nodiscard]] native_t &native() noexcept;

	[[nodiscard]] const next_layer_t &next_layer() const noexcept;
	[[nodiscard]] next_layer_t &next_layer() noexcept;

protected:
	[[nodiscard]] awaitable<error_code> _close(cancellation_signal *cnl_sig) noexcept;
	void _cancel() noexcept;

	bool m_handshake_cancel = false;
	next_layer_t m_next_layer;

private:
	friend class basic_stream<ssl_stream,executor_t>;
	static void _delete_native(void *native);
	void next_layer_ext();
};

}} //namespace libgs::io
#include <libgs/io/detail/ssl_stream.h>

#endif //LIBGS_ENABLE_OPENSSL
#endif //LIBGS_IO_SSL_STREAM_H
