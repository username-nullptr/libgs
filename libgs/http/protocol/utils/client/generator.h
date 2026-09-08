// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H

#include <libgs/http/protocol/utils/client/request_arg.h>
#include <libgs/http/protocol/utils/core/generator.h>
#include <libgs/core/url.h>

namespace libgs::http
{

template <>
class LIBGS_HTTP_API generator<protocol_model::client> final :
	public mutable_headers<generator<protocol_model::client>>,
	public mutable_cookies<value,generator<protocol_model::client>>,
	public mutable_chunk_attributes<generator<protocol_model::client>>
{
	LIBGS_DISABLE_COPY(generator)

public:
	using url_t = libgs::url;
	using request_arg_t = request_arg;

	using version_t = http::version;
	using mutable_headers::set_header;

public:
	generator(version_enum version, url_t url, request_arg_t arg,
		request_target_form target_form = request_target_form::origin
	);
	generator(url_t url, request_arg_t arg,
		request_target_form target_form = request_target_form::origin
	);
	~generator() override;

	generator(generator &&other) noexcept;
	generator &operator=(generator &&other) noexcept;

public:
	generator &emplace(url_t url, request_arg_t arg);
	generator &emplace(request_arg_t arg);
	generator &emplace(url_t url);

	[[nodiscard]] const url_t &url() const noexcept;
	[[nodiscard]] url_t &url() noexcept;

	[[nodiscard]] request_arg_t arg() const noexcept;
	[[nodiscard]] operator request_arg_t() const noexcept;

	generator &set_target_form(request_target_form form) noexcept;
	[[nodiscard]] request_target_form target_form() const noexcept;

public:
	template <method_enum Method>
	[[nodiscard]] std::string header_data(size_t body_size = 0);
	[[nodiscard]] std::string header_data(method_enum method, size_t body_size = 0);

	[[nodiscard]] std::string body_data(const const_buffer &buffer);
	[[nodiscard]] std::string chunk_end_data(const headers_t &trailer_headers = {});

public:
	[[nodiscard]] version_enum version() const noexcept;
	[[nodiscard]] generator_state pro_state() const noexcept;
	generator &reset() noexcept;

private:
	base_generator &base() noexcept;
	class impl;
	impl *m_impl;
};

using client_generator = generator<protocol_model::client>;

} //namespace libgs::http
#include <libgs/http/protocol/utils/client/detail/generator.h>


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CLIENT_GENERATOR_H
