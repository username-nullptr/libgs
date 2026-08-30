#include <libgs/http/client/reply.h>
#include <libgs/http/server/request.h>
#include <libgs/http/server/response.h>

#include <cstring>
#include <future>
#include <iostream>

namespace http = libgs::http;

class scripted_connection final : public http::connection
{
public:
	explicit scripted_connection(asio::any_io_executor exec, std::string input = {}) :
		m_exec(std::move(exec)), m_input(std::move(input)) {}

	libgs::sys_expected<> cancel() noexcept override
	{
		for( auto &item : m_timers )
		{
			if( auto timer = item.lock() )
			{
				timer->cancel();
			}
		}
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> close() noexcept override
	{
		m_open = false;
		return cancel();
	}

	libgs::sys_expected<> set_options(const http::tcp_socket_options&) noexcept override
	{
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<http::tcp_socket_state> options() const noexcept override
	{
		http::tcp_socket_state result {};
		result.receive_buffer_size = 4096;
		return result;
	}

	[[nodiscard]] bool is_open() const noexcept override {
		return m_open;
	}

	libgs::sys_expected<probe_state_t> probe() noexcept override {
		return probe_state_t::no_event;
	}

	[[nodiscard]] http::endpoint remote_endpoint() const noexcept override {
		return {};
	}

	[[nodiscard]] http::endpoint local_endpoint() const noexcept override {
		return {};
	}

	[[nodiscard]] executor_t get_executor() noexcept override {
		return m_exec;
	}

	void stall(bool value = true) noexcept {
		m_stall = value;
	}

	void fail_reads(libgs::error_code error) noexcept {
		m_read_error = error;
	}

	void fail_writes(libgs::error_code error) noexcept {
		m_write_error = error;
	}

	[[nodiscard]] const std::string &output() const noexcept {
		return m_output;
	}

protected:
	using io_handler_t = http::connection::io_handler_t;

	libgs::io_expected read_some(libgs::mutable_buffer output) noexcept override
	{
		if( m_read_error )
			return libgs::io_unexpected(m_read_error);
		if( not m_open )
			return libgs::io_unexpected(std::make_error_code(std::errc::not_connected));
		if( m_position == m_input.size() )
			return libgs::io_unexpected(make_error_code(libgs::errc::eof));

		auto size = std::min(output.size(), m_input.size() - m_position);
		if( size > 0 )
			std::memcpy(output.data(), m_input.data() + m_position, size);
		m_position += size;
		return size;
	}

	libgs::io_expected write_all(const libgs::const_buffer &input) noexcept override
	{
		if( m_write_error )
			return libgs::io_unexpected(m_write_error);
		if( not m_open )
			return libgs::io_unexpected(std::make_error_code(std::errc::not_connected));
		if( input.size() > 0 )
		{
			m_output.append(static_cast<const char*>(input.data()), input.size());
		}
		return input.size();
	}

	libgs::io_expected write_all
	(std::span<const libgs::const_buffer> inputs) noexcept override
	{
		size_t sum = 0;
		for( const auto &input : inputs )
		{
			auto result = write_all(input);
			if( not result )
				return result;
			sum += *result;
		}
		return sum;
	}

	void co_read_some(libgs::mutable_buffer output, io_handler_t handler) noexcept override
	{
		defer(std::move(handler), [this, output] {
			return read_some(output);
		});
	}

	void co_write_all(libgs::const_buffer input, io_handler_t handler) noexcept override
	{
		defer(std::move(handler), [this, input] {
			return write_all(input);
		});
	}

	void co_write_all
	(std::span<const libgs::const_buffer> inputs, io_handler_t handler) noexcept override
	{
		http::detail::const_buffer_sequence sequence(inputs);
		defer(std::move(handler),
			[this, sequence = std::move(sequence)]() mutable {
				return write_all(sequence.buffers());
			});
	}

private:
	template <typename Operation>
	void defer(io_handler_t handler, Operation operation) noexcept
	{
		auto timer = std::make_shared<asio::steady_timer>(m_exec);
		timer->expires_after(m_stall ? std::chrono::hours(24) :
			std::chrono::steady_clock::duration::zero());
		m_timers.emplace_back(timer);

		auto slot = asio::get_associated_cancellation_slot(handler);
		if( slot.is_connected() )
		{
			slot.assign([weak = std::weak_ptr(timer)](asio::cancellation_type) {
				if( auto timer = weak.lock() )
				{
					timer->cancel();
				}
			});
		}
		timer->async_wait(
		[timer, handler = std::move(handler), operation = std::move(operation)]
		(libgs::error_code timer_error) mutable
		{
			if( timer_error )
			{
				std::move(handler)(timer_error, 0);
				return ;
			}
			auto result = operation();
			if( result )
				std::move(handler)(libgs::error_code{}, *result);
			else
				std::move(handler)(result.error(), 0);
		});
	}

private:
	executor_t m_exec;
	std::string m_input;
	std::string m_output;
	size_t m_position = 0;
	bool m_open = true;
	bool m_stall = false;
	libgs::error_code m_read_error {};
	libgs::error_code m_write_error {};
	std::vector<std::weak_ptr<asio::steady_timer>> m_timers;
};

struct checks
{
	void operator()(bool condition, std::string_view name)
	{
		++count;
		if( condition )
			std::cout << "PASS " << name << '\n';
		else
		{
			++failed;
			std::cout << "FAIL " << name << '\n';
		}
	}

	int count = 0;
	int failed = 0;
};

using size_callback = void(*)(libgs::error_code,size_t);
using void_callback = void(*)(libgs::error_code);
using timed_future = libgs::redirect_time_t<libgs::use_future_t>;

static_assert(http::response::write_task_token_v<timed_future,size_t>);
static_assert(http::response::write_task_token_v<libgs::detached_t,size_t>);
static_assert(http::request::task_token_v<timed_future>);
static_assert(http::basic_reply<>::task_token_v<size_callback,size_t>);
static_assert(http::basic_reply<>::task_token_v<libgs::use_future_t,size_t>);

static std::shared_ptr<http::basic_reply<>> make_reply
(const std::shared_ptr<scripted_connection> &connection)
{
	auto lease = std::make_shared<http::connection_lease>(connection,
		[](http::connection_ptr) {});
	return std::make_shared<http::basic_reply<>>(std::move(lease));
}

static void test_sync(checks &check)
{
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor(),
			"HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nsync");
		auto reply = make_reply(connection);
		check(reply->read<std::string>() == "sync", "sync value return");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->fail_reads(std::make_error_code(std::errc::connection_reset));
		auto reply = make_reply(connection);
		bool threw = false;
		try {
			libgs::ignore_unused(reply->wait());
		}
		catch(const std::system_error &error) {
			threw = error.code() == std::errc::connection_reset;
		}
		check(threw, "sync default token throws");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->fail_writes(std::make_error_code(std::errc::broken_pipe));
		http::response response(connection);
		libgs::error_code error {};
		auto size = response.write(libgs::buffer("body"), error);
		check(size == 0 and error == std::errc::broken_pipe,
			"sync error_code token receives error");
	}
}

static void test_callback_and_lifetime(checks &check)
{
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor(),
			"HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n");
		bool called = false;
		{
			auto reply = make_reply(connection);
			reply->wait([&](libgs::error_code error, http::status_enum status) {
				called = not error and status == http::status::no_content;
			});
		}
		io.run();
		check(called, "callback completion and reply state lifetime");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		std::string body = "detached-owned-body";
		{
			http::response response(connection);
			response.write(libgs::buffer(body), libgs::detached);
		}
		body.assign(body.size(), 'x');
		io.run();
		check(connection->output().find("detached-owned-body") != std::string::npos,
			"detached response owns state and body");
	}
	{
		asio::io_context operation_io;
		asio::io_context completion_io;
		auto connection = std::make_shared<scripted_connection>(
			operation_io.get_executor(),
			"HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n"
		);
		bool called = false;
		auto reply = make_reply(connection);
		reply->wait(asio::bind_executor(completion_io.get_executor(),
			[&](libgs::error_code error, http::status_enum) { called = not error; }
		));
		operation_io.run();
		check(not called, "associated executor defers callback");
		completion_io.run();
		check(called, "callback runs on associated executor");
	}
}

static void test_future_and_coroutine(checks &check)
{
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor(),
			"HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nfuture");
		auto reply = make_reply(connection);
		auto future = reply->read<std::string>(libgs::use_future);
		io.run();
		check(future.get() == "future", "future returns value");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->fail_writes(std::make_error_code(std::errc::broken_pipe));
		http::response response(connection);
		using namespace std::chrono_literals;
		auto future = response.write(libgs::buffer("future-error"),
			libgs::redirect_time(libgs::use_future, 1s));
		io.run();
		bool threw = false;
		try {
			libgs::ignore_unused(future.get());
		}
		catch(const std::system_error &error) {
			threw = error.code() == std::errc::broken_pipe;
		}
		check(threw, "future propagates error as exception");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor(),
			"POST / HTTP/1.1\r\nHost: test\r\nContent-Length: 4\r\n\r\ncoro");
		auto request = std::make_shared<http::request>(connection);
		bool completed = false;
		asio::co_spawn(io,
		[request, &completed]() -> libgs::awaitable<void>
		{
			auto body = co_await request->read<std::string>(libgs::use_awaitable);
			completed = body == "coro";
		},
		asio::detached);
		io.run();
		check(completed, "coroutine returns value");
	}
}

static void test_error_cancel_timeout(checks &check)
{
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->fail_reads(std::make_error_code(std::errc::connection_reset));
		auto reply = make_reply(connection);
		libgs::error_code received {};
		reply->wait([&](libgs::error_code error, http::status_enum) {
			received = error;
		});
		io.run();
		check(received == std::errc::connection_reset,
			"callback receives operation error argument");
	}
	{
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->stall();
		auto reply = make_reply(connection);
		asio::cancellation_signal signal;
		libgs::error_code received {};
		reply->wait(asio::bind_cancellation_slot(signal.slot(),
			[&](libgs::error_code error, http::status_enum) { received = error; }
		));
		io.poll();
		io.restart();
		signal.emit(asio::cancellation_type::all);
		io.run();
		check(received == libgs::errc::operation_aborted,
			"cancellation slot propagates to I/O");
	}
	{
		using namespace std::chrono_literals;
		asio::io_context io;
		auto connection = std::make_shared<scripted_connection>(io.get_executor());
		connection->stall();
		http::request request(connection);
		libgs::error_code received {};
		request.wait(libgs::redirect_time(
			[&](libgs::error_code error) { received = error; }, 5ms
		));
		io.run();
		check(received == libgs::errc::timed_out,
			"asynchronous timeout reports timed_out");
	}
}

int main()
{
	checks check;
	test_sync(check);
	test_callback_and_lifetime(check);
	test_future_and_coroutine(check);
	test_error_cancel_timeout(check);
	std::cout << "SUMMARY checks=" << check.count << " failed=" << check.failed << '\n';
	return check.failed == 0 ? 0 : 1;
}
