// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/http/protocol/cookie.h>
#include <libgs/http/protocol/types.h>
#include <libgs/http/protocol/version.h>
#include <libgs/http/client.h>
#include <libgs/http/server/response.h>
#include <libgs/core/system/app_utls.h>

namespace
{

class scoped_environment
{
	struct entry
	{
		std::string name;
		libgs::optional<std::string> value;
	};

public:
	scoped_environment(std::initializer_list<std::string_view> names)
	{
		for(const auto name : names)
		{
			auto value = libgs::app::getenv(name);
			m_entries.push_back({std::string(name), value ?
				libgs::optional<std::string>(*value) : libgs::nullopt});
		}
	}

	~scoped_environment()
	{
		for(const auto &item : m_entries)
		{
			if( item.value )
				libgs::ignore_unused(libgs::app::setenv(item.name, *item.value));
			else
				libgs::ignore_unused(libgs::app::unsetenv(item.name));
		}
	}

private:
	std::vector<entry> m_entries;
};

class scripted_connection final : public libgs::http::basic_connection<>
{
public:
	struct io_step
	{
		size_t transferred = 0;
		libgs::error_code error {};
		bool wait_for_cancellation = false;
	};
	struct read_step
	{
		std::string payload {};
		libgs::error_code error {};
		size_t offset = 0;
	};

	explicit scripted_connection(executor_t exec) : m_exec(std::move(exec)) {}

	void push_write(size_t transferred, libgs::error_code error = {})
	{
		m_writes.push_back({transferred, error});
	}

	void push_cancellable_write(size_t transferred)
	{
		m_writes.push_back({transferred, {}, true});
	}

	void push_read(std::string payload, libgs::error_code error = {})
	{
		m_reads.push_back({std::move(payload), error});
	}

	void set_probe_state(libgs::sys_expected<probe_state_t> state)
	{
		m_probe_state = std::move(state);
	}

	libgs::sys_expected<> cancel() noexcept override {
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> close() noexcept override {
		m_open = false;
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> set_options(
		const libgs::http::tcp_socket_options&
	) noexcept override {
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<libgs::http::tcp_socket_state>
	options() const noexcept override {
		return libgs::http::tcp_socket_state {};
	}

	bool is_open() const noexcept override {
		return m_open;
	}

	libgs::sys_expected<probe_state_t> probe() noexcept override {
		return m_probe_state;
	}

	libgs::http::endpoint remote_endpoint() const noexcept override {
		return {};
	}

	libgs::http::endpoint local_endpoint() const noexcept override {
		return {};
	}

	executor_t get_executor() noexcept override {
		return m_exec;
	}

	[[nodiscard]] const std::string &written_data() const noexcept {
		return m_written_data;
	}

protected:
	size_t read_some(
		libgs::mutable_buffer buffer, libgs::error_code &error
	) noexcept override
	{
		return read_into(buffer, error);
	}

	size_t write_all(
		const libgs::const_buffer &buffer, libgs::error_code &error
	) noexcept override
	{
		auto step = next_write(buffer.size());
		m_written_data.append(static_cast<const char*>(buffer.data()),
			step.transferred);
		error = step.error;
		return step.transferred;
	}

	void co_read_some(
		libgs::mutable_buffer buffer, io_handler_t completion
	) noexcept override
	{
		libgs::error_code error {};
		const auto size = read_into(buffer, error);
		asio::post(m_exec,
			[handler = std::move(completion), error, size]() mutable {
				std::move(handler)(error, size);
			});
	}

	void co_write_all(
		libgs::const_buffer buffer, io_handler_t completion
	) noexcept override
	{
		auto step = next_write(buffer.size());
		m_written_data.append(static_cast<const char*>(buffer.data()),
			step.transferred);
		if( step.wait_for_cancellation )
		{
			auto slot = asio::get_associated_cancellation_slot(completion);
			if( not slot.is_connected() )
			{
				asio::post(m_exec,
					[handler = std::move(completion)]() mutable {
						std::move(handler)(
							std::make_error_code(std::errc::operation_not_supported), 0
						);
					});
				return ;
			}
			struct pending_write
			{
				std::optional<io_handler_t> handler;
			};
			auto pending = std::make_shared<pending_write>(
				pending_write {std::move(completion)}
			);
			slot.assign(
				[pending, step](asio::cancellation_type type) mutable
				{
					if( type == asio::cancellation_type::none or
						not pending->handler )
						return ;
					auto pending_completion = std::move(*pending->handler);
					pending->handler.reset();
					std::move(pending_completion)(
						asio::error::operation_aborted, step.transferred
					);
				});
			return ;
		}
		asio::post(m_exec,
			[handler = std::move(completion), step]() mutable {
				std::move(handler)(step.error, step.transferred);
			});
	}

private:
	[[nodiscard]] size_t read_into(
		libgs::mutable_buffer buffer, libgs::error_code &error
	) noexcept
	{
		if( m_reads.empty() )
		{
			error = asio::error::eof;
			return 0;
		}

		auto &front = m_reads.front();
		const auto remaining = front.payload.size() - front.offset;
		const auto size = std::min(remaining, buffer.size());
		if( size > 0 )
		{
			std::memcpy(buffer.data(), front.payload.data() + front.offset, size);
			front.offset += size;
		}
		if( front.offset == front.payload.size() )
		{
			error = front.error;
			m_reads.pop_front();
		}
		return size;
	}

	[[nodiscard]] io_step next_write(size_t requested) noexcept
	{
		if( m_writes.empty() )
			return {requested, {}};
		auto step = m_writes.front();
		m_writes.pop_front();
		step.transferred = std::min(step.transferred, requested);
		return step;
	}

	executor_t m_exec;
	std::deque<read_step> m_reads {};
	std::deque<io_step> m_writes {};
	libgs::sys_expected<probe_state_t> m_probe_state {probe_state_t::no_event};
	std::string m_written_data {};
	bool m_open = true;
};

class scripted_connector final : public libgs::http::connector
{
public:
	explicit scripted_connector(executor_t exec) :
		libgs::http::connector(std::move(exec)) {}

	[[nodiscard]] size_t connection_count(std::string_view host) const
	{
		auto pos = m_connection_counts.find(std::string(host));
		return pos == m_connection_counts.end() ? 0 : pos->second;
	}

	void push_response(std::string response)
	{
		m_responses.emplace_back(std::move(response));
	}

	[[nodiscard]] std::shared_ptr<scripted_connection> last_connection() const
	{
		return m_connections.empty() ? nullptr : m_connections.back();
	}

	[[nodiscard]] const libgs::http::connect_target &last_target() const
	{
		return m_targets.back();
	}

protected:
	libgs::sys_expected<connection_ptr> do_connect(
		const libgs::http::connect_target &target
	) noexcept override
	{
		libgs::error_code error {};
		try {
			m_targets.emplace_back(target);
			++m_connection_counts[target.host];
			auto connection = std::make_shared<scripted_connection>(get_executor());
			if( not m_responses.empty() )
			{
				connection->push_read(std::move(m_responses.front()));
				m_responses.pop_front();
			}
			m_connections.emplace_back(connection);
			return connection_ptr(std::move(connection));
		}
		catch(const std::bad_alloc&) {
			error = std::make_error_code(std::errc::not_enough_memory);
		}
		catch(...) {
			error = std::make_error_code(std::errc::io_error);
		}
		return libgs::sys_unexpected(error);
	}

	libgs::awaitable<libgs::sys_expected<connection_ptr>> co_do_connect(
		const libgs::http::connect_target &target
	) noexcept override
	{
		co_return do_connect(target);
	}

private:
	std::unordered_map<std::string,size_t> m_connection_counts {};
	std::deque<std::string> m_responses {};
	std::vector<std::shared_ptr<scripted_connection>> m_connections {};
	std::vector<libgs::http::connect_target> m_targets {};
};

void protocol_enums()
{
	using namespace libgs::http;

	LIBGS_TEST_CHECK_EQ(method::from_string("GET"), method::get);
	LIBGS_TEST_CHECK_EQ(std::string(method::string(method::delet)), "DELETE");
	LIBGS_TEST_CHECK_EQ(
		std::string(status::description(status::not_found)), "Not Found"
	);
	LIBGS_TEST_CHECK_EQ(version::from_string("1.1"), version::v11);
	LIBGS_TEST_CHECK_EQ(std::string(version::string(version::v10)), "1.0");
	LIBGS_TEST_CHECK_EQ(version(version::v11).number(), 1.1);

	constexpr methods safe_methods {method::get, method::head};
	static_assert(safe_methods.test_flag(method::get));
	static_assert(safe_methods.test_flag(method::head));
	static_assert(not safe_methods.test_flag(method::post));
}

void cookie_values()
{
	libgs::http::cookie item("session-token");
	item.set_domain("example.test")
		.set_path("/account")
		.set_same_site("Strict")
		.set_priority("High")
		.set_expires(3600)
		.set_max_age(1800)
		.set_size(13)
		.set_http_only(true)
		.set_secure(true);

	LIBGS_TEST_CHECK_EQ(item.value<std::string>(), "session-token");
	LIBGS_TEST_CHECK_EQ(*item.domain(), "example.test");
	LIBGS_TEST_CHECK_EQ(*item.path(), "/account");
	LIBGS_TEST_CHECK_EQ(*item.same_site(), "Strict");
	LIBGS_TEST_CHECK_EQ(*item.priority(), "High");
	LIBGS_TEST_CHECK_EQ(*item.expires(), std::uint64_t {3600});
	LIBGS_TEST_CHECK_EQ(*item.max_age(), std::uint64_t {1800});
	LIBGS_TEST_CHECK_EQ(*item.size(), size_t {13});
	LIBGS_TEST_CHECK(*item.http_only());
	LIBGS_TEST_CHECK(*item.secure());

	item.unset_domain().unset_http_only().unset_secure();
	LIBGS_TEST_CHECK(not item.domain());
	LIBGS_TEST_CHECK(not item.http_only());
	LIBGS_TEST_CHECK(not item.secure());

	libgs::http::cookie copied(item);
	copied.set_path("/other");
	LIBGS_TEST_CHECK_EQ(*item.path(), "/account");
	LIBGS_TEST_CHECK_EQ(*copied.path(), "/other");
}

void client_url_validation()
{
	libgs::io_context_t context;
	libgs::http::client client(context.get_executor());
	std::error_code error;

	auto fragmented = client.request_get(
		libgs::url("http://example.test/path#fragment"), error
	);
	LIBGS_TEST_CHECK(not fragmented);
	LIBGS_TEST_CHECK(error == std::errc::invalid_argument);

	error.clear();
	auto wrong_scheme = client.request_get(
		libgs::url("ws://example.test/path"), error
	);
	LIBGS_TEST_CHECK(not wrong_scheme);
	LIBGS_TEST_CHECK(error == std::errc::protocol_not_supported);

	error.clear();
	auto missing_host = client.request_get(
		libgs::url("http:///path"), error
	);
	LIBGS_TEST_CHECK(not missing_host);
	LIBGS_TEST_CHECK(error == std::errc::invalid_argument);
}

void client_proxy_inheritance_and_environment()
{
	using namespace libgs::http;
	const scoped_environment environment({
		"http_proxy", "HTTP_PROXY", "https_proxy", "HTTPS_PROXY",
		"ws_proxy", "WS_PROXY", "wss_proxy", "WSS_PROXY",
		"all_proxy", "ALL_PROXY", "no_proxy", "NO_PROXY",
	});
	LIBGS_TEST_CHECK(libgs::app::setenv("http_proxy",
		"http://user:secret@proxy.test:8080/"));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("HTTP_PROXY"));
#endif
	LIBGS_TEST_CHECK(libgs::app::setenv("https_proxy",
		"http://secure-proxy.test:8443/"));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("HTTPS_PROXY"));
#endif
	LIBGS_TEST_CHECK(libgs::app::unsetenv("all_proxy"));
	LIBGS_TEST_CHECK(libgs::app::unsetenv("ALL_PROXY"));
	LIBGS_TEST_CHECK(libgs::app::setenv("ws_proxy",
		"http://websocket-only.test:8082/"));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("WS_PROXY"));
#endif
	LIBGS_TEST_CHECK(libgs::app::setenv("wss_proxy",
		"http://secure-websocket-only.test:8444/"));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("WSS_PROXY"));
#endif
	LIBGS_TEST_CHECK(libgs::app::setenv("no_proxy",
		".bypass.test,127.0.0.0/8"));
#if !defined(_WIN32)
	LIBGS_TEST_CHECK(libgs::app::unsetenv("NO_PROXY"));
#endif

	libgs::io_context_t context;
	auto connector = std::make_shared<scripted_connector>(context.get_executor());
	connection_pool pool(connector);
	client requester(std::move(pool));
	libgs::error_code error;

	{
		auto request = requester.request_get(
			"http://origin.test/resource?q=1", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "proxy.test");
		LIBGS_TEST_CHECK_EQ(connector->last_target().port, 8080);
		LIBGS_TEST_CHECK(not connector->last_target().tunnel);
		const auto &wire = connector->last_connection()->written_data();
		LIBGS_TEST_CHECK(wire.starts_with(
			"GET http://origin.test/resource?q=1 HTTP/1.1\r\n"));
		LIBGS_TEST_CHECK(wire.find(
			"Proxy-Authorization: Basic dXNlcjpzZWNyZXQ=\r\n") !=
			std::string::npos);
	}

	{
		auto request = requester.make_get("https://secure.test/resource", error);
		LIBGS_TEST_CHECK(request and not error);
		const auto &target = connector->last_target();
		LIBGS_TEST_CHECK_EQ(target.host, "secure.test");
		LIBGS_TEST_CHECK_EQ(target.port, 443);
		LIBGS_TEST_CHECK(target.tunnel);
		LIBGS_TEST_CHECK_EQ(target.tunnel->type,
			proxy_tunnel_type::http_connect);
		LIBGS_TEST_CHECK_EQ(target.tunnel->host, "secure-proxy.test");
		LIBGS_TEST_CHECK_EQ(target.tunnel->port, 8443);
	}

	{
		auto request = requester.make_get("http://api.bypass.test/value", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "api.bypass.test");
		LIBGS_TEST_CHECK(not connector->last_target().tunnel);
	}

	{
		auto request = requester.make_get("http://127.12.34.56/value", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "127.12.34.56");
	}

	{
		client::req_info request_info("http://direct.test/value");
		request_info.proxy = no_proxy;
		auto request = requester.make_get(std::move(request_info), error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "direct.test");
	}

	{
		client::req_info request_info("http://override-origin.test/value");
		request_info.proxy = libgs::url("http://override-proxy.test:9000/");
		auto request = requester.make_get(std::move(request_info), error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host,
			"override-proxy.test");
		LIBGS_TEST_CHECK_EQ(connector->last_target().port, 9000);
	}

	LIBGS_TEST_CHECK(libgs::app::unsetenv("http_proxy"));
	LIBGS_TEST_CHECK(libgs::app::setenv("HTTP_PROXY",
		"http://uppercase-proxy.test:8081/"));
	{
		auto request = requester.make_get("http://uppercase-origin.test/", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host,
			"uppercase-proxy.test");
		LIBGS_TEST_CHECK_EQ(connector->last_target().port, 8081);
	}

	LIBGS_TEST_CHECK(libgs::app::unsetenv("HTTP_PROXY"));
	{
		auto request = requester.make_get("http://http-only.test/", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "http-only.test");
		LIBGS_TEST_CHECK(not connector->last_target().tunnel);
	}

	LIBGS_TEST_CHECK(libgs::app::unsetenv("https_proxy"));
	LIBGS_TEST_CHECK(libgs::app::unsetenv("HTTPS_PROXY"));
	{
		auto request = requester.make_get("https://https-only.test/", error);
		LIBGS_TEST_CHECK(request and not error);
		LIBGS_TEST_CHECK_EQ(connector->last_target().host, "https-only.test");
		LIBGS_TEST_CHECK(not connector->last_target().tunnel);
	}

	LIBGS_TEST_CHECK(libgs::app::setenv("all_proxy",
		"socks5h://socks-proxy.test:1081/"));
	{
		auto request = requester.make_get("http://fallback-origin.test/", error);
		LIBGS_TEST_CHECK(request and not error);
		const auto &target = connector->last_target();
		LIBGS_TEST_CHECK_EQ(target.host, "fallback-origin.test");
		LIBGS_TEST_CHECK(target.tunnel);
		LIBGS_TEST_CHECK_EQ(target.tunnel->type, proxy_tunnel_type::socks5);
		LIBGS_TEST_CHECK_EQ(target.tunnel->host, "socks-proxy.test");
		LIBGS_TEST_CHECK_EQ(target.tunnel->port, 1081);
	}
}

void connection_partial_write_counts()
{
	libgs::io_context_t context;
	auto connection = std::make_shared<scripted_connection>(
		context.get_executor()
	);
	const std::string payload = "abcdef";
	const libgs::const_buffer body(payload.data(), payload.size());
	const auto failure = std::make_error_code(std::errc::broken_pipe);

	connection->push_write(3, failure);
	libgs::error_code error {};
	auto size = connection->write(body, error);
	LIBGS_TEST_CHECK_EQ(size, size_t {3});
	LIBGS_TEST_CHECK_EQ(error, failure);

	error = failure;
	size = connection->write(body, error);
	LIBGS_TEST_CHECK_EQ(size, payload.size());
	LIBGS_TEST_CHECK(not error);

	connection->push_write(2, failure);
	auto expected = connection->write(body);
	LIBGS_TEST_CHECK(not expected.has_value());
	LIBGS_TEST_CHECK_EQ(expected.error(), failure);

	const std::string first = "ab";
	const std::string second = "cde";
	const std::array<libgs::const_buffer,2> buffers {{
		{first.data(), first.size()},
		{second.data(), second.size()},
	}};
	connection->push_write(first.size());
	connection->push_write(1, failure);
	error.clear();
	size = connection->write(std::span<const libgs::const_buffer>(buffers), error);
	LIBGS_TEST_CHECK_EQ(size, first.size() + size_t {1});
	LIBGS_TEST_CHECK_EQ(error, failure);

	connection->push_write(4, failure);
	bool async_completed = false;
	connection->write(body,
		[&](libgs::error_code async_error, size_t transferred)
		{
			LIBGS_TEST_CHECK_EQ(transferred, size_t {4});
			LIBGS_TEST_CHECK_EQ(async_error, failure);
			async_completed = true;
		});
	context.run();
	LIBGS_TEST_CHECK(async_completed);

	context.restart();
	connection->push_write(first.size());
	connection->push_write(2, failure);
	bool async_sequence_completed = false;
	connection->write(std::span<const libgs::const_buffer>(buffers),
		[&](libgs::error_code async_error, size_t transferred)
		{
			LIBGS_TEST_CHECK_EQ(transferred, first.size() + size_t {2});
			LIBGS_TEST_CHECK_EQ(async_error, failure);
			async_sequence_completed = true;
		});
	context.run();
	LIBGS_TEST_CHECK(async_sequence_completed);

	context.restart();
	connection->push_write(5, failure);
	bool timed_completed = false;
	using namespace libgs::operators;
	connection->write(body,
		([&](libgs::error_code async_error, size_t transferred)
		{
			LIBGS_TEST_CHECK_EQ(transferred, size_t {5});
			LIBGS_TEST_CHECK_EQ(async_error, failure);
			timed_completed = true;
		}) | std::chrono::seconds(1));
	context.run();
	LIBGS_TEST_CHECK(timed_completed);

	context.restart();
	connection->push_cancellable_write(2);
	bool timeout_completed = false;
	connection->write(body,
		([&](libgs::error_code async_error, size_t transferred)
		{
			LIBGS_TEST_CHECK_EQ(transferred, size_t {2});
			LIBGS_TEST_CHECK_EQ(async_error,
				asio::error::make_error_code(asio::error::timed_out));
			timeout_completed = true;
		}) | std::chrono::milliseconds(1));
	context.run();
	LIBGS_TEST_CHECK(timeout_completed);
}

void http_body_write_counts()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	const std::string payload = "abcdef";
	const auto failure = std::make_error_code(std::errc::broken_pipe);

	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(3, failure);
		auto lease = std::make_shared<connection_lease>(
			connection, [](connection_lease::connection_ptr) {}
		);
		request_context<method::post> request(
			std::move(lease), libgs::url("http://example.test/upload")
		);

		libgs::error_code error {};
		auto bytes = request.write(libgs::buffer(payload), error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {3});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(5, failure); // "6\r\n" plus two body bytes.
		auto lease = std::make_shared<connection_lease>(
			connection, [](connection_lease::connection_ptr) {}
		);
		request_context<method::post> request(
			std::move(lease), libgs::url("http://example.test/upload")
		);
		request.set_header(header::transfer_encoding, "chunked");

		libgs::error_code error {};
		auto bytes = request.write(libgs::buffer(payload), error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {2});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(4, failure);
		response reply(connection);

		libgs::error_code error {};
		auto bytes = reply.write(libgs::buffer(payload), error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {4});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(6, failure); // "6\r\n" plus three body bytes.
		response reply(connection);
		reply.set_header(header::transfer_encoding, "chunked");

		libgs::error_code error {};
		auto bytes = reply.write(libgs::buffer(payload), error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {3});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(5, failure);
		auto reply = std::make_shared<response>(connection);
		bool completed = false;
		asio::co_spawn(context,
			[&, reply]() -> libgs::awaitable<void>
			{
				auto [error, bytes] = co_await reply->write (
					libgs::buffer(payload), asio::as_tuple(libgs::use_awaitable)
				);
				LIBGS_TEST_CHECK_EQ(bytes, size_t {5});
				LIBGS_TEST_CHECK_EQ(error, failure);
				completed = true;
				co_return;
			}, asio::detached);
		context.run();
		LIBGS_TEST_CHECK(completed);
	}
	{
		context.restart();
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(4, failure);
		auto reply = std::make_shared<response>(connection);
		bool completed = false;
		using namespace libgs::operators;
		asio::co_spawn(context,
			[&, reply]() -> libgs::awaitable<void>
			{
				auto [error, bytes] = co_await reply->write (
					libgs::buffer(payload),
					asio::as_tuple(libgs::use_awaitable) |
						std::chrono::seconds(1)
				);
				LIBGS_TEST_CHECK_EQ(bytes, size_t {4});
				LIBGS_TEST_CHECK_EQ(error, failure);
				completed = true;
				co_return;
			}, asio::detached);
		context.run();
		LIBGS_TEST_CHECK(completed);
	}
	{
		context.restart();
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_cancellable_write(2);
		auto reply = std::make_shared<response>(connection);
		bool completed = false;
		using namespace libgs::operators;
		asio::co_spawn(context,
			[&, reply]() -> libgs::awaitable<void>
			{
				auto [error, bytes] = co_await reply->write (
					libgs::buffer(payload),
					asio::as_tuple(libgs::use_awaitable) |
						std::chrono::milliseconds(1)
				);
				LIBGS_TEST_CHECK_EQ(bytes, size_t {2});
				LIBGS_TEST_CHECK_EQ(error,
					asio::error::make_error_code(asio::error::timed_out));
				completed = true;
				co_return;
			}, asio::detached);
		context.run();
		LIBGS_TEST_CHECK(completed);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		auto lease = std::make_shared<connection_lease>(
			connection, [](connection_lease::connection_ptr) {}
		);
		request_context<method::post> request(
			std::move(lease), libgs::url("http://example.test/chunked")
		);
		request.set_header(header::transfer_encoding, "chunked");
		libgs::error_code error {};
		LIBGS_TEST_CHECK_EQ(request.write(libgs::buffer(payload), error), payload.size());
		LIBGS_TEST_CHECK(not error);
		LIBGS_TEST_CHECK_EQ(request.chunk_end(error), size_t {0});
		LIBGS_TEST_CHECK(not error);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		response reply(connection);
		reply.set_header(header::transfer_encoding, "chunked");
		libgs::error_code error {};
		LIBGS_TEST_CHECK_EQ(reply.write(libgs::buffer(payload), error), payload.size());
		LIBGS_TEST_CHECK(not error);
		LIBGS_TEST_CHECK_EQ(reply.chunk_end(error), size_t {0});
		LIBGS_TEST_CHECK(not error);
	}
}

void http_file_body_write_counts()
{
	using namespace libgs::http;
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "payload.txt";
	const std::string payload = "file-body";
	{
		std::ofstream stream(file, std::ios::binary);
		stream.write(payload.data(), static_cast<std::streamsize>(payload.size()));
	}

	libgs::io_context_t context;
	const auto failure = std::make_error_code(std::errc::broken_pipe);
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		response reply(connection);
		libgs::error_code error {};
		auto bytes = reply.send_file(file, error);
		LIBGS_TEST_CHECK_EQ(bytes, payload.size());
		LIBGS_TEST_CHECK(not error);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(4, failure);
		response reply(connection);
		libgs::error_code error {};
		auto bytes = reply.send_file(file, error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {4});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		auto lease = std::make_shared<connection_lease>(
			connection, [](connection_lease::connection_ptr) {}
		);
		request_context<method::put> request(
			std::move(lease), libgs::url("http://example.test/upload")
		);
		libgs::error_code error {};
		auto bytes = request.upload_file(basic_body_norms{}, file, error);
		LIBGS_TEST_CHECK_EQ(bytes, payload.size());
		LIBGS_TEST_CHECK(not error);
	}
	{
		auto connection = std::make_shared<scripted_connection>(
			context.get_executor()
		);
		connection->push_write(std::numeric_limits<size_t>::max());
		connection->push_write(3, failure);
		auto lease = std::make_shared<connection_lease>(
			connection, [](connection_lease::connection_ptr) {}
		);
		request_context<method::put> request(
			std::move(lease), libgs::url("http://example.test/upload")
		);
		libgs::error_code error {};
		auto bytes = request.upload_file(basic_body_norms{}, file, error);
		LIBGS_TEST_CHECK_EQ(bytes, size_t {3});
		LIBGS_TEST_CHECK_EQ(error, failure);
	}
}

void connection_pool_indexed_lru()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	auto connector = std::make_shared<scripted_connector>(context.get_executor());
	connection_pool_config config;
	config.max_count = 2;
	config.timeout.idle = std::chrono::seconds(60);
	connection_pool pool(connector, config);

	const connect_target first {"first.test", 80, security_mode::plain};
	const connect_target second {"second.test", 80, security_mode::plain};
	const connect_target third {"third.test", 80, security_mode::plain};

	auto acquire_and_release = [&](const connect_target &target)
	{
		auto lease = pool.get(target);
		LIBGS_TEST_CHECK(lease);
		LIBGS_TEST_CHECK(*lease);
		(*lease)->release();
	};

	acquire_and_release(first);
	acquire_and_release(second);
	LIBGS_TEST_CHECK_EQ(pool.count(), 2U);

	// At capacity, a new target evicts the globally oldest idle connection.
	acquire_and_release(third);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("first.test"), 1U);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("second.test"), 1U);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("third.test"), 1U);
	LIBGS_TEST_CHECK_EQ(pool.count(), 2U);

	// first.test was evicted, while third.test remains reusable by exact key.
	acquire_and_release(first);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("first.test"), 2U);
	acquire_and_release(third);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("third.test"), 1U);
	LIBGS_TEST_CHECK_EQ(pool.count(), 2U);
}

void connection_pool_indexed_waiters()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	auto connector = std::make_shared<scripted_connector>(context.get_executor());
	connection_pool_config config;
	config.max_count = 1;
	connection_pool pool(connector, config);
	const connect_target first {"first.test", 80, security_mode::plain};
	const connect_target second {"second.test", 80, security_mode::plain};

	auto held_result = pool.get(first);
	LIBGS_TEST_CHECK(held_result);
	auto held = *held_result;
	std::vector<std::string> acquisition_order;

	auto second_waiter = asio::co_spawn(context,
	[&]() -> libgs::awaitable<void>
	{
		auto lease = co_await pool.get(second, libgs::use_awaitable);
		acquisition_order.emplace_back("second");
		lease->release();
	}, asio::use_future);
	auto matching_waiter = asio::co_spawn(context,
	[&]() -> libgs::awaitable<void>
	{
		auto lease = co_await pool.get(first, libgs::use_awaitable);
		acquisition_order.emplace_back("first");
		lease->release();
	}, asio::use_future);

	// Both waiters are enqueued before the held connection is returned.
	asio::post(context, [held] { held->release(); });
	context.run();
	second_waiter.get();
	matching_waiter.get();
	LIBGS_TEST_CHECK_EQ(acquisition_order.size(), 2U);
	LIBGS_TEST_CHECK_EQ(acquisition_order[0], "first");
	LIBGS_TEST_CHECK_EQ(acquisition_order[1], "second");
}

void connection_pool_discards_failed_connections()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	auto connector = std::make_shared<scripted_connector>(context.get_executor());
	connection_pool pool(connector);
	const connect_target target {"failed.test", 80, security_mode::plain};

	auto first_result = pool.get(target);
	LIBGS_TEST_CHECK(first_result);
	auto first = *first_result;
	auto failed_connection = connector->last_connection();
	LIBGS_TEST_CHECK(failed_connection);
	failed_connection->set_probe_state(connection_probe_state::peer_closed);
	first->release();

	LIBGS_TEST_CHECK(not failed_connection->is_open());
	LIBGS_TEST_CHECK_EQ(pool.count(), 0U);

	auto second_result = pool.get(target);
	LIBGS_TEST_CHECK(second_result);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("failed.test"), 2U);
	(*second_result)->release();
}

void http_client_does_not_reuse_malformed_reply()
{
	using namespace libgs::http;
	libgs::io_context_t context;
	auto connector = std::make_shared<scripted_connector>(context.get_executor());
	connector->push_response(
		"HTTP/1.1 200 OK\r\nMissing-Colon\r\n\r\n");
	connector->push_response(
		"HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
	connection_pool pool(connector);
	client requester(std::move(pool), {.default_proxy = no_proxy});

	libgs::error_code error {};
	auto malformed = requester.request_get(
		"http://malformed.test/first", error);
	LIBGS_TEST_CHECK(malformed);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(malformed->wait_reply(error), status::none);
	LIBGS_TEST_CHECK(error);
	LIBGS_TEST_CHECK(not malformed->reply()->lease().is_valid());

	error.clear();
	auto valid = requester.request_get("http://malformed.test/second", error);
	LIBGS_TEST_CHECK(valid);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(valid->wait_reply(error), status::ok);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(connector->connection_count("malformed.test"), 2U);
}

} //namespace

int main(int argc, const char *const argv[])
{
	return libgs::test::run(argc, argv, {
		{"protocol enums", protocol_enums},
		{"cookie values", cookie_values},
		{"client URL validation", client_url_validation},
		{"client proxy inheritance and environment",
			client_proxy_inheritance_and_environment},
		{"connection partial write counts", connection_partial_write_counts},
		{"HTTP body write counts", http_body_write_counts},
		{"HTTP file body write counts", http_file_body_write_counts},
		{"connection pool indexed LRU", connection_pool_indexed_lru},
		{"connection pool indexed waiters", connection_pool_indexed_waiters},
		{"connection pool discards failed connections",
			connection_pool_discards_failed_connections},
		{"HTTP client does not reuse malformed replies",
			http_client_does_not_reuse_malformed_reply},
	});
}
