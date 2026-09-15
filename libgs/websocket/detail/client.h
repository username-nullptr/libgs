// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_CLIENT_H
#define LIBGS_WEBSOCKET_DETAIL_CLIENT_H

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

#include <libgs/websocket/protocol/handshake.h>
#include <libgs/http/client/detail/proxy.h>

#include <libgs/websocket/detail/permessage_deflate.h>
#include <libgs/websocket/detail/secure_random.h>
#include <libgs/websocket/detail/handshake_io.h>

namespace libgs::websocket { namespace detail
{

constexpr std::chrono::milliseconds default_handshake_timeout {30000};

[[nodiscard]] LIBGS_WEBSOCKET_API
bool ascii_equal_case_insensitive(std::string_view lhs, std::string_view rhs) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
bool websocket_owned_request_header(std::string_view name) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
sys_expected<url> canonical_websocket_url(const url &endpoint) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
error_code validate_open_request(connect_request &request, const stream_config &stream) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
sys_expected<url> http_transport_url(const url &endpoint) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
uint16_t websocket_effective_port(const url &value) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
bool same_websocket_origin(const url &lhs, const url &rhs) noexcept;

[[nodiscard]] LIBGS_WEBSOCKET_API
bool websocket_redirect_status(http::status_enum status) noexcept;

template <core_concepts::exec Exec>
LIBGS_WEBSOCKET_TAPI void close_reply_connection
(const std::shared_ptr<http::basic_reply<Exec>> &reply) noexcept
{
	if( not reply or not reply->lease().is_valid() )
		return ;

	if( auto connection = reply->lease().take() )
	{
		ignore_unused(connection->cancel());
		ignore_unused(connection->close());
	}
}

template <typename ConnectionPtr>
[[nodiscard]] LIBGS_WEBSOCKET_TAPI error_code configure_client_connection
(const ConnectionPtr &connection, optional<bool> no_delay) noexcept
{
	if( not connection )
		return make_error_code(std::errc::not_connected);

	if( not no_delay )
		return {};

	http::tcp_socket_options options;
	options.no_delay = *no_delay;

	auto configured = connection->set_options(options);
	return configured ? error_code{} : configured.error();
}

[[nodiscard]] LIBGS_WEBSOCKET_API
std::vector<std::byte> pending_bytes(std::string pending);

[[nodiscard]] LIBGS_WEBSOCKET_API
std::chrono::milliseconds remaining_timeout(std::chrono::steady_clock::time_point deadline) noexcept;

template <typename RequestInfo>
[[nodiscard]] error_code configure_proxy_request
(const optional<proxy_t> &proxy, const url &endpoint, RequestInfo &info, bool inherit_global_proxy) noexcept
{
	const bool use_global = proxy ?
		std::holds_alternative<use_global_proxy_t>(*proxy) : inherit_global_proxy;

	if( not proxy and not use_global )
		return {};
	try {
		if( use_global )
		{
			auto resolved = [&]() -> sys_expected<http::detail::resolved_proxy>
			{
				if( ascii_equal_case_insensitive(endpoint.protocol(), "ws") )
				{
					return http::detail::resolve_global_proxy(endpoint, {
						"ws_proxy", "WS_PROXY", "http_proxy", "HTTP_PROXY",
						"all_proxy", "ALL_PROXY"
					});
				}
				if( ascii_equal_case_insensitive(endpoint.protocol(), "wss") )
				{
					return http::detail::resolve_global_proxy(endpoint, {
						"wss_proxy", "WSS_PROXY", "https_proxy", "HTTPS_PROXY",
						"all_proxy", "ALL_PROXY"
					});
				}
				return sys_unexpected (
					make_error_code(std::errc::protocol_not_supported)
				);
			}();

			if( not resolved )
				return resolved.error();

			if( resolved->forward )
			{
				info.proxy = std::move(*resolved->forward);
				if( resolved->authorization and
					info.arg.headers().find(http::header::proxy_authorization) == info.arg.headers().end() )
				{
					info.arg.set_header(http::header::proxy_authorization,
						*resolved->authorization
					);
				}
			}
			else if( resolved->tunnel )
				info.proxy = std::move(*resolved->tunnel);
			else
				info.proxy = no_proxy;
			return {};
		}
		if( std::holds_alternative<no_proxy_t>(*proxy) )
		{
			info.proxy = no_proxy;
			return {};
		}
		const auto &config = std::get<proxy_config>(*proxy);
		const auto endpoint_scheme = strtls::to_lower(endpoint.protocol());
		const auto proxy_scheme = strtls::to_lower(config.endpoint.protocol());

		if( config.type == proxy_type::http and endpoint_scheme == "ws" )
		{
			if( config.authorization )
				info.arg.set_header(http::header::proxy_authorization, *config.authorization);

			info.proxy = config.endpoint;
			return {};
		}
		http::proxy_tunnel tunnel;
		tunnel.type = config.type == proxy_type::http ?
			http::proxy_tunnel_type::http_connect :
			http::proxy_tunnel_type::socks5;

		tunnel.host = config.endpoint.host();
		tunnel.port = config.endpoint.port();

		if( tunnel.port == 0 )
		{
			tunnel.port = config.type == proxy_type::socks5 ? 1080 :
				proxy_scheme == "https" ? 443 : 80;
		}
		tunnel.security = proxy_scheme == "https" ?
			http::security_mode::tls : http::security_mode::plain;

		tunnel.authorization = config.authorization;
		tunnel.username = config.username;
		tunnel.password = config.password;

		info.proxy = std::move(tunnel);
		return {};
	}
	catch(const std::bad_alloc&) {
		return make_error_code(std::errc::not_enough_memory);
	}
	catch(...) {}
	return make_error_code(std::errc::io_error);
}

template <core_concepts::exec Exec, http::version_enum Version>
void open_sync(http::basic_client<Exec,Version> &http_client, connect_request request,
	basic_open_diagnostics<Exec> *diagnostics, stream_config stream_config_value,
	optional<bool> no_delay, std::chrono::milliseconds timeout,
	basic_stream<Exec> &result, error_code &error) noexcept
{
	using context_ptr = http::basic_client<Exec,Version>::
		template context_ptr<http::method::get>;

	context_ptr context;
	try {
		error = validate_open_request(request, stream_config_value);
		if( error )
			return ;

		if( diagnostics )
			diagnostics->endpoint = request.endpoint;

		if( timeout <= std::chrono::milliseconds::zero() )
		{
			error = asio::error::timed_out;
			return ;
		}
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		auto base_options = request.request_options;
		auto endpoint = request.endpoint;
		size_t redirects = 0;
		for(;;)
		{
			if( remaining_timeout(deadline) <= std::chrono::milliseconds::zero() )
			{
				error = asio::error::timed_out;
				return ;
			}
			std::array<std::byte,16> nonce {};
			if( auto random = secure_random_bytes(asio::buffer(nonce)); not random )
			{
				error = random.error();
				return ;
			}
			auto key = make_client_key(nonce);
			if( not key )
			{
				error = key.error();
				return ;
			}
			opening_request opening {
				.key = std::move(*key),
				.subprotocols = request.subprotocols,
				.extensions = request.extensions,
			};
			auto opening_headers = make_opening_request_headers(opening);
			if( not opening_headers )
			{
				error = opening_headers.error();
				return ;
			}
			auto transport = http_transport_url(endpoint);
			if( not transport )
			{
				error = transport.error();
				return ;
			}
			auto options = base_options;
			for(auto &[name, value] : *opening_headers)
				options.set_header(name, value);

			typename http::basic_client<Exec,Version>::req_info info(
				std::move(*transport), std::move(options)
			);
			info.max_redirects = 0;
			info.auto_decompression = false;

			const bool inherit_global_proxy =
				std::holds_alternative<use_global_proxy_t>(http_client.config().default_proxy);

			if( auto proxy_error = configure_proxy_request
				(request.proxy, endpoint, info, inherit_global_proxy) )
			{
				error = proxy_error;
				return ;
			}
			context = http_client.request_get(std::move(info), error);
			if( error )
				return ;

			if( remaining_timeout(deadline) <= std::chrono::milliseconds::zero() )
			{
				context->cancel();
				close_reply_connection(context->reply());
				error = asio::error::timed_out;
				return ;
			}
			ignore_unused(context->wait_reply(error));
			if( error )
				return ;

			if( remaining_timeout(deadline) <= std::chrono::milliseconds::zero() )
			{
				context->cancel();
				close_reply_connection(context->reply());
				error = asio::error::timed_out;
				return ;
			}
			auto reply = context->reply();
			const auto status = reply->status();

			if( websocket_redirect_status(status) )
			{
				if( redirects >= request.max_redirects )
				{
					if( diagnostics )
					{
						diagnostics->endpoint = endpoint;
						diagnostics->reply = reply;
					}
					error = make_error_code(errc::redirect_limit_exceeded);
					return ;
				}
				auto location = reply->header(http::header::location);
				if( not location )
				{
					if( diagnostics )
					{
						diagnostics->endpoint = endpoint;
						diagnostics->reply = reply;
					}
					error = make_error_code(errc::handshake_rejected);
					return ;
				}
				auto resolved = url::resolve(endpoint, location->to_string());
				auto probe = request;

				probe.endpoint = std::move(resolved);
				error = validate_open_request(probe, stream_config_value);
				if( error )
					return ;

				auto next = std::move(probe.endpoint);
				if( ascii_equal_case_insensitive(endpoint.protocol(), "wss") and
					ascii_equal_case_insensitive(next.protocol(), "ws") and
					not request.allow_insecure_redirects )
				{
					error = make_error_code(errc::insecure_redirect);
					return ;
				}
				if( not same_websocket_origin(endpoint, next) )
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
			if( diagnostics )
			{
				diagnostics->endpoint = endpoint;
				diagnostics->reply = reply;
			}
			auto response = parse_opening_response(status, reply->headers(), opening);
			if( not response )
			{
				error = response.error();
				if( status == http::status::switching_protocols )
					close_reply_connection(reply);
				return ;
			}
			if( not detail::supported_extension_response(response->extensions, request.extensions) )
			{
				error = make_error_code(errc::unsupported_extension);
				close_reply_connection(reply);
				return ;
			}
			auto pending = pending_bytes(reply->take_pending_data());
			auto connection = reply->lease().take();

			error = configure_client_connection(connection, no_delay);
			if( error )
			{
				if( connection )
					ignore_unused(connection->close());
				return ;
			}
			adopt_options adopt {
				.stream_role = role::client,
				.pending_data = std::move(pending),
				.negotiated_subprotocol = response->subprotocol.value_or(""),
				.negotiated_extensions = response->extensions,
			};
			result.adopt(std::move(connection), std::move(adopt), error);
			return ;
		}
	}
	catch(...)
	{
		error = exception_error(std::current_exception());
		if( context )
			close_reply_connection(context->reply());
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Handler>
auto async_open(http::basic_client<Exec,Version> &http_client, connect_request request,
	basic_open_diagnostics<Exec> *diagnostics, stream_config stream_config_value,
	optional<bool> no_delay, std::chrono::milliseconds timeout, Handler &&handler)
{
	using result_t = basic_stream<Exec>;
	using token_t = std::remove_cvref_t<Handler>;
	token_t completion_token(std::forward<Handler>(handler));

	return asio::async_initiate<token_t,void(error_code,result_t)>(
		asio::co_composed<void(error_code,result_t)>([](
			auto state, http::basic_client<Exec,Version> *client, connect_request active_request,
			basic_open_diagnostics<Exec> *active_diagnostics, stream_config active_stream_config,
			optional<bool> active_no_delay, std::chrono::milliseconds active_timeout) -> void
		{
			LIBGS_UNUSED(state);
			using context_ptr = http::basic_client<Exec,Version>::
				template context_ptr<http::method::get>;

			result_t result(client->get_executor(), active_stream_config);
			context_ptr context;
			try {
				if( auto error = validate_open_request(active_request, active_stream_config) )
				{
					co_return std::tuple<error_code,result_t> {
						error, std::move(result)
					};
				}
				if( active_diagnostics )
					active_diagnostics->endpoint = active_request.endpoint;

				if( active_timeout <= std::chrono::milliseconds::zero() )
				{
					co_return std::tuple<error_code,result_t> {
						asio::error::timed_out, std::move(result)
					};
				}
				const auto deadline = std::chrono::steady_clock::now() + active_timeout;
				auto base_options = active_request.request_options;
				auto endpoint = active_request.endpoint;
				size_t redirects = 0;
				for(;;)
				{
					auto remaining = remaining_timeout(deadline);
					if( remaining <= std::chrono::milliseconds::zero() )
					{
						co_return std::tuple<error_code,result_t> {
							asio::error::timed_out, std::move(result)
						};
					}
					std::array<std::byte,16> nonce {};
					if( auto random = secure_random_bytes(asio::buffer(nonce)); not random )
					{
						co_return std::tuple<error_code,result_t> {
							random.error(), std::move(result)
						};
					}
					auto key = make_client_key(nonce);
					if( not key )
					{
						co_return std::tuple<error_code,result_t> {
							key.error(), std::move(result)
						};
					}
					opening_request opening {
						.key = std::move(*key),
						.subprotocols = active_request.subprotocols,
						.extensions = active_request.extensions,
					};
					auto opening_headers = make_opening_request_headers(opening);
					if( not opening_headers )
					{
						co_return std::tuple<error_code,result_t> {
							opening_headers.error(), std::move(result)
						};
					}
					auto transport = http_transport_url(endpoint);
					if( not transport )
					{
						co_return std::tuple<error_code,result_t> {
							transport.error(), std::move(result)
						};
					}
					auto options = base_options;
					for(auto &[name, value] : *opening_headers)
						options.set_header(name, value);

					typename http::basic_client<Exec,Version>::req_info info (
						std::move(*transport), std::move(options)
					);
					info.auto_decompression = false;
					info.max_redirects = 0;

					const bool inherit_global_proxy =
						std::holds_alternative<use_global_proxy_t>(client->config().default_proxy);

					if( auto proxy_error = configure_proxy_request
						(active_request.proxy, endpoint, info, inherit_global_proxy) )
					{
						co_return std::tuple<error_code,result_t> {
							proxy_error, std::move(result)
						};
					}
					auto [request_error, next_context] = co_await client->request_get (
						std::move(info), asio::as_tuple(deferred)
					);
					if( request_error )
					{
						co_return std::tuple<error_code,result_t> {
							request_error, std::move(result)
						};
					}
					context = std::move(next_context);
					remaining = remaining_timeout(deadline);

					if( remaining <= std::chrono::milliseconds::zero() )
					{
						context->cancel();
						close_reply_connection(context->reply());

						co_return std::tuple<error_code,result_t> {
							asio::error::timed_out, std::move(result)
						};
					}
					auto [reply_error, status] = co_await context->wait_reply (
						asio::as_tuple(deferred)
					);
					if( reply_error )
					{
						context->cancel();
						close_reply_connection(context->reply());

						co_return std::tuple<error_code,result_t> {
							reply_error, std::move(result)
						};
					}
					auto reply = context->reply();
					if( websocket_redirect_status(status) )
					{
						if( redirects >= active_request.max_redirects )
						{
							if( active_diagnostics )
							{
								active_diagnostics->endpoint = endpoint;
								active_diagnostics->reply = reply;
							}
							co_return std::tuple<error_code,result_t> {
								make_error_code(errc::redirect_limit_exceeded),
								std::move(result)
							};
						}
						auto location = reply->header(http::header::location);
						if( not location )
						{
							if( active_diagnostics )
							{
								active_diagnostics->endpoint = endpoint;
								active_diagnostics->reply = reply;
							}
							co_return std::tuple<error_code,result_t> {
								make_error_code(errc::handshake_rejected),
								std::move(result)
							};
						}
						auto resolved = url::resolve(endpoint, location->to_string());
						auto probe = active_request;
						probe.endpoint = std::move(resolved);

						if( auto redirect_error = validate_open_request(probe, active_stream_config) )
						{
							co_return std::tuple<error_code,result_t> {
								redirect_error, std::move(result)
							};
						}
						auto next = std::move(probe.endpoint);

						if( ascii_equal_case_insensitive(endpoint.protocol(), "wss") and
							ascii_equal_case_insensitive(next.protocol(), "ws") and
							not active_request.allow_insecure_redirects )
						{
							co_return std::tuple<error_code,result_t> {
								make_error_code(errc::insecure_redirect),
								std::move(result)
							};
						}
						if( not same_websocket_origin(endpoint, next) )
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
					if( active_diagnostics )
					{
						active_diagnostics->endpoint = endpoint;
						active_diagnostics->reply = reply;
					}
					auto response = parse_opening_response(status, reply->headers(), opening);
					if( not response )
					{
						if( status == http::status::switching_protocols )
							close_reply_connection(reply);

						co_return std::tuple<error_code,result_t> {
							response.error(), std::move(result)
						};
					}
					if( not detail::supported_extension_response
						(response->extensions, active_request.extensions) )
					{
						close_reply_connection(reply);
						co_return std::tuple<error_code,result_t> {
							make_error_code(errc::unsupported_extension),
							std::move(result)
						};
					}
					auto pending = pending_bytes(reply->take_pending_data());
					auto connection = reply->lease().take();

					auto option_error = configure_client_connection (
						connection, active_no_delay
					);
					if( option_error )
					{
						if( connection )
							ignore_unused(connection->close());
						co_return std::tuple<error_code,result_t> {
							option_error, std::move(result)
						};
					}
					adopt_options adopt {
						.stream_role = role::client,
						.pending_data = std::move(pending),
						.negotiated_subprotocol = response->subprotocol.value_or(""),
						.negotiated_extensions = response->extensions,
					};
					error_code adopt_error;
					result.adopt(std::move(connection), std::move(adopt), adopt_error);

					co_return std::tuple<error_code,result_t> {
						adopt_error, std::move(result)
					};
				}
			}
			catch(...)
			{
				if( context )
					close_reply_connection(context->reply());

				co_return std::tuple<error_code,result_t> {
					exception_error(std::current_exception()), std::move(result)
				};
			}
		},
		http_client.get_executor()), completion_token, &http_client,
		std::move(request), diagnostics, stream_config_value, no_delay, timeout
	);
}

} //namespace detail

template <core_concepts::exec Exec>
class LIBGS_WEBSOCKET_TAPI basic_client<Exec>::impl :
	public std::enable_shared_from_this<impl>
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	struct open_operation
	{
		asio::cancellation_signal cancellation {};
		std::atomic_bool finished {false};
	};

public:
	impl() requires core_concepts::match_sched<io_executor_t,executor_t> :
		m_http_client(), m_config(validate_config({})) {}

	explicit impl(const config_t &config) requires
		core_concepts::match_sched<io_executor_t,executor_t> :
		m_http_client(), m_config(validate_config(config)) {}

	explicit impl(const core_concepts::match_exec<executor_t> auto &exec, const config_t &config) :
		m_http_client(exec), m_config(validate_config(config)) {}

	explicit impl(http_client_t &&http_client, const config_t &config) :
		m_http_client(std::move(http_client)), m_config(validate_config(config)) {}

private:
	[[nodiscard]] static config_t validate_config(config_t config)
	{
		if( config.stream.read_buffer_size == 0 or
			config.stream.ping_interval < std::chrono::milliseconds::zero() )
		{
			system_error::loc_throw (
				make_error_code(std::errc::invalid_argument),
				"libgs::websocket::basic_client"
			);
		}
		return config;
	}

	void prepare_request(connect_request_t &request) const
	{
		if( not request.proxy )
			request.proxy = m_config.default_proxy;

		if( not request.stream_options )
			request.stream_options = m_config.stream;

		if( not request.handshake_timeout )
			request.handshake_timeout = m_config.handshake_timeout;
	}

	void retain_operation(const std::shared_ptr<open_operation> &operation)
	{
		std::lock_guard lock(m_mutex);
		m_operations.erase (
			std::remove_if (
				m_operations.begin(), m_operations.end(),
				[](const auto &item) { return item.expired(); }
			),
			m_operations.end()
		);
		m_operations.emplace_back(operation);
		m_pending_open_count.fetch_add(1, std::memory_order_relaxed);
	}

	void release_operation(const std::shared_ptr<open_operation> &operation) noexcept
	{
		if( operation->finished.exchange(true, std::memory_order_acq_rel) )
			return ;

		m_pending_open_count.fetch_sub(1, std::memory_order_relaxed);
		std::lock_guard lock(m_mutex);

		m_operations.erase(std::remove_if (
			m_operations.begin(), m_operations.end(),
			[&](const auto &item)
			{
				auto value = item.lock();
				return not value or value == operation;
			}),
			m_operations.end()
		);
	}

public:
	template <typename Token>
	[[nodiscard]] auto async_open
	(connect_request_t request, diagnostics_t *diagnostics, Token &&token)
	{
		using token_t = std::remove_cvref_t<Token>;
		token_t completion_token(std::forward<Token>(token));

		prepare_request(request);
		auto self = this->shared_from_this();

		return asio::async_initiate<token_t,void(error_code,stream_t)>(
		[self = std::move(self), request = std::move(request), diagnostics]
		(auto completion_handler) mutable
		{
			auto operation = std::make_shared<open_operation>();
			auto slot = asio::get_associated_cancellation_slot(completion_handler);

			auto completion_exec = asio::get_associated_executor (
				completion_handler, self->m_http_client.get_executor()
			);
			auto allocator = asio::get_associated_allocator(completion_handler);
			self->retain_operation(operation);

			if( slot.is_connected() )
			{
				std::weak_ptr<open_operation> weak_operation = operation;
				slot.assign([weak_operation](asio::cancellation_type type) noexcept
				{
					if( type == asio::cancellation_type::none )
						return ;

					if( auto active = weak_operation.lock() )
						active->cancellation.emit(type);
				});
			}
			auto completion = asio::bind_cancellation_slot(operation->cancellation.slot(),
			[self, operation, slot, completion_exec, allocator, handler = std::move(completion_handler)]
			(error_code error, stream_t stream) mutable
			{
				if( slot.is_connected() )
					slot.clear();

				self->release_operation(operation);
				asio::post(completion_exec, asio::bind_allocator(allocator,
				[handler = std::move(handler), error, stream = std::move(stream)]() mutable {
					std::move(handler)(error, std::move(stream));
				}));
			});

			const auto stream_options = *request.stream_options;
			const auto timeout = *request.handshake_timeout;
			const auto no_delay = self->m_config.no_delay;

			detail::initiate_handshake_io<stream_t>(self->m_http_client.get_executor(),
				[client = &self->m_http_client, request = std::move(request), diagnostics,
				 stream_options, no_delay, timeout]<typename Handler>(Handler &&handler) mutable
				{
					detail::async_open(*client, std::move(request), diagnostics,
						stream_options, no_delay, timeout,
						std::forward<Handler>(handler)
					);
				},
				timeout,
				[exec = self->m_http_client.get_executor(), stream_options] {
					return stream_t(exec, stream_options);
				},
				std::move(completion)
			);
		},
		completion_token);
	}

	[[nodiscard]] stream_t open_sync
	(connect_request_t request, diagnostics_t *diagnostics, error_code &error) noexcept
	{
		try {
			prepare_request(request);
			m_pending_open_count.fetch_add(1, std::memory_order_relaxed);

			struct counter_guard
			{
				std::atomic_size_t &counter;
				~counter_guard() {
					counter.fetch_sub(1, std::memory_order_relaxed);
				}
			}
			guard {m_pending_open_count};

			const auto stream_options = *request.stream_options;
			const auto timeout = *request.handshake_timeout;
			stream_t result(m_http_client.get_executor(), stream_options);

			detail::open_sync(m_http_client, std::move(request), diagnostics,
				stream_options, m_config.no_delay, timeout, result, error
			);
			return result;
		}
		catch(...) {
			error = exception_error(std::current_exception());
		}
		return stream_t(m_http_client.get_executor(), m_config.stream);
	}

	void cancel() noexcept
	{
		std::vector<std::shared_ptr<open_operation>> operations;
		{
			std::lock_guard lock(m_mutex);
			operations.reserve(m_operations.size());

			for(auto &item : m_operations)
			{
				if( auto operation = item.lock() )
					operations.emplace_back(std::move(operation));
			}
		}
		for(auto &operation : operations)
			operation->cancellation.emit(asio::cancellation_type::all);
	}

public:
	http_client_t m_http_client;
	config_t m_config {};

	std::atomic_size_t m_pending_open_count {0};
	std::mutex m_mutex;

	std::vector <
		std::weak_ptr<open_operation>
	> m_operations {};
};

template <core_concepts::exec Exec>
basic_client<Exec>::basic_client() requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>())
{

}

template <core_concepts::exec Exec>
basic_client<Exec>::basic_client(config_t config) requires
	core_concepts::match_sched<io_executor_t,executor_t> :
	m_impl(std::make_shared<impl>(std::move(config)))
{

}

template <core_concepts::exec Exec>
template <typename Exec0>
basic_client<Exec>::basic_client(Exec0 &&exec, config_t config) requires
(not std::same_as<std::remove_cvref_t<Exec0>,basic_client> and core_concepts::match_sched<Exec0,executor_t>) :
	m_impl(std::make_shared<impl>(get_executor_helper(std::forward<Exec0>(exec)), std::move(config)))
{

}

template <core_concepts::exec Exec>
basic_client<Exec>::basic_client(http_client_t &&http_client, config_t config) :
	m_impl(std::make_shared<impl>(std::move(http_client), std::move(config)))
{

}

template <core_concepts::exec Exec>
basic_client<Exec>::basic_client(basic_client &&other) noexcept :
	m_impl(std::move(other.m_impl))
{

}

template <core_concepts::exec Exec>
basic_client<Exec> &basic_client<Exec>::operator=(basic_client &&other) noexcept
{
	m_impl = std::move(other.m_impl);
	return *this;
}

template <core_concepts::exec Exec>
basic_client<Exec>::~basic_client() = default;

template <core_concepts::exec Exec>
template <typename Token>
auto basic_client<Exec>::open(connect_request_t request, Token &&token)
	requires open_token_v<Token>
{
	if constexpr( is_error_code_token_v<Token> )
		return m_impl->open_sync(std::move(request), nullptr, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto result = m_impl->open_sync(std::move(request), nullptr, error);
		if( error )
			system_error::loc_throw(error, "libgs::websocket::basic_client::open");
		return result;
	}
	else
	{
		return m_impl->async_open(std::move(request), nullptr,
			std::forward<Token>(token)
		);
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_client<Exec>::open(connect_request_t request, diagnostics_t &diagnostics, Token &&token)
	requires open_token_v<Token>
{
	diagnostics.endpoint = request.endpoint;
	diagnostics.reply.reset();

	if constexpr( is_error_code_token_v<Token> )
		return m_impl->open_sync(std::move(request), &diagnostics, token);

	else if constexpr( is_sync_opt_token_v<Token> )
	{
		error_code error;
		auto result = m_impl->open_sync(std::move(request), &diagnostics, error);
		if( error )
			system_error::loc_throw(error, "libgs::websocket::basic_client::open");
		return result;
	}
	else
	{
		return m_impl->async_open(std::move(request), &diagnostics,
			std::forward<Token>(token)
		);
	}
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_client<Exec>::open(url endpoint, Token &&token)
	requires open_token_v<Token>
{
	return open(connect_request_t(std::move(endpoint)), std::forward<Token>(token));
}

template <core_concepts::exec Exec>
template <typename Token>
auto basic_client<Exec>::open(url endpoint, diagnostics_t &diagnostics, Token &&token)
	requires open_token_v<Token>
{
	return open(connect_request_t(std::move(endpoint)), diagnostics,
		std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec>
std::shared_ptr<http::cookie_jar> basic_client<Exec>::cookie_store() noexcept
{
	return m_impl->m_http_client.cookie_store();
}

template <core_concepts::exec Exec>
size_t basic_client<Exec>::pending_open_count() const noexcept
{
	return m_impl->m_pending_open_count.load(std::memory_order_relaxed);
}

template <core_concepts::exec Exec>
auto basic_client<Exec>::config() const -> config_t
{
	return m_impl->m_config;
}

template <core_concepts::exec Exec>
auto basic_client<Exec>::http_client() const noexcept -> const http_client_t&
{
	return m_impl->m_http_client;
}

template <core_concepts::exec Exec>
auto basic_client<Exec>::http_client() noexcept -> http_client_t&
{
	return m_impl->m_http_client;
}

template <core_concepts::exec Exec>
auto basic_client<Exec>::get_executor() const noexcept -> executor_t
{
	return m_impl->m_http_client.get_executor();
}

template <core_concepts::exec Exec>
basic_client<Exec> &basic_client<Exec>::cancel() noexcept
{
	m_impl->cancel();
	return *this;
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, connect_request request, Token &&token) requires
	(Version == http::version::v11) and concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	const auto stream_options = request.stream_options.value_or(stream_config{});
	const auto timeout = request.handshake_timeout.value_or(detail::default_handshake_timeout);

	if constexpr( is_error_code_token_v<Token> )
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		detail::open_sync(http_client, std::move(request),
			static_cast<basic_open_diagnostics<Exec>*>(nullptr),
			stream_options, nullopt, timeout, result, token
		);
		return result;
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		error_code error;

		detail::open_sync(http_client, std::move(request),
			static_cast<basic_open_diagnostics<Exec>*>(nullptr),
			stream_options, nullopt, timeout, result, error
		);
		if( error )
			system_error::loc_throw(error, "libgs::websocket::open");
		return result;
	}
	else
	{
		return detail::initiate_handshake_io<basic_stream<Exec>>(http_client.get_executor(),
			[&http_client, request = std::move(request), stream_options, timeout]
			<typename Handler>(Handler &&handler) mutable
			{
				detail::async_open(http_client, std::move(request),
					static_cast<basic_open_diagnostics<Exec>*>(nullptr),
					stream_options, nullopt, timeout, std::forward<Handler>(handler)
				);
			},
			timeout,
			[exec = http_client.get_executor(), stream_options]{
				return basic_stream<Exec>(exec, stream_options);
			},
			unbound_redirect_time(std::forward<Token>(token))
		);
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, connect_request request,
	basic_open_diagnostics<Exec> &diagnostics, Token &&token) requires
	(Version == http::version::v11) and concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	diagnostics.endpoint = request.endpoint;
	diagnostics.reply.reset();

	const auto stream_options = request.stream_options.value_or(stream_config{});
	const auto timeout = request.handshake_timeout.value_or(detail::default_handshake_timeout);

	if constexpr( is_error_code_token_v<Token> )
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		detail::open_sync(http_client, std::move(request), &diagnostics,
			stream_options, nullopt, timeout, result, token
		);
		return result;
	}
	else if constexpr( is_sync_opt_token_v<Token> )
	{
		basic_stream<Exec> result(http_client.get_executor(), stream_options);
		error_code error;

		detail::open_sync(http_client, std::move(request), &diagnostics,
			stream_options, nullopt, timeout, result, error
		);
		if( error )
			system_error::loc_throw(error, "libgs::websocket::open");
		return result;
	}
	else
	{
		return detail::initiate_handshake_io<basic_stream<Exec>>(http_client.get_executor(),
			[&http_client, request = std::move(request), &diagnostics, stream_options, timeout]
			<typename Handler>(Handler &&handler) mutable
			{
				detail::async_open(http_client, std::move(request), &diagnostics,
					stream_options, nullopt, timeout, std::forward<Handler>(handler)
				);
			},
			timeout,
			[exec = http_client.get_executor(), stream_options]{
				return basic_stream<Exec>(exec, stream_options);
			},
			unbound_redirect_time(std::forward<Token>(token))
		);
	}
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, url endpoint, Token &&token) requires
	(Version == http::version::v11) and concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	return open(http_client, connect_request(std::move(endpoint)),
		std::forward<Token>(token)
	);
}

template <core_concepts::exec Exec, http::version_enum Version, typename Token>
auto open(http::basic_client<Exec,Version> &http_client, url endpoint,
	basic_open_diagnostics<Exec> &diagnostics, Token &&token) requires
	(Version == http::version::v11) and concepts::dis_detach_opt_token<Token,error_code,basic_stream<Exec>>
{
	return open(http_client, connect_request(std::move(endpoint)),
		diagnostics, std::forward<Token>(token)
	);
}

} //namespace libgs::websocket

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif

#endif //LIBGS_WEBSOCKET_DETAIL_CLIENT_H
