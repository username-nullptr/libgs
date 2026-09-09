// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/http/protocol/cookie.h>
#include <libgs/http/protocol/types.h>
#include <libgs/http/protocol/version.h>
#include <libgs/http/client.h>
#include <libgs/http/server/response.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <string>

namespace
{

class scripted_connection final : public libgs::http::basic_connection<>
{
public:
	struct io_step
	{
		size_t transferred = 0;
		libgs::error_code error {};
		bool wait_for_cancellation = false;
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

	libgs::sys_expected<> cancel() noexcept override {
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> close() noexcept override {
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
		return true;
	}

	libgs::sys_expected<probe_state_t> probe() noexcept override {
		return probe_state_t::no_event;
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

protected:
	size_t read_some(
		libgs::mutable_buffer, libgs::error_code &error
	) noexcept override
	{
		error = asio::error::eof;
		return 0;
	}

	size_t write_all(
		const libgs::const_buffer &buffer, libgs::error_code &error
	) noexcept override
	{
		auto step = next_write(buffer.size());
		error = step.error;
		return step.transferred;
	}

	void co_read_some(
		libgs::mutable_buffer, io_handler_t completion
	) noexcept override
	{
		asio::post(m_exec,
			[handler = std::move(completion)]() mutable {
				std::move(handler)(asio::error::eof, 0);
			});
	}

	void co_write_all(
		libgs::const_buffer buffer, io_handler_t completion
	) noexcept override
	{
		auto step = next_write(buffer.size());
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
	std::deque<io_step> m_writes {};
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

} //namespace

int main()
{
	return libgs::test::run({
		{"protocol enums", protocol_enums},
		{"cookie values", cookie_values},
		{"client URL validation", client_url_validation},
		{"connection partial write counts", connection_partial_write_counts},
		{"HTTP body write counts", http_body_write_counts},
		{"HTTP file body write counts", http_file_body_write_counts},
	});
}
