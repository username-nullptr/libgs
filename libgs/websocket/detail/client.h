// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_CLIENT_H
#define LIBGS_WEBSOCKET_DETAIL_CLIENT_H

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

#include <libgs/websocket/detail/handshake_io.h>
#include <libgs/websocket/detail/secure_random.h>
#include <libgs/websocket/protocol/handshake.h>
#include <array>
#include <cstring>

namespace libgs::websocket { namespace detail
{

inline constexpr std::chrono::milliseconds default_handshake_timeout {30000};

[[nodiscard]] inline bool ascii_equal_case_insensitive
(std::string_view lhs, std::string_view rhs) noexcept
{
	if(lhs.size() != rhs.size())
		return false;
	for(size_t index = 0; index < lhs.size(); ++index)
	{
		auto left = static_cast<unsigned char>(lhs[index]);
		auto right = static_cast<unsigned char>(rhs[index]);
		if(left >= 'A' and left <= 'Z')
			left = static_cast<unsigned char>(left + ('a' - 'A'));
		if(right >= 'A' and right <= 'Z')
			right = static_cast<unsigned char>(right + ('a' - 'A'));
		if(left != right)
			return false;
	}
	return true;
}

[[nodiscard]] inline bool websocket_owned_request_header
(std::string_view name) noexcept
{
	return ascii_equal_case_insensitive(name, http::header::host) or
		ascii_equal_case_insensitive(name, http::header::connection) or
		ascii_equal_case_insensitive(name, http::header::upgrade) or
		ascii_equal_case_insensitive(name, http::header::proxy_authorization) or
		(name.size() >= 14 and ascii_equal_case_insensitive(
			name.substr(0, 14), "Sec-WebSocket-"));
}

[[nodiscard]] inline sys_expected<url>
canonical_websocket_url(const url &endpoint) noexcept
{
	try
	{
		if(not endpoint.is_valid() or endpoint.host().empty() or
			endpoint.has_fragment())
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		const auto scheme = strtls::to_lower(endpoint.protocol());
		std::string_view canonical_scheme;
		if(scheme == "ws" or scheme == "http")
			canonical_scheme = "ws";
		else if(scheme == "wss" or scheme == "https")
			canonical_scheme = "wss";
		else
			return sys_unexpected(
				make_error_code(std::errc::protocol_not_supported));

		auto text = endpoint.to_string();
		auto separator = text.find("://");
		if(separator == std::string::npos)
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		text.replace(0, separator, canonical_scheme);

		url result(text);
		if(not result.is_valid())
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		return result;
	}
	catch(const std::bad_alloc&)
	{
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...)
	{
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
}

[[nodiscard]] inline error_code validate_open_request
(connect_request &request, const stream_config &stream) noexcept
{
	try
	{
		auto endpoint = canonical_websocket_url(request.endpoint);
		if(not endpoint)
			return endpoint.error();
		request.endpoint = std::move(*endpoint);

		if(stream.read_buffer_size == 0)
			return make_error_code(std::errc::invalid_argument);
		if(not request.extensions.empty())
			return make_error_code(errc::unsupported_extension);

		for(const auto &[name, value] : request.request_options.headers())
		{
			ignore_unused(value);
			if(websocket_owned_request_header(name))
				return make_error_code(errc::invalid_upgrade);
		}
		return {};
	}
	catch(const std::bad_alloc&)
	{
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...)
	{
		return make_error_code(std::errc::io_error);
	}
}

[[nodiscard]] inline sys_expected<url>
http_transport_url(const url &endpoint) noexcept
{
	try
	{
		auto text = endpoint.to_string();
		auto separator = text.find("://");
		if(separator == std::string::npos)
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		const auto scheme = strtls::to_lower(endpoint.protocol());
		text.replace(0, separator, scheme == "wss" ? "https" : "http");
		url result(text);
		if(not result.is_valid())
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		return result;
	}
	catch(const std::bad_alloc&)
	{
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...)
	{
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
}

[[nodiscard]] inline uint16_t websocket_effective_port(const url &value) noexcept
{
	if(value.port() != 0)
		return value.port();
	return ascii_equal_case_insensitive(value.protocol(), "wss") ? 443 : 80;
}

[[nodiscard]] inline bool same_websocket_origin
(const url &lhs, const url &rhs) noexcept
{
	return ascii_equal_case_insensitive(lhs.protocol(), rhs.protocol()) and
		ascii_equal_case_insensitive(lhs.host(), rhs.host()) and
		websocket_effective_port(lhs) == websocket_effective_port(rhs);
}

[[nodiscard]] inline bool websocket_redirect_status
(http::status_enum status) noexcept
{
	return status == http::status::moved_permanently or
		status == http::status::found or status == http::status::see_other or
		status == http::status::temporary_redirect or
		status == http::status::permanent_redirect;
}

template <core_concepts::exec Exec>
void close_reply_connection
(const std::shared_ptr<http::basic_reply<Exec>> &reply) noexcept
{
	if(not reply or not reply->lease().is_valid())
		return;
	if(auto connection = reply->lease().take())
	{
		ignore_unused(connection->cancel());
		ignore_unused(connection->close());
	}
}

[[nodiscard]] inline std::vector<std::byte> pending_bytes(std::string pending)
{
	std::vector<std::byte> result(pending.size());
	if(not pending.empty())
		std::memcpy(result.data(), pending.data(), pending.size());
	return result;
}

[[nodiscard]] inline std::chrono::milliseconds remaining_timeout
(std::chrono::steady_clock::time_point deadline) noexcept
{
	const auto now = std::chrono::steady_clock::now();
	if(now >= deadline)
		return std::chrono::milliseconds::zero();
	auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
		deadline - now);
	return remaining > std::chrono::milliseconds::zero() ?
		remaining : std::chrono::milliseconds(1);
}

template <core_concepts::exec Exec, http::version_enum Version>
void open_sync(http::basic_client<Exec,Version> &http_client,
	connect_request request, basic_open_diagnostics<Exec> *diagnostics,
	stream_config stream_config_value, std::chrono::milliseconds timeout,
	basic_stream<Exec> &result, error_code &error) noexcept
{
	using context_ptr = typename http::basic_client<Exec,Version>
		::template context_ptr<http::method::get>;
	context_ptr context;
	try
	{
		error = validate_open_request(request, stream_config_value);
		if(error)
			return;
		if(diagnostics)
			diagnostics->endpoint = request.endpoint;
		if(timeout <= std::chrono::milliseconds::zero())
		{
			error = asio::error::timed_out;
			return;
		}

		const auto deadline = std::chrono::steady_clock::now() + timeout;
		auto endpoint = request.endpoint;
		auto base_options = request.request_options;
		size_t redirects = 0;

		for(;;)
		{
			if(remaining_timeout(deadline) <= std::chrono::milliseconds::zero())
			{
				error = asio::error::timed_out;
				return;
			}

			std::array<std::byte,16> nonce {};
			if(auto random = secure_random_bytes(asio::buffer(nonce)); not random)
			{
				error = random.error();
				return;
			}
			auto key = make_client_key(nonce);
			if(not key)
			{
				error = key.error();
				return;
			}
			opening_request opening {
				.key = std::move(*key),
				.subprotocols = request.subprotocols
			};
			auto opening_headers = make_opening_request_headers(opening);
			if(not opening_headers)
			{
				error = opening_headers.error();
				return;
			}

			auto transport = http_transport_url(endpoint);
			if(not transport)
			{
				error = transport.error();
				return;
			}
			auto options = base_options;
			for(auto &[name, value] : *opening_headers)
				options.set_header(name, value);

			typename http::basic_client<Exec,Version>::req_info info(
				std::move(*transport), std::move(options));
			info.max_redirects = 0;
			info.auto_decompression = false;
			context = http_client.request_get(std::move(info), error);
			if(error)
				return;
			if(remaining_timeout(deadline) <= std::chrono::milliseconds::zero())
			{
				context->cancel();
				close_reply_connection(context->reply());
				error = asio::error::timed_out;
				return;
			}
			ignore_unused(context->wait_reply(error));
			if(error)
				return;
			if(remaining_timeout(deadline) <= std::chrono::milliseconds::zero())
			{
				context->cancel();
				close_reply_connection(context->reply());
				error = asio::error::timed_out;
				return;
			}

			auto reply = context->reply();
			const auto status = reply->status();
			if(websocket_redirect_status(status))
			{
				if(redirects >= request.max_redirects)
				{
					if(diagnostics)
					{
						diagnostics->endpoint = endpoint;
						diagnostics->reply = reply;
					}
					error = make_error_code(errc::redirect_limit_exceeded);
					return;
				}
				auto location = reply->header(http::header::location);
				if(not location)
				{
					if(diagnostics)
					{
						diagnostics->endpoint = endpoint;
						diagnostics->reply = reply;
					}
					error = make_error_code(errc::handshake_rejected);
					return;
				}
				auto resolved = url::resolve(endpoint, location->to_string());
				auto probe = request;
				probe.endpoint = std::move(resolved);
				error = validate_open_request(probe, stream_config_value);
				if(error)
					return;
				auto next = std::move(probe.endpoint);
				if(ascii_equal_case_insensitive(endpoint.protocol(), "wss") and
					ascii_equal_case_insensitive(next.protocol(), "ws") and
					not request.allow_insecure_redirects)
				{
					error = make_error_code(errc::insecure_redirect);
					return;
				}
				if(not same_websocket_origin(endpoint, next))
				{
					base_options.unset_header(http::header::authorization);
					base_options.unset_header("Cookie");
					base_options.cookies().clear();
				}
				endpoint = std::move(next);
				context.reset();
				++redirects;
				continue;
			}

			if(diagnostics)
			{
				diagnostics->endpoint = endpoint;
				diagnostics->reply = reply;
			}
			auto response = parse_opening_response(status, reply->headers(), opening);
			if(not response)
			{
				error = response.error();
				if(status == http::status::switching_protocols)
					close_reply_connection(reply);
				return;
			}
			if(not response->extensions.empty())
			{
				error = make_error_code(errc::unsupported_extension);
				close_reply_connection(reply);
				return;
			}

			auto pending = pending_bytes(reply->take_pending_data());
			auto connection = reply->lease().take();
			adopt_options adopt {
				.stream_role = role::client,
				.pending_data = std::move(pending),
				.negotiated_subprotocol = response->subprotocol.value_or("")
			};
			result.adopt(std::move(connection), std::move(adopt), error);
			return;
		}
	}
	catch(...)
	{
		error = exception_error(std::current_exception());
		if(context)
			close_reply_connection(context->reply());
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Handler>
auto async_open(http::basic_client<Exec,Version> &http_client,
	connect_request request, basic_open_diagnostics<Exec> *diagnostics,
	stream_config stream_config_value, std::chrono::milliseconds timeout,
	Handler &&handler)
{
	using result_t = basic_stream<Exec>;
	using token_t = std::remove_cvref_t<Handler>;
	token_t completion_token(std::forward<Handler>(handler));

	return asio::async_initiate<token_t,void(error_code,result_t)>(
		asio::co_composed<void(error_code,result_t)>([](
			auto state, http::basic_client<Exec,Version> *client,
			connect_request active_request,
			basic_open_diagnostics<Exec> *active_diagnostics,
			stream_config active_stream_config,
			std::chrono::milliseconds active_timeout) -> void
		{
			using context_ptr = typename http::basic_client<Exec,Version>
				::template context_ptr<http::method::get>;
			result_t result(client->get_executor(), active_stream_config);
			context_ptr context;
			try
			{
				auto error = validate_open_request(
					active_request, active_stream_config);
				if(error)
					co_return std::tuple<error_code,result_t>{
						error, std::move(result)};
				if(active_diagnostics)
					active_diagnostics->endpoint = active_request.endpoint;
				if(active_timeout <= std::chrono::milliseconds::zero())
				{
					co_return std::tuple<error_code,result_t>{
						asio::error::timed_out, std::move(result)};
				}

				const auto deadline = std::chrono::steady_clock::now() +
					active_timeout;
				auto endpoint = active_request.endpoint;
				auto base_options = active_request.request_options;
				size_t redirects = 0;

				for(;;)
				{
					auto remaining = remaining_timeout(deadline);
					if(remaining <= std::chrono::milliseconds::zero())
					{
						co_return std::tuple<error_code,result_t>{
							asio::error::timed_out, std::move(result)};
					}

					std::array<std::byte,16> nonce {};
					if(auto random = secure_random_bytes(asio::buffer(nonce));
						not random)
					{
						co_return std::tuple<error_code,result_t>{
							random.error(), std::move(result)};
					}
					auto key = make_client_key(nonce);
					if(not key)
					{
						co_return std::tuple<error_code,result_t>{
							key.error(), std::move(result)};
					}
					opening_request opening {
						.key = std::move(*key),
						.subprotocols = active_request.subprotocols
					};
					auto opening_headers = make_opening_request_headers(opening);
					if(not opening_headers)
					{
						co_return std::tuple<error_code,result_t>{
							opening_headers.error(), std::move(result)};
					}
					auto transport = http_transport_url(endpoint);
					if(not transport)
					{
						co_return std::tuple<error_code,result_t>{
							transport.error(), std::move(result)};
					}

					auto options = base_options;
					for(auto &[name, value] : *opening_headers)
						options.set_header(name, value);
					typename http::basic_client<Exec,Version>::req_info info(
						std::move(*transport), std::move(options));
					info.max_redirects = 0;
					info.auto_decompression = false;

					auto [request_error, next_context] =
						co_await client->request_get(
							std::move(info), asio::as_tuple(deferred));
					if(request_error)
					{
						co_return std::tuple<error_code,result_t>{
							request_error, std::move(result)};
					}
					context = std::move(next_context);
					remaining = remaining_timeout(deadline);
					if(remaining <= std::chrono::milliseconds::zero())
					{
						context->cancel();
						close_reply_connection(context->reply());
						co_return std::tuple<error_code,result_t>{
							asio::error::timed_out, std::move(result)};
					}

					auto [reply_error, status] = co_await context->wait_reply(
						asio::as_tuple(deferred));
					if(reply_error)
					{
						context->cancel();
						close_reply_connection(context->reply());
						co_return std::tuple<error_code,result_t>{
							reply_error, std::move(result)};
					}

					auto reply = context->reply();
					if(websocket_redirect_status(status))
					{
						if(redirects >= active_request.max_redirects)
						{
							if(active_diagnostics)
							{
								active_diagnostics->endpoint = endpoint;
								active_diagnostics->reply = reply;
							}
							co_return std::tuple<error_code,result_t>{
								make_error_code(errc::redirect_limit_exceeded),
								std::move(result)};
						}
						auto location = reply->header(http::header::location);
						if(not location)
						{
							if(active_diagnostics)
							{
								active_diagnostics->endpoint = endpoint;
								active_diagnostics->reply = reply;
							}
							co_return std::tuple<error_code,result_t>{
								make_error_code(errc::handshake_rejected),
								std::move(result)};
						}
						auto resolved = url::resolve(
							endpoint, location->to_string());
						auto probe = active_request;
						probe.endpoint = std::move(resolved);
						auto redirect_error = validate_open_request(
							probe, active_stream_config);
						if(redirect_error)
						{
							co_return std::tuple<error_code,result_t>{
								redirect_error, std::move(result)};
						}
						auto next = std::move(probe.endpoint);
						if(ascii_equal_case_insensitive(endpoint.protocol(), "wss") and
							ascii_equal_case_insensitive(next.protocol(), "ws") and
							not active_request.allow_insecure_redirects)
						{
							co_return std::tuple<error_code,result_t>{
								make_error_code(errc::insecure_redirect),
								std::move(result)};
						}
						if(not same_websocket_origin(endpoint, next))
						{
							base_options.unset_header(http::header::authorization);
							base_options.unset_header("Cookie");
							base_options.cookies().clear();
						}
						endpoint = std::move(next);
						context.reset();
						++redirects;
						continue;
					}

					if(active_diagnostics)
					{
						active_diagnostics->endpoint = endpoint;
						active_diagnostics->reply = reply;
					}
					auto response = parse_opening_response(
						status, reply->headers(), opening);
					if(not response)
					{
						if(status == http::status::switching_protocols)
							close_reply_connection(reply);
						co_return std::tuple<error_code,result_t>{
							response.error(), std::move(result)};
					}
					if(not response->extensions.empty())
					{
						close_reply_connection(reply);
						co_return std::tuple<error_code,result_t>{
							make_error_code(errc::unsupported_extension),
							std::move(result)};
					}

					auto pending = pending_bytes(reply->take_pending_data());
					auto connection = reply->lease().take();
					adopt_options adopt {
						.stream_role = role::client,
						.pending_data = std::move(pending),
						.negotiated_subprotocol =
							response->subprotocol.value_or("")
					};
					error_code adopt_error;
					result.adopt(std::move(connection), std::move(adopt), adopt_error);
					co_return std::tuple<error_code,result_t>{
						adopt_error, std::move(result)};
				}
			}
			catch(...)
			{
				if(context)
					close_reply_connection(context->reply());
				co_return std::tuple<error_code,result_t>{
					exception_error(std::current_exception()), std::move(result)};
			}
		}, http_client.get_executor()), completion_token, &http_client,
		std::move(request), diagnostics, stream_config_value, timeout);
}

} //namespace detail

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, connect_request request,
	Token &&token)
requires (Version == http::version::v11) and
	concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	const auto stream_options = request.stream_options.value_or(stream_config{});
	const auto timeout = request.handshake_timeout.value_or(
		detail::default_handshake_timeout);
	if constexpr(is_error_code_token_v<Token>)
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		detail::open_sync(http_client, std::move(request),
			static_cast<basic_open_diagnostics<Exec>*>(nullptr),
			stream_options, timeout, result, token);
		return result;
	}
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		error_code error;
		detail::open_sync(http_client, std::move(request),
			static_cast<basic_open_diagnostics<Exec>*>(nullptr),
			stream_options, timeout, result, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::open");
		return result;
	}
	else
	{
		return detail::initiate_handshake_io<basic_stream<Exec>>(
			http_client.get_executor(),
			[&http_client, request = std::move(request), stream_options, timeout]
			<typename Handler>(Handler &&handler) mutable
			{
				detail::async_open(http_client, std::move(request),
					static_cast<basic_open_diagnostics<Exec>*>(nullptr),
					stream_options, timeout, std::forward<Handler>(handler));
			}, timeout, [exec = http_client.get_executor(), stream_options] {
				return basic_stream<Exec>(exec, stream_options);
			}, unbound_redirect_time(std::forward<Token>(token)));
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, connect_request request,
	basic_open_diagnostics<Exec> &diagnostics, Token &&token)
requires (Version == http::version::v11) and
	concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	diagnostics.endpoint = request.endpoint;
	diagnostics.reply.reset();
	const auto stream_options = request.stream_options.value_or(stream_config{});
	const auto timeout = request.handshake_timeout.value_or(
		detail::default_handshake_timeout);
	if constexpr(is_error_code_token_v<Token>)
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		detail::open_sync(http_client, std::move(request), &diagnostics,
			stream_options, timeout, result, token);
		return result;
	}
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		error_code error;
		detail::open_sync(http_client, std::move(request), &diagnostics,
			stream_options, timeout, result, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::open");
		return result;
	}
	else
	{
		return detail::initiate_handshake_io<basic_stream<Exec>>(
			http_client.get_executor(),
			[&http_client, request = std::move(request), &diagnostics,
			 stream_options, timeout]<typename Handler>(Handler &&handler) mutable
			{
				detail::async_open(http_client, std::move(request), &diagnostics,
					stream_options, timeout, std::forward<Handler>(handler));
			}, timeout, [exec = http_client.get_executor(), stream_options] {
				return basic_stream<Exec>(exec, stream_options);
			}, unbound_redirect_time(std::forward<Token>(token)));
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, url endpoint,
	Token &&token)
requires (Version == http::version::v11) and
	concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	return open(http_client, connect_request(std::move(endpoint)),
		std::forward<Token>(token));
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, url endpoint,
	basic_open_diagnostics<Exec> &diagnostics, Token &&token)
requires (Version == http::version::v11) and
	concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	return open(http_client, connect_request(std::move(endpoint)), diagnostics,
		std::forward<Token>(token));
}

} //namespace libgs::websocket

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif

#endif //LIBGS_WEBSOCKET_DETAIL_CLIENT_H
