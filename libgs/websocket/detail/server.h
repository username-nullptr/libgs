// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_SERVER_H
#define LIBGS_WEBSOCKET_DETAIL_SERVER_H

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

#include <libgs/websocket/detail/handshake_io.h>
#include <libgs/websocket/protocol/handshake.h>
#include <algorithm>
#include <cstring>

namespace libgs::websocket { namespace detail
{

inline constexpr const char
	* sec_websocket_accept = "Sec-WebSocket-Accept",
	* sec_websocket_version = "Sec-WebSocket-Version",
	* sec_websocket_protocol = "Sec-WebSocket-Protocol",
	* sec_websocket_extensions = "Sec-WebSocket-Extensions";

[[nodiscard]] inline bool server_ascii_equal_case_insensitive
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

[[nodiscard]] inline bool server_deadline_expired
(std::chrono::steady_clock::time_point deadline) noexcept
{
	return std::chrono::steady_clock::now() >= deadline;
}

[[nodiscard]] inline std::chrono::milliseconds server_remaining_timeout
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

template <core_concepts::exec Exec>
void close_upgrade_connection(http::basic_service_context<Exec> &context) noexcept
{
	context.request().cancel();
	context.response().cancel();
	ignore_unused(context.request().connection().cancel());
	ignore_unused(context.request().connection().close());
}

template <core_concepts::exec Exec>
[[nodiscard]] request_info snapshot_request
(const http::basic_request<Exec> &request)
{
	return request_info {
		.method = request.method(),
		.version = request.version(),
		.target = std::string(request.target()),
		.path = std::string(request.path()),
		.request_headers = request.headers(),
		.query_parameters = request.parameters(),
		.path_arguments = request.path_args(),
		.remote_endpoint = request.remote_endpoint(),
		.local_endpoint = request.local_endpoint()
	};
}

[[nodiscard]] inline bool valid_rejection_status(http::status_enum status) noexcept
{
	const auto value = static_cast<uint32_t>(status);
	return value >= 200 and value <= 599 and
		status != http::status::switching_protocols;
}

inline void erase_protocol_response_headers(http::headers &headers) noexcept
{
	headers.erase(http::header::connection);
	headers.erase(http::header::upgrade);
	headers.erase(sec_websocket_accept);
	headers.erase(sec_websocket_version);
	headers.erase(sec_websocket_protocol);
	headers.erase(sec_websocket_extensions);
	headers.erase(http::header::content_length);
	headers.erase(http::header::transfer_encoding);
}

template <core_concepts::exec Exec>
struct server_upgrade_plan
{
	request_info request {};
	opening_request opening {};
	opening_response response {};
	http::status_enum status = http::status::bad_request;
	http::headers headers {};
	std::string body {};
	error_code error {};
	bool accepted = false;
	bool preserve_error_on_write_failure = false;
};

template <core_concepts::exec Exec>
void reject_upgrade(server_upgrade_plan<Exec> &plan, error_code error,
	http::status_enum status = http::status::bad_request)
{
	plan.accepted = false;
	plan.error = error;
	plan.status = status;
	if(status == http::status::upgrade_required)
		plan.headers[sec_websocket_version] = "13";
}

template <core_concepts::exec Exec>
void reject_upgrade(server_upgrade_plan<Exec> &plan,
	upgrade_rejection rejection, error_code error)
{
	plan.accepted = false;
	plan.error = error;
	plan.status = valid_rejection_status(rejection.status) ?
		rejection.status : http::status::internal_server_error;
	plan.headers = std::move(rejection.headers);
	erase_protocol_response_headers(plan.headers);
	plan.body = std::move(rejection.body);
}

template <core_concepts::exec Exec>
[[nodiscard]] server_upgrade_plan<Exec> make_server_upgrade_plan(
	http::basic_service_context<Exec> &context, const upgrade_options &options,
	std::chrono::steady_clock::time_point deadline) noexcept
{
	server_upgrade_plan<Exec> plan;
	try
	{
		plan.request = snapshot_request(context.request());
		if(options.stream.read_buffer_size == 0)
		{
			reject_upgrade(plan, make_error_code(std::errc::invalid_argument),
				http::status::internal_server_error);
			return plan;
		}

		auto opening = parse_opening_request(context.request().method(),
			context.request().version(), context.request().headers());
		if(not opening)
		{
			reject_upgrade(plan, opening.error(),
				opening.error() == errc::unsupported_version ?
				http::status::upgrade_required : http::status::bad_request);
			return plan;
		}
		plan.opening = std::move(*opening);

		if(not options.supported_extensions.empty())
		{
			reject_upgrade(plan, make_error_code(errc::unsupported_extension));
			return plan;
		}
		if(server_deadline_expired(deadline))
		{
			reject_upgrade(plan, asio::error::timed_out);
			return plan;
		}

		if(options.request_validator)
		{
			try
			{
				if(auto rejection = options.request_validator(plan.request))
				{
					reject_upgrade(plan, std::move(*rejection),
						make_error_code(errc::handshake_rejected));
					return plan;
				}
			}
			catch(...)
			{
				reject_upgrade(plan, exception_error(std::current_exception()),
					http::status::internal_server_error);
				plan.preserve_error_on_write_failure = true;
				return plan;
			}
		}
		if(server_deadline_expired(deadline))
		{
			reject_upgrade(plan, asio::error::timed_out);
			return plan;
		}

		if(options.origin_validator)
		{
			try
			{
				std::optional<std::string_view> origin;
				if(auto iterator = plan.request.request_headers.find(http::header::origin);
					iterator != plan.request.request_headers.end())
					origin = iterator->second.to_string();
				if(auto rejection = options.origin_validator(origin))
				{
					reject_upgrade(plan, std::move(*rejection),
						make_error_code(errc::handshake_rejected));
					return plan;
				}
			}
			catch(...)
			{
				reject_upgrade(plan, exception_error(std::current_exception()),
					http::status::internal_server_error);
				plan.preserve_error_on_write_failure = true;
				return plan;
			}
		}
		if(server_deadline_expired(deadline))
		{
			reject_upgrade(plan, asio::error::timed_out);
			return plan;
		}

		std::optional<std::string> selected_protocol;
		if(options.subprotocol_selector)
		{
			try
			{
				selected_protocol = options.subprotocol_selector(
					plan.opening.subprotocols);
			}
			catch(...)
			{
				reject_upgrade(plan, exception_error(std::current_exception()),
					http::status::internal_server_error);
				plan.preserve_error_on_write_failure = true;
				return plan;
			}
		}
		else
		{
			for(const auto &offered : plan.opening.subprotocols)
			{
				if(std::ranges::find(options.supported_subprotocols, offered) !=
					options.supported_subprotocols.end())
				{
					selected_protocol = offered;
					break;
				}
			}
		}
		if(server_deadline_expired(deadline))
		{
			reject_upgrade(plan, asio::error::timed_out);
			return plan;
		}
		if(selected_protocol and
			(std::ranges::find(plan.opening.subprotocols, *selected_protocol) ==
				plan.opening.subprotocols.end() or
			 std::ranges::find(options.supported_subprotocols, *selected_protocol) ==
				options.supported_subprotocols.end()))
		{
			reject_upgrade(plan, make_error_code(errc::unsupported_subprotocol));
			return plan;
		}
		if(options.require_subprotocol and not selected_protocol)
		{
			reject_upgrade(plan, make_error_code(errc::unsupported_subprotocol));
			return plan;
		}
		plan.response.subprotocol = std::move(selected_protocol);

		if(options.extension_selector)
		{
			try
			{
				plan.response.extensions = options.extension_selector(
					plan.opening.extensions);
			}
			catch(...)
			{
				reject_upgrade(plan, exception_error(std::current_exception()),
					http::status::internal_server_error);
				plan.preserve_error_on_write_failure = true;
				return plan;
			}
			if(not plan.response.extensions.empty())
			{
				reject_upgrade(plan,
					make_error_code(errc::unsupported_extension));
				return plan;
			}
		}
		if(server_deadline_expired(deadline))
		{
			reject_upgrade(plan, asio::error::timed_out);
			return plan;
		}

		auto protocol_headers = make_opening_response_headers(
			plan.opening, plan.response);
		if(not protocol_headers)
		{
			reject_upgrade(plan, protocol_headers.error());
			return plan;
		}
		plan.headers = options.response_headers;
		erase_protocol_response_headers(plan.headers);
		for(auto &[name, value] : *protocol_headers)
			plan.headers[name] = value;
		plan.status = http::status::switching_protocols;
		plan.accepted = true;
		plan.error.clear();
		return plan;
	}
	catch(...)
	{
		reject_upgrade(plan, exception_error(std::current_exception()),
			http::status::internal_server_error);
		return plan;
	}
}

template <core_concepts::exec Exec>
void prepare_response(http::basic_response<Exec> &response,
	const server_upgrade_plan<Exec> &plan)
{
	response.unset_header(http::header::content_length);
	response.unset_header(http::header::transfer_encoding);
	response.unset_header(http::header::connection);
	response.unset_header(http::header::upgrade);
	response.unset_header(sec_websocket_accept);
	response.unset_header(sec_websocket_version);
	response.unset_header(sec_websocket_protocol);
	response.unset_header(sec_websocket_extensions);
	response.set_status(plan.status);
	for(const auto &[name, value] : plan.headers)
		response.set_header(name, value);
}

template <core_concepts::exec Exec>
void upgrade_sync(http::basic_service_context<Exec> &context,
	upgrade_options options, basic_accept_result<Exec> &result,
	error_code &error) noexcept
{
	try
	{
		if(options.handshake_timeout <= std::chrono::milliseconds::zero())
		{
			close_upgrade_connection(context);
			error = asio::error::timed_out;
			return;
		}
		const auto deadline = std::chrono::steady_clock::now() +
			options.handshake_timeout;
		auto plan = make_server_upgrade_plan(context, options, deadline);
		result.request = plan.request;
		if(plan.error == asio::error::timed_out or server_deadline_expired(deadline))
		{
			close_upgrade_connection(context);
			error = asio::error::timed_out;
			return;
		}

		prepare_response(context.response(), plan);
		ignore_unused(context.response().write(
			asio::buffer(plan.body), error));
		if(error)
		{
			close_upgrade_connection(context);
			if(plan.preserve_error_on_write_failure)
				error = plan.error;
			return;
		}
		if(server_deadline_expired(deadline))
		{
			close_upgrade_connection(context);
			error = asio::error::timed_out;
			return;
		}
		if(not plan.accepted)
		{
			error = plan.error;
			return;
		}

		auto pending = context.request().take_pending_data();
		auto connection = context.hand_over_connection();
		adopt_options adopt {
			.stream_role = role::server,
			.pending_data = std::vector<std::byte>(pending.size()),
			.negotiated_subprotocol = plan.response.subprotocol.value_or("")
		};
		if(not pending.empty())
			std::memcpy(adopt.pending_data.data(), pending.data(), pending.size());
		result.handshake.subprotocol = adopt.negotiated_subprotocol;
		result.stream.adopt(std::move(connection), std::move(adopt), error);
	}
	catch(...)
	{
		error = exception_error(std::current_exception());
		close_upgrade_connection(context);
	}
}

template <core_concepts::exec Exec, typename Handler>
auto async_upgrade(http::basic_service_context<Exec> &context,
	upgrade_options options, Handler &&handler)
{
	using result_t = basic_accept_result<Exec>;
	using token_t = std::remove_cvref_t<Handler>;
	token_t completion_token(std::forward<Handler>(handler));

	return asio::async_initiate<token_t,void(error_code,result_t)>(
		asio::co_composed<void(error_code,result_t)>([](
			auto state, http::basic_service_context<Exec> *active_context,
			upgrade_options active_options) -> void
		{
			result_t result(active_context->get_executor());
			try
			{
				if(active_options.handshake_timeout <=
					std::chrono::milliseconds::zero())
				{
					close_upgrade_connection(*active_context);
					co_return std::tuple<error_code,result_t>{
						asio::error::timed_out, std::move(result)};
				}
				const auto deadline = std::chrono::steady_clock::now() +
					active_options.handshake_timeout;
				auto plan = make_server_upgrade_plan(
					*active_context, active_options, deadline);
				result.request = plan.request;
				if(plan.error == asio::error::timed_out or
					server_deadline_expired(deadline))
				{
					close_upgrade_connection(*active_context);
					co_return std::tuple<error_code,result_t>{
						asio::error::timed_out, std::move(result)};
				}

				prepare_response(active_context->response(), plan);
				auto remaining = server_remaining_timeout(deadline);
				if(remaining <= std::chrono::milliseconds::zero())
				{
					close_upgrade_connection(*active_context);
					co_return std::tuple<error_code,result_t>{
						asio::error::timed_out, std::move(result)};
				}
				auto [write_error, transferred] =
					co_await active_context->response().write(
						asio::buffer(plan.body), asio::as_tuple(deferred));
				ignore_unused(transferred);
				if(write_error)
				{
					close_upgrade_connection(*active_context);
					co_return std::tuple<error_code,result_t>{
						plan.preserve_error_on_write_failure ?
							plan.error : write_error, std::move(result)};
				}
				if(server_deadline_expired(deadline))
				{
					close_upgrade_connection(*active_context);
					co_return std::tuple<error_code,result_t>{
						asio::error::timed_out, std::move(result)};
				}
				if(not plan.accepted)
				{
					co_return std::tuple<error_code,result_t>{
						plan.error, std::move(result)};
				}

				auto pending = active_context->request().take_pending_data();
				auto connection = active_context->hand_over_connection();
				adopt_options adopt {
					.stream_role = role::server,
					.pending_data = std::vector<std::byte>(pending.size()),
					.negotiated_subprotocol =
						plan.response.subprotocol.value_or("")
				};
				if(not pending.empty())
					std::memcpy(adopt.pending_data.data(),
						pending.data(), pending.size());
				result.handshake.subprotocol = adopt.negotiated_subprotocol;
				error_code adopt_error;
				result.stream.adopt(std::move(connection),
					std::move(adopt), adopt_error);
				co_return std::tuple<error_code,result_t>{
					adopt_error, std::move(result)};
			}
			catch(...)
			{
				close_upgrade_connection(*active_context);
				co_return std::tuple<error_code,result_t>{
					exception_error(std::current_exception()), std::move(result)};
			}
		}, context.get_executor()), completion_token, &context,
		std::move(options));
}

} //namespace detail

template <core_concepts::exec Exec>
bool is_upgrade_request(const http::basic_request<Exec> &request) noexcept
{
	if(not request.is_upgrade())
		return false;
	auto protocol = http::upgrade_protocol(request.headers());
	if(not protocol)
		return false;
	std::string_view value(*protocol);
	while(not value.empty() and (value.front() == ' ' or value.front() == '\t'))
		value.remove_prefix(1);
	while(not value.empty() and (value.back() == ' ' or value.back() == '\t'))
		value.remove_suffix(1);
	return detail::server_ascii_equal_case_insensitive(value, "websocket");
}

template <core_concepts::exec Exec, typename Token>
auto upgrade(http::basic_service_context<Exec> &context, Token &&token)
requires concepts::dis_detach_opt_token<
	Token,error_code,basic_accept_result<Exec>>
{
	return upgrade(context, upgrade_options{}, std::forward<Token>(token));
}

template <core_concepts::exec Exec, typename Token>
auto upgrade(http::basic_service_context<Exec> &context,
	upgrade_options options, Token &&token)
requires concepts::dis_detach_opt_token<
	Token,error_code,basic_accept_result<Exec>>
{
	if constexpr(is_error_code_token_v<Token>)
	{
		basic_accept_result<Exec> result(context.get_executor());
		detail::upgrade_sync(context, std::move(options), result, token);
		return result;
	}
	else if constexpr(is_sync_opt_token_v<Token>)
	{
		basic_accept_result<Exec> result(context.get_executor());
		error_code error;
		detail::upgrade_sync(context, std::move(options), result, error);
		if(error)
			system_error::loc_throw(error, "libgs::websocket::upgrade");
		return result;
	}
	else
	{
		const auto timeout = options.handshake_timeout;
		return detail::initiate_handshake_io<basic_accept_result<Exec>>(
			context.get_executor(),
			[&context, options = std::move(options)]
			<typename Handler>(Handler &&handler) mutable
			{
				detail::async_upgrade(context, std::move(options),
					std::forward<Handler>(handler));
			}, timeout, [exec = context.get_executor()] {
				return basic_accept_result<Exec>(exec);
			}, unbound_redirect_time(std::forward<Token>(token)));
	}
}

} //namespace libgs::websocket

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif

#endif //LIBGS_WEBSOCKET_DETAIL_SERVER_H
