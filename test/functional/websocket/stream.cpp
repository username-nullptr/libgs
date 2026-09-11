// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/websocket/stream.h>
#include <libgs/websocket/protocol/generator.h>
#include <libgs/websocket/protocol/parser.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <future>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace
{

namespace ws = libgs::websocket;

template <typename Stream, typename Connection>
concept adopts_connection = requires(Stream &stream,
	std::shared_ptr<Connection> connection, libgs::error_code &error)
{
	stream.adopt(std::move(connection), {}, error);
};

using native_executor = libgs::io_context_t::executor_type;
using native_stream = ws::basic_stream<native_executor>;
using native_connection = libgs::http::basic_connection<native_executor>;
static_assert(std::same_as<native_stream::connection_t,native_connection>);
static_assert(adopts_connection<native_stream,native_connection>);
static_assert(not adopts_connection<ws::stream,native_connection>);

class memory_connection final : public libgs::http::basic_connection<>
{
public:
	explicit memory_connection(executor_t exec) : m_exec(std::move(exec)) {}

	void feed(std::vector<std::byte> input) {
		m_input = std::move(input);
		m_input_offset = 0;
	}

	void read_chunk_size(size_t value) noexcept {
		m_read_chunk_size = value;
	}

	void fail_after(size_t wire_bytes) noexcept {
		m_fail_after = wire_bytes;
	}

	void stall_reads(bool value = true) noexcept {
		m_stall_reads = value;
	}

	void stall_writes(bool value = true) noexcept {
		m_stall_writes = value;
	}

	[[nodiscard]] const std::vector<std::byte> &wire() const noexcept {
		return m_wire;
	}

	[[nodiscard]] size_t cancel_count() const noexcept {
		return m_cancel_count;
	}

	[[nodiscard]] size_t close_count() const noexcept {
		return m_close_count;
	}

	[[nodiscard]] bool read_pending() const noexcept {
		return static_cast<bool>(m_pending_read);
	}

	libgs::sys_expected<> cancel() noexcept override
	{
		m_cancel_count++;
		complete_pending_read(asio::error::operation_aborted);
		if( m_pending_write )
		{
			auto completion = std::move(m_pending_write);
			asio::post(m_exec, [handler = std::move(completion)]() mutable {
				std::move(handler)(asio::error::operation_aborted, 0);
			});
		}
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> close() noexcept override
	{
		m_close_count++;
		m_open = false;
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<> set_options(
		const libgs::http::tcp_socket_options&) noexcept override
	{
		return libgs::make_sys_expected();
	}

	libgs::sys_expected<libgs::http::tcp_socket_state>
	options() const noexcept override
	{
		return libgs::http::tcp_socket_state {};
	}

	bool is_open() const noexcept override {
		return m_open;
	}

	libgs::sys_expected<probe_state_t> probe() noexcept override {
		return probe_state_t::no_event;
	}

	libgs::http::endpoint remote_endpoint() const noexcept override
	{
		return {.address = asio::ip::make_address_v4("192.0.2.10"), .port = 443};
	}

	libgs::http::endpoint local_endpoint() const noexcept override
	{
		return {.address = asio::ip::make_address_v4("192.0.2.20"), .port = 49152};
	}

	executor_t get_executor() noexcept override {
		return m_exec;
	}

protected:
	size_t read_some(libgs::mutable_buffer buffer,
		libgs::error_code &error) noexcept override
	{
		if( m_input_offset == m_input.size() )
		{
			error = asio::error::eof;
			return 0;
		}
		const auto size = std::min({buffer.size(), m_read_chunk_size,
			m_input.size() - m_input_offset});
		if( size != 0 )
		{
			std::memcpy(buffer.data(), m_input.data() + m_input_offset, size);
			m_input_offset += size;
		}
		error.clear();
		return size;
	}

	size_t write_all(const libgs::const_buffer &buffer,
		libgs::error_code &error) noexcept override
	{
		const auto available = m_fail_after ?
			(*m_fail_after > m_wire.size() ? *m_fail_after - m_wire.size() : 0) :
			std::numeric_limits<size_t>::max();
		const auto size = std::min(buffer.size(), available);
		if( size != 0 )
		{
			const auto *data = static_cast<const std::byte*>(buffer.data());
			m_wire.insert(m_wire.end(), data, data + size);
		}
		error = size == buffer.size() ? libgs::error_code{} :
			std::make_error_code(std::errc::broken_pipe);
		return size;
	}

	void co_read_some(libgs::mutable_buffer buffer,
		io_handler_t completion) noexcept override
	{
		if( m_stall_reads and m_input_offset == m_input.size() )
		{
			m_pending_read = std::move(completion);
			auto slot = asio::get_associated_cancellation_slot(m_pending_read);
			if( slot.is_connected() )
			{
				slot.assign([this](asio::cancellation_type type) noexcept {
					if( type != asio::cancellation_type::none )
						complete_pending_read(
							asio::error::operation_aborted, false);
				});
			}
			return;
		}
		libgs::error_code error;
		auto size = read_some(buffer, error);
		asio::post(m_exec, [handler = std::move(completion), error, size]() mutable {
			std::move(handler)(error, size);
		});
	}

	void co_write_all(libgs::const_buffer buffer,
		io_handler_t completion) noexcept override
	{
		if( m_stall_writes )
		{
			m_pending_write = std::move(completion);
			return;
		}
		libgs::error_code error;
		auto size = write_all(buffer, error);
		asio::post(m_exec,
			[handler = std::move(completion), error, size]() mutable {
				std::move(handler)(error, size);
			});
	}

private:
	void complete_pending_read(libgs::error_code error,
		bool clear_slot = true) noexcept
	{
		if( not m_pending_read )
			return;
		if( clear_slot )
		{
			auto slot = asio::get_associated_cancellation_slot(m_pending_read);
			if( slot.is_connected() )
				slot.clear();
		}
		auto completion = std::move(m_pending_read);
		asio::post(m_exec,
			[handler = std::move(completion), error]() mutable {
				std::move(handler)(error, 0);
			});
	}

	executor_t m_exec;
	std::vector<std::byte> m_input;
	size_t m_input_offset = 0;
	size_t m_read_chunk_size = std::numeric_limits<size_t>::max();
	std::vector<std::byte> m_wire;
	std::optional<size_t> m_fail_after;
	io_handler_t m_pending_read;
	io_handler_t m_pending_write;
	size_t m_cancel_count = 0;
	size_t m_close_count = 0;
	bool m_stall_reads = false;
	bool m_stall_writes = false;
	bool m_open = true;
};

uint8_t octet(const std::vector<std::byte> &wire, size_t offset)
{
	return std::to_integer<uint8_t>(wire.at(offset));
}

void append_frame(std::vector<std::byte> &wire, ws::opcode op, bool fin,
	std::span<const std::byte> payload)
{
	ws::masking_key key {{
		std::byte {0x11}, std::byte {0x22}, std::byte {0x33}, std::byte {0x44}
	}};
	ws::frame_header header {
		.fin = fin,
		.op = op,
		.payload_size = payload.size(),
		.mask = key,
	};
	auto encoded = ws::encode_frame_header(header, {
		.local_role = ws::role::client,
	});
	LIBGS_TEST_CHECK(encoded.has_value());
	const auto old_size = wire.size();
	wire.resize(old_size + encoded->size + payload.size());
	std::memcpy(wire.data() + old_size, encoded->buffer().data(), encoded->size);
	auto copied = ws::mask_copy(libgs::mutable_buffer(
		wire.data() + old_size + encoded->size, payload.size()),
		libgs::const_buffer(payload.data(), payload.size()), key);
	LIBGS_TEST_CHECK(copied.has_value());
}

void append_frame(std::vector<std::byte> &wire, ws::opcode op, bool fin,
	std::string_view payload)
{
	append_frame(wire, op, fin, std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(payload.data()), payload.size()));
}

std::vector<ws::opcode> parse_server_frames(std::vector<std::byte> wire)
{
	ws::frame_parser parser({.local_role = ws::role::client});
	std::vector<ws::opcode> result;
	size_t offset = 0;
	while( offset < wire.size() )
	{
		auto parsed = parser.parse(libgs::mutable_buffer(
			wire.data() + offset, wire.size() - offset));
		LIBGS_TEST_CHECK(parsed.has_value());
		LIBGS_TEST_CHECK(parsed->frame_finished);
		offset += parsed->consumed;
		result.push_back(parser.header().op);
	}
	return result;
}

uint16_t parse_server_close_code(std::vector<std::byte> wire)
{
	ws::frame_parser parser({.local_role = ws::role::client});
	size_t offset = 0;
	while( offset < wire.size() )
	{
		auto parsed = parser.parse(libgs::mutable_buffer(
			wire.data() + offset, wire.size() - offset));
		LIBGS_TEST_CHECK(parsed.has_value());
		LIBGS_TEST_CHECK(parsed->frame_finished);
		offset += parsed->consumed;
		if( parser.header().op != ws::opcode::close )
			continue;
		auto close = ws::decode_close_payload(parsed->payload);
		LIBGS_TEST_CHECK(close.has_value());
		LIBGS_TEST_CHECK(close->code.has_value());
		return *close->code;
	}
	LIBGS_TEST_CHECK(false);
	return 0;
}

std::string payload_text(const std::vector<std::byte> &payload)
{
	return std::string(reinterpret_cast<const char*>(payload.data()),
		payload.size());
}

void test_adopt_and_lifecycle()
{
	libgs::io_context_t context;
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	ws::stream stream(context.get_executor());
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::idle);

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection), {
		.stream_role = ws::role::server,
		.pending_data = {std::byte {'p'}, std::byte {'e'}, std::byte {'n'},
			std::byte {'d'}, std::byte {'i'}, std::byte {'n'}, std::byte {'g'}},
		.negotiated_subprotocol = "chat",
	}, error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK(stream.is_open());
	LIBGS_TEST_CHECK_EQ(stream.stream_role(), ws::role::server);
	LIBGS_TEST_CHECK_EQ(stream.negotiated_subprotocol(), "chat");
	LIBGS_TEST_CHECK_EQ(stream.remote_endpoint().port, uint16_t {443});
	LIBGS_TEST_CHECK_EQ(stream.local_endpoint().port, uint16_t {49152});

	auto second = std::make_shared<memory_connection>(context.get_executor());
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(second), {}, error);
	LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::already_open));
	LIBGS_TEST_CHECK(second->is_open());

	stream.shutdown(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});

	stream.shutdown(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
}

void test_adopt_validation()
{
	libgs::io_context_t context;
	libgs::error_code error;

	ws::stream null_stream(context.get_executor());
	null_stream.adopt(std::shared_ptr<libgs::http::connection> {}, {}, error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::invalid_argument));
	LIBGS_TEST_CHECK_EQ(null_stream.state(), ws::connection_state::idle);

	ws::stream extension_stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	ws::adopt_options options;
	options.negotiated_extensions.push_back({.name = "permessage-deflate"});
	extension_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(connection),
		std::move(options), error);
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::errc::unsupported_extension));
	LIBGS_TEST_CHECK(connection->is_open());

	ws::stream_config bad_config;
	bad_config.read_buffer_size = 0;
	ws::stream invalid_stream(context.get_executor(), bad_config);
	invalid_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(connection), {}, error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::invalid_argument));

	libgs::io_context_t other_context;
	ws::stream mismatched_stream(context.get_executor());
	auto other_connection =
		std::make_shared<memory_connection>(other_context.get_executor());
	mismatched_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(other_connection), {}, error);
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::invalid_argument));
	LIBGS_TEST_CHECK(other_connection->is_open());
}

void test_server_write_and_fragmentation()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	const auto transferred = stream.write_text("hello", error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(transferred, size_t {5});

	const auto &wire = connection->wire();
	LIBGS_TEST_CHECK_EQ(wire.size(), size_t {11});
	LIBGS_TEST_CHECK_EQ(octet(wire, 0), 0x01);
	LIBGS_TEST_CHECK_EQ(octet(wire, 1), 0x02);
	LIBGS_TEST_CHECK(std::memcmp(wire.data() + 2, "he", 2) == 0);
	LIBGS_TEST_CHECK_EQ(octet(wire, 4), 0x00);
	LIBGS_TEST_CHECK_EQ(octet(wire, 5), 0x02);
	LIBGS_TEST_CHECK(std::memcmp(wire.data() + 6, "ll", 2) == 0);
	LIBGS_TEST_CHECK_EQ(octet(wire, 8), 0x80);
	LIBGS_TEST_CHECK_EQ(octet(wire, 9), 0x01);
	LIBGS_TEST_CHECK_EQ(octet(wire, 10), static_cast<uint8_t>('o'));

	LIBGS_TEST_CHECK_EQ(stream.write_binary(libgs::const_buffer {}, error),
		size_t {0});
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(connection->wire().size(), size_t {13});
	LIBGS_TEST_CHECK_EQ(octet(connection->wire(), 11), 0x82);
	LIBGS_TEST_CHECK_EQ(octet(connection->wire(), 12), 0x00);
}

void test_client_write_is_masked()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 0;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::client}, error);

	const std::array<std::byte,4> payload {
		std::byte {0x10}, std::byte {0x20}, std::byte {0x30}, std::byte {0x40}
	};
	const std::array<libgs::const_buffer,2> payload_buffers {
		libgs::const_buffer(payload.data(), 1),
		libgs::const_buffer(payload.data() + 1, payload.size() - 1)
	};
	const auto transferred = stream.write_binary(
		libgs::const_buffer(payload.data(), payload.size()), error
	);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(transferred, payload.size());
	LIBGS_TEST_CHECK_EQ(connection->wire().size(), size_t {10});
	LIBGS_TEST_CHECK((octet(connection->wire(), 1) & 0x80) != 0);

	auto wire = connection->wire();
	ws::frame_parser parser({.local_role = ws::role::server});
	auto parsed = parser.parse(libgs::mutable_buffer(wire.data(), wire.size()));
	LIBGS_TEST_CHECK(parsed.has_value());
	LIBGS_TEST_CHECK(parsed->frame_finished);
	LIBGS_TEST_CHECK(parser.header().mask.has_value());
	ws::apply_mask(parsed->payload, *parser.header().mask,
		parsed->payload_offset);
	LIBGS_TEST_CHECK(parsed->payload.size() == payload.size());
	LIBGS_TEST_CHECK(std::memcmp(parsed->payload.data(), payload.data(),
		payload.size()) == 0);

	const auto split_transferred = stream.write(ws::message_type::binary,
		payload_buffers, error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(split_transferred, payload.size());
	auto split_wire = std::vector<std::byte>(
		connection->wire().begin() + 10, connection->wire().end());
	ws::frame_parser split_parser({.local_role = ws::role::server});
	auto split_parsed = split_parser.parse(
		libgs::mutable_buffer(split_wire.data(), split_wire.size()));
	LIBGS_TEST_CHECK(split_parsed.has_value());
	LIBGS_TEST_CHECK(split_parser.header().mask.has_value());
	ws::apply_mask(split_parsed->payload, *split_parser.header().mask,
		split_parsed->payload_offset);
	LIBGS_TEST_CHECK(std::memcmp(split_parsed->payload.data(), payload.data(),
		payload.size()) == 0);
}

void test_preflight_and_partial_failure()
{
	libgs::io_context_t context;
	libgs::error_code error;

	ws::stream idle(context.get_executor());
	LIBGS_TEST_CHECK_EQ(idle.write_text("x", error), size_t {0});
	LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::not_open));

	ws::stream text_stream(context.get_executor());
	auto text_connection =
		std::make_shared<memory_connection>(context.get_executor());
	text_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(text_connection),
		{.stream_role = ws::role::server}, error);
	const std::array<std::byte,3> invalid_utf8 {
		std::byte {'a'}, std::byte {0xC0}, std::byte {0x80}
	};
	LIBGS_TEST_CHECK_EQ(text_stream.write(ws::message_type::text,
		libgs::const_buffer(invalid_utf8.data(), invalid_utf8.size()), error),
		size_t {0});
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::protocol_errc::invalid_utf8));
	LIBGS_TEST_CHECK(text_connection->wire().empty());
	LIBGS_TEST_CHECK(text_stream.is_open());

	const std::array<std::byte,4> split_utf8 {
		std::byte {0xF0}, std::byte {0x9F}, std::byte {0x98}, std::byte {0x80}
	};
	const std::array<libgs::const_buffer,3> split_utf8_buffers {
		libgs::const_buffer(split_utf8.data(), 1),
		libgs::const_buffer(split_utf8.data() + 1, 1),
		libgs::const_buffer(split_utf8.data() + 2, 2)
	};
	LIBGS_TEST_CHECK_EQ(text_stream.write(ws::message_type::text,
		split_utf8_buffers, error), split_utf8.size());
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(text_connection->wire().size(), size_t {6});
	LIBGS_TEST_CHECK(std::memcmp(text_connection->wire().data() + 2,
		split_utf8.data(), split_utf8.size()) == 0);

	ws::stream_config limited_config;
	limited_config.max_message_size = 4;
	ws::stream limited_stream(context.get_executor(), limited_config);
	auto limited_connection =
		std::make_shared<memory_connection>(context.get_executor());
	limited_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(limited_connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK_EQ(limited_stream.write_text("hello", error), size_t {0});
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::errc::message_too_big));
	LIBGS_TEST_CHECK(limited_connection->wire().empty());
	LIBGS_TEST_CHECK(limited_stream.is_open());

	ws::stream partial_stream(context.get_executor());
	auto partial_connection =
		std::make_shared<memory_connection>(context.get_executor());
	partial_connection->fail_after(4); // 2-byte header plus 2 payload bytes.
	partial_stream.adopt(
		std::static_pointer_cast<libgs::http::connection>(partial_connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK_EQ(partial_stream.write_text("hello", error), size_t {2});
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::broken_pipe));
	LIBGS_TEST_CHECK_EQ(partial_stream.state(), ws::connection_state::failed);
	LIBGS_TEST_CHECK(not partial_connection->is_open());
	LIBGS_TEST_CHECK_EQ(partial_connection->wire().size(), size_t {4});

	LIBGS_TEST_CHECK_EQ(partial_stream.write_text("again", error), size_t {0});
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::broken_pipe));
	partial_stream.shutdown(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(partial_connection->close_count(), size_t {1});
}

void test_async_write()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto completed = stream.write_text("async", libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(completed.get(), size_t {5});
	LIBGS_TEST_CHECK_EQ(connection->wire().size(), size_t {7});
	LIBGS_TEST_CHECK_EQ(static_cast<unsigned char>(connection->wire()[0]), 0x81);

	stream.shutdown(error);
	LIBGS_TEST_CHECK(not error);
}

void test_control_write()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	LIBGS_TEST_CHECK_EQ(stream.ping(libgs::const_buffer("hi", 2), error),
		size_t {2});
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(stream.pong(error), size_t {0});
	LIBGS_TEST_CHECK(not error);
	const std::vector<ws::opcode> expected {ws::opcode::ping, ws::opcode::pong};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);

	std::array<std::byte,126> oversized {};
	LIBGS_TEST_CHECK_EQ(stream.ping(libgs::const_buffer(
		oversized.data(), oversized.size()), error), size_t {0});
	LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(
		ws::protocol_errc::control_payload_too_large));
	LIBGS_TEST_CHECK(stream.is_open());
}

void test_write_queue_fairness_and_barrier()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto first = stream.write_text("abcdef", libgs::use_future);
	auto ping = stream.ping(libgs::const_buffer("p", 1), libgs::use_future);
	auto pong = stream.pong(libgs::const_buffer("q", 1), libgs::use_future);
	auto second = stream.write_text("xy", libgs::use_future);
	auto barrier = stream.wait_written(libgs::use_future);
	context.run();

	LIBGS_TEST_CHECK_EQ(first.get(), size_t {6});
	LIBGS_TEST_CHECK_EQ(ping.get(), size_t {1});
	LIBGS_TEST_CHECK_EQ(pong.get(), size_t {1});
	LIBGS_TEST_CHECK_EQ(second.get(), size_t {2});
	barrier.get();
	const std::vector<ws::opcode> expected {
		ws::opcode::text,
		ws::opcode::ping,
		ws::opcode::continuation,
		ws::opcode::pong,
		ws::opcode::continuation,
		ws::opcode::text,
	};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
}

void test_write_queue_limits()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 0;
	config.max_queued_write_operations = 1;
	config.max_queued_write_bytes = 3;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto active = stream.write_text("active", libgs::use_future);
	auto queued = stream.write_text("123", libgs::use_future);
	auto rejected = stream.write_text("x", libgs::use_future);
	auto rejected_control = stream.ping(libgs::use_future);
	auto barrier = stream.wait_written(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(active.get(), size_t {6});
	LIBGS_TEST_CHECK_EQ(queued.get(), size_t {3});
	barrier.get();

	for( auto *future : {&rejected, &rejected_control} )
	{
		try
		{
			libgs::ignore_unused(future->get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(),
				ws::make_error_code(ws::errc::write_queue_full));
		}
	}
	LIBGS_TEST_CHECK(stream.is_open());
}

void test_queued_write_cancellation()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 0;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	std::future<size_t> active;
	std::future<size_t> cancelled_data;
	std::future<size_t> following_data;
	std::future<size_t> cancelled_ping;
	std::future<void> barrier;
	asio::cancellation_signal data_cancellation;
	asio::cancellation_signal ping_cancellation;
	asio::post(context, [&] {
		active = stream.write_text("active", libgs::use_future);
		cancelled_data = stream.write_text("cancelled",
			asio::bind_cancellation_slot(
				data_cancellation.slot(), libgs::use_future));
		following_data = stream.write_text("following", libgs::use_future);
		cancelled_ping = stream.ping(libgs::const_buffer("p", 1),
			asio::bind_cancellation_slot(
				ping_cancellation.slot(), libgs::use_future));
		data_cancellation.emit(asio::cancellation_type::terminal);
		ping_cancellation.emit(asio::cancellation_type::terminal);
		barrier = stream.wait_written(libgs::use_future);
	});
	context.run();

	LIBGS_TEST_CHECK_EQ(active.get(), size_t {6});
	LIBGS_TEST_CHECK_EQ(following_data.get(), size_t {9});
	for( auto *future : {&cancelled_data, &cancelled_ping} )
	{
		try
		{
			libgs::ignore_unused(future->get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(),
				asio::error::make_error_code(asio::error::operation_aborted));
		}
	}
	try
	{
		barrier.get();
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::operation_aborted));
	}
	const std::vector<ws::opcode> expected {
		ws::opcode::text, ws::opcode::text,
	};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
	LIBGS_TEST_CHECK(stream.is_open());
}

void test_write_barrier_cancellation()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	std::future<size_t> write;
	std::future<void> cancelled;
	std::future<void> observer;
	asio::cancellation_signal cancellation;
	asio::post(context, [&] {
		write = stream.write_text("still written", libgs::use_future);
		cancelled = stream.wait_written(asio::bind_cancellation_slot(
			cancellation.slot(), libgs::use_future));
		observer = stream.wait_written(libgs::redirect_time(
			libgs::use_future, std::chrono::seconds(1)));
		cancellation.emit(asio::cancellation_type::terminal);
	});
	context.run();

	LIBGS_TEST_CHECK_EQ(write.get(), size_t {13});
	try
	{
		cancelled.get();
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::operation_aborted));
	}
	observer.get();
	const std::vector<ws::opcode> expected {ws::opcode::text};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {0});
	LIBGS_TEST_CHECK(stream.is_open());
}

void test_detached_write_barrier_error()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->fail_after(4); // 2-byte header plus 2 payload bytes.
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	stream.write_text("hello", libgs::detached);
	auto barrier = stream.wait_written(libgs::use_future);
	context.run();
	try
	{
		barrier.get();
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			std::make_error_code(std::errc::broken_pipe));
	}
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::failed);

	context.restart();
	auto observed = stream.wait_written(libgs::use_future);
	context.run();
	observed.get();
}

void test_detached_write_owns_payload()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	std::string first = "ab";
	std::string second = "cdef";
	const std::array<libgs::const_buffer,2> payload {
		libgs::buffer(first), libgs::buffer(second)
	};
	stream.write(ws::message_type::text, payload,
		libgs::redirect_time(libgs::detached, std::chrono::seconds(1)));
	std::ranges::fill(first, 'x');
	std::ranges::fill(second, 'x');
	auto barrier = stream.wait_written(libgs::use_future);
	context.run();
	barrier.get();

	const auto &wire = connection->wire();
	LIBGS_TEST_CHECK_EQ(wire.size(), size_t {12});
	LIBGS_TEST_CHECK(std::memcmp(wire.data() + 2, "ab", 2) == 0);
	LIBGS_TEST_CHECK(std::memcmp(wire.data() + 6, "cd", 2) == 0);
	LIBGS_TEST_CHECK(std::memcmp(wire.data() + 10, "ef", 2) == 0);
}

void test_control_failure_completes_paused_data()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->fail_after(5); // First data frame, then one Ping header byte.
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	bool data_done = false;
	bool ping_done = false;
	size_t data_size = 0;
	libgs::error_code data_error;
	libgs::error_code ping_error;
	stream.write_text("abcdef", [&](libgs::error_code result, size_t size) {
		data_done = true;
		data_error = result;
		data_size = size;
	});
	stream.ping(libgs::const_buffer("p", 1),
		[&](libgs::error_code result, size_t) {
			ping_done = true;
			ping_error = result;
		});
	auto barrier = stream.wait_written(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK(data_done);
	LIBGS_TEST_CHECK(ping_done);
	LIBGS_TEST_CHECK_EQ(data_size, size_t {2});
	LIBGS_TEST_CHECK_EQ(data_error, std::make_error_code(std::errc::broken_pipe));
	LIBGS_TEST_CHECK_EQ(ping_error, std::make_error_code(std::errc::broken_pipe));
	try
	{
		barrier.get();
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			std::make_error_code(std::errc::broken_pipe));
	}
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::failed);
}

void test_shutdown_aborts_write_queue()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	bool first_done = false;
	bool second_done = false;
	libgs::error_code first_error;
	libgs::error_code second_error;
	stream.write_text("first", [&](libgs::error_code result, size_t) {
		first_done = true;
		first_error = result;
	});
	stream.write_text("second", [&](libgs::error_code result, size_t) {
		second_done = true;
		second_error = result;
	});
	auto closed = stream.wait_closed(libgs::use_future);
	stream.shutdown(error);
	LIBGS_TEST_CHECK(not error);
	context.run();
	LIBGS_TEST_CHECK(first_done);
	LIBGS_TEST_CHECK(second_done);
	LIBGS_TEST_CHECK_EQ(first_error,
		asio::error::make_error_code(asio::error::operation_aborted));
	LIBGS_TEST_CHECK_EQ(second_error,
		asio::error::make_error_code(asio::error::operation_aborted));
	try
	{
		libgs::ignore_unused(closed.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::operation_aborted));
	}
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	auto retained = stream.wait_closed(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK(not retained.clean);
}

void test_read_fragmentation_and_automatic_pong()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.read_buffer_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->read_chunk_size(1);

	std::vector<std::byte> input;
	append_frame(input, ws::opcode::text, false, "he");
	append_frame(input, ws::opcode::ping, true, "p");
	append_frame(input, ws::opcode::continuation, true, "llo");
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto value = stream.read<std::string>(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(value.type, ws::message_type::text);
	LIBGS_TEST_CHECK_EQ(value.body, "hello");

	const auto &wire = connection->wire();
	LIBGS_TEST_CHECK_EQ(wire.size(), size_t {3});
	LIBGS_TEST_CHECK_EQ(octet(wire, 0), uint8_t {0x8A});
	LIBGS_TEST_CHECK_EQ(octet(wire, 1), uint8_t {0x01});
	LIBGS_TEST_CHECK_EQ(octet(wire, 2), static_cast<uint8_t>('p'));
}

void test_read_pending_multiple_messages()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	std::vector<std::byte> pending;
	append_frame(pending, ws::opcode::binary, true, "one");
	append_frame(pending, ws::opcode::text, true, "two");

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection), {
		.stream_role = ws::role::server,
		.pending_data = std::move(pending),
	}, error);
	LIBGS_TEST_CHECK(not error);

	auto first = stream.read<>(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(first.type, ws::message_type::binary);
	LIBGS_TEST_CHECK_EQ(first.body.size(), size_t {3});
	LIBGS_TEST_CHECK(std::memcmp(first.body.data(), "one", 3) == 0);

	auto second = stream.read<std::string>(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(second.type, ws::message_type::text);
	LIBGS_TEST_CHECK_EQ(second.body, "two");
}

void test_async_read()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.read_buffer_size = 3;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->read_chunk_size(2);
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::text, true, "async read");
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto completed = stream.read<std::string>(libgs::use_future);
	context.run();
	auto value = completed.get();
	LIBGS_TEST_CHECK_EQ(value.type, ws::message_type::text);
	LIBGS_TEST_CHECK_EQ(value.body, "async read");
}

void test_async_read_cancellation_preserves_parser()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->stall_reads();

	std::vector<std::byte> wire;
	append_frame(wire, ws::opcode::text, true, "hello");
	constexpr size_t split = 7; // Header, masking key, and one payload byte.
	std::vector<std::byte> prefix(wire.begin(), wire.begin() + split);
	std::vector<std::byte> suffix(wire.begin() + split, wire.end());
	connection->feed(std::move(prefix));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	asio::cancellation_signal cancellation;
	auto first = stream.read<std::string>(asio::bind_cancellation_slot(
		cancellation.slot(), libgs::use_future));
	while( not connection->read_pending() )
		LIBGS_TEST_CHECK_EQ(context.poll_one(), size_t {1});
	cancellation.emit(asio::cancellation_type::terminal);
	context.run();
	try
	{
		libgs::ignore_unused(first.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::operation_aborted));
	}
	LIBGS_TEST_CHECK(stream.is_open());
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {0});

	connection->stall_reads(false);
	connection->feed(std::move(suffix));
	context.restart();
	auto resumed = stream.read<std::string>(libgs::use_future);
	context.run();
	auto value = resumed.get();
	LIBGS_TEST_CHECK_EQ(value.type, ws::message_type::text);
	LIBGS_TEST_CHECK_EQ(value.body, "hello");
}

void test_async_read_timeout()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->stall_reads();
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto timed = stream.read<std::string>(libgs::redirect_time(
		libgs::use_future, std::chrono::milliseconds(1)));
	context.run();
	try
	{
		libgs::ignore_unused(timed.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::timed_out));
	}
	LIBGS_TEST_CHECK(stream.is_open());
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {0});

	std::vector<std::byte> wire;
	append_frame(wire, ws::opcode::binary, true, "after timeout");
	connection->stall_reads(false);
	connection->feed(std::move(wire));
	context.restart();
	auto resumed = stream.read<std::string>(libgs::use_future);
	context.run();
	auto value = resumed.get();
	LIBGS_TEST_CHECK_EQ(value.type, ws::message_type::binary);
	LIBGS_TEST_CHECK_EQ(value.body, "after timeout");
}

void test_read_limits_and_utf8()
{
	libgs::io_context_t context;
	libgs::error_code error;

	ws::stream_config limited_config;
	limited_config.max_message_size = 4;
	ws::stream limited(context.get_executor(), limited_config);
	auto limited_connection =
		std::make_shared<memory_connection>(context.get_executor());
	std::vector<std::byte> oversized;
	append_frame(oversized, ws::opcode::binary, false, "123");
	append_frame(oversized, ws::opcode::continuation, true, "45");
	limited_connection->feed(std::move(oversized));
	limited.adopt(std::static_pointer_cast<libgs::http::connection>(
		limited_connection), {.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	libgs::ignore_unused(limited.read<>(error));
	LIBGS_TEST_CHECK_EQ(error, ws::make_error_code(ws::errc::message_too_big));
	LIBGS_TEST_CHECK_EQ(limited.state(), ws::connection_state::failed);
	LIBGS_TEST_CHECK_EQ(parse_server_close_code(limited_connection->wire()),
		static_cast<uint16_t>(ws::close_code::message_too_big));
	LIBGS_TEST_CHECK_EQ(limited_connection->close_count(), size_t {1});

	ws::stream invalid(context.get_executor());
	auto invalid_connection =
		std::make_shared<memory_connection>(context.get_executor());
	const std::array<std::byte,2> invalid_text {
		std::byte {0xC0}, std::byte {0x80}
	};
	std::vector<std::byte> invalid_wire;
	append_frame(invalid_wire, ws::opcode::text, true, invalid_text);
	invalid_connection->feed(std::move(invalid_wire));
	invalid.adopt(std::static_pointer_cast<libgs::http::connection>(
		invalid_connection), {.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	libgs::ignore_unused(invalid.read<>(error));
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::protocol_errc::invalid_utf8));
	LIBGS_TEST_CHECK_EQ(invalid.state(), ws::connection_state::failed);
	LIBGS_TEST_CHECK_EQ(parse_server_close_code(invalid_connection->wire()),
		static_cast<uint16_t>(ws::close_code::invalid_payload));
	LIBGS_TEST_CHECK_EQ(invalid_connection->close_count(), size_t {1});
}

void test_protocol_failure_during_async_write()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());

	// FIN + reserved opcode 0x3, masked empty payload. Keeping this in pending
	// data lets the read coroutine observe the error while the first data frame
	// owns the asynchronous transport write.
	std::vector<std::byte> invalid_frame {
		std::byte {0x83}, std::byte {0x80},
		std::byte {0x11}, std::byte {0x22},
		std::byte {0x33}, std::byte {0x44},
	};
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection), {
		.stream_role = ws::role::server,
		.pending_data = std::move(invalid_frame),
	}, error);
	LIBGS_TEST_CHECK(not error);

	auto received = stream.read<>(libgs::use_future);
	auto written = stream.write_text("abcd", libgs::use_future);
	context.run();

	for( auto action : {0, 1} )
	{
		try
		{
			if( action == 0 )
				libgs::ignore_unused(received.get());
			else
				libgs::ignore_unused(written.get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK(exception.code() ==
				ws::protocol_errc::reserved_opcode);
		}
	}

	const std::vector<ws::opcode> expected {
		ws::opcode::text, ws::opcode::close,
	};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
	LIBGS_TEST_CHECK_EQ(parse_server_close_code(connection->wire()),
		static_cast<uint16_t>(ws::close_code::protocol_error));
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::failed);
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
}

void test_protocol_failure_close_deadline()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.close_timeout = std::chrono::milliseconds(1);
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->stall_writes();

	std::vector<std::byte> invalid_frame {
		std::byte {0x83}, std::byte {0x80},
		std::byte {0x11}, std::byte {0x22},
		std::byte {0x33}, std::byte {0x44},
	};
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection), {
		.stream_role = ws::role::server,
		.pending_data = std::move(invalid_frame),
	}, error);
	LIBGS_TEST_CHECK(not error);

	auto received = stream.read<>(libgs::use_future);
	context.run();
	try
	{
		libgs::ignore_unused(received.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK(exception.code() ==
			ws::protocol_errc::reserved_opcode);
	}

	LIBGS_TEST_CHECK(connection->wire().empty());
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::failed);
	libgs::ignore_unused(stream.read<>(error));
	LIBGS_TEST_CHECK(error == ws::protocol_errc::reserved_opcode);
}

void test_wait_ctrl_and_retention()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::ping, true, "ping");
	append_frame(input, ws::opcode::pong, true, "pong");
	append_frame(input, ws::opcode::text, true, "reply");
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	bool control_done = false;
	bool pong_queued_before_completion = false;
	ws::control_event first;
	stream.wait_ctrl([&](libgs::error_code result, ws::control_event event) {
		LIBGS_TEST_CHECK(not result);
		control_done = true;
		first = std::move(event);
		pong_queued_before_completion = not connection->wire().empty();
	});
	auto received = stream.read<std::string>(libgs::use_future);
	context.run();

	LIBGS_TEST_CHECK(control_done);
	LIBGS_TEST_CHECK(pong_queued_before_completion);
	LIBGS_TEST_CHECK_EQ(first.type, ws::control_type::ping);
	LIBGS_TEST_CHECK_EQ(payload_text(first.payload), "ping");
	LIBGS_TEST_CHECK_EQ(received.get().body, "reply");
	const std::vector<ws::opcode> expected {ws::opcode::pong};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);

	auto retained = stream.wait_ctrl(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(retained.type, ws::control_type::pong);
	LIBGS_TEST_CHECK_EQ(payload_text(retained.payload), "pong");
	libgs::ignore_unused(stream.wait_ctrl(error));
	LIBGS_TEST_CHECK_EQ(error,
		std::make_error_code(std::errc::operation_would_block));
}

void test_wait_ctrl_manual_pong()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.automatic_pong = false;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::ping, true, "manual");
	append_frame(input, ws::opcode::pong, true, "noise");
	append_frame(input, ws::opcode::binary, true, "data");
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto received = stream.read<>(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(received.get().body.size(), size_t {4});
	LIBGS_TEST_CHECK(connection->wire().empty());

	auto event = stream.wait_ctrl(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(event.type, ws::control_type::ping);
	LIBGS_TEST_CHECK_EQ(payload_text(event.payload), "manual");
	LIBGS_TEST_CHECK_EQ(stream.pong(libgs::const_buffer(
		event.payload.data(), event.payload.size()), error), event.payload.size());
	LIBGS_TEST_CHECK(not error);
	const std::vector<ws::opcode> expected {ws::opcode::pong};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
}

void test_wait_ctrl_overlap_and_cancellation()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	asio::cancellation_signal cancellation;
	auto cancelled = stream.wait_ctrl(asio::bind_cancellation_slot(
		cancellation.slot(), libgs::use_future));
	auto overlap = stream.wait_ctrl(libgs::use_future);
	cancellation.emit(asio::cancellation_type::terminal);
	context.run();
	for( auto result : {
		std::pair {&cancelled,
			asio::error::make_error_code(asio::error::operation_aborted)},
		std::pair {&overlap,
			std::make_error_code(std::errc::operation_in_progress)},
	} )
	{
		try
		{
			libgs::ignore_unused(result.first->get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(), result.second);
		}
	}
	LIBGS_TEST_CHECK(stream.is_open());

	context.restart();
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::ping, true, "next");
	append_frame(input, ws::opcode::text, true, "ok");
	connection->feed(std::move(input));
	auto next = stream.wait_ctrl(libgs::use_future);
	auto received = stream.read<std::string>(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(next.get().type, ws::control_type::ping);
	LIBGS_TEST_CHECK_EQ(received.get().body, "ok");
}

void test_wait_ctrl_eof()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto control = stream.wait_ctrl(libgs::use_future);
	auto received = stream.read<>(libgs::use_future);
	context.run();
	try
	{
		libgs::ignore_unused(control.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::eof));
	}
	try
	{
		libgs::ignore_unused(received.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::eof));
	}
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
}

void test_wait_ctrl_terminal_transitions()
{
	{
		libgs::io_context_t context;
		ws::stream stream(context.get_executor());
		auto connection =
			std::make_shared<memory_connection>(context.get_executor());
		auto close_payload = ws::encode_close_payload(ws::close_frame {
			ws::close_code::normal_closure, "done"
		});
		LIBGS_TEST_CHECK(close_payload.has_value());
		std::vector<std::byte> input;
		append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
			close_payload->storage.data(), close_payload->size));
		connection->feed(std::move(input));

		libgs::error_code error;
		stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
			{.stream_role = ws::role::server}, error);
		LIBGS_TEST_CHECK(not error);
		auto control = stream.wait_ctrl(libgs::use_future);
		auto received = stream.read<>(libgs::use_future);
		context.run();
		try
		{
			libgs::ignore_unused(control.get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(),
				ws::make_error_code(ws::errc::closing));
		}
		try
		{
			libgs::ignore_unused(received.get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(),
				asio::error::make_error_code(asio::error::eof));
		}
		LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	}

	{
		libgs::io_context_t context;
		ws::stream stream(context.get_executor());
		auto connection =
			std::make_shared<memory_connection>(context.get_executor());
		libgs::error_code error;
		stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
			{.stream_role = ws::role::server}, error);
		LIBGS_TEST_CHECK(not error);
		auto control = stream.wait_ctrl(libgs::use_future);
		stream.shutdown(error);
		LIBGS_TEST_CHECK(not error);
		context.run();
		try
		{
			libgs::ignore_unused(control.get());
			LIBGS_TEST_CHECK(false);
		}
		catch(const std::system_error &exception)
		{
			LIBGS_TEST_CHECK_EQ(exception.code(),
				asio::error::make_error_code(asio::error::operation_aborted));
		}
	}
}

void test_peer_close()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	auto close_payload = ws::encode_close_payload(ws::close_frame {
		ws::close_code::normal_closure, "done"
	});
	LIBGS_TEST_CHECK(close_payload.has_value());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
		close_payload->storage.data(), close_payload->size));
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	libgs::ignore_unused(stream.read<>(error));
	LIBGS_TEST_CHECK_EQ(error, asio::error::make_error_code(asio::error::eof));
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	LIBGS_TEST_CHECK(not connection->is_open());
	auto close = stream.peer_close();
	LIBGS_TEST_CHECK(close.has_value());
	LIBGS_TEST_CHECK_EQ(close->code, libgs::optional<uint16_t> {1000});
	LIBGS_TEST_CHECK_EQ(close->reason, "done");
	LIBGS_TEST_CHECK(close->clean);
	LIBGS_TEST_CHECK_EQ(octet(connection->wire(), 0), uint8_t {0x88});
}

void test_automatic_pong_during_async_write()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::ping, true, "p");
	append_frame(input, ws::opcode::text, true, "reply");
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto written = stream.write_text("hello", libgs::use_future);
	auto received = stream.read<std::string>(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(written.get(), size_t {5});
	LIBGS_TEST_CHECK_EQ(received.get().body, "reply");
	LIBGS_TEST_CHECK(stream.is_open());

	auto wire = connection->wire();
	ws::frame_parser parser({.local_role = ws::role::client});
	size_t offset = 0;
	size_t data_frames = 0;
	size_t pong_frames = 0;
	while( offset < wire.size() )
	{
		auto parsed = parser.parse(libgs::mutable_buffer(
			wire.data() + offset, wire.size() - offset));
		LIBGS_TEST_CHECK(parsed.has_value());
		LIBGS_TEST_CHECK(parsed->frame_finished);
		offset += parsed->consumed;
		if( parser.header().op == ws::opcode::pong )
			pong_frames++;
		else if( parser.header().op == ws::opcode::text or
			parser.header().op == ws::opcode::continuation )
			data_frames++;
	}
	LIBGS_TEST_CHECK_EQ(data_frames, size_t {3});
	LIBGS_TEST_CHECK_EQ(pong_frames, size_t {1});
}

void test_peer_close_during_async_write()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 2;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	auto close_payload = ws::encode_close_payload(ws::close_frame {
		ws::close_code::going_away, "bye"
	});
	LIBGS_TEST_CHECK(close_payload.has_value());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
		close_payload->storage.data(), close_payload->size));
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto written = stream.write_text("hello", libgs::use_future);
	auto received = stream.read<>(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_EQ(written.get(), size_t {5});
	try
	{
		libgs::ignore_unused(received.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::eof));
	}
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	LIBGS_TEST_CHECK(stream.peer_close()->clean);
	LIBGS_TEST_CHECK(not connection->is_open());

	auto wire = connection->wire();
	ws::frame_parser parser({.local_role = ws::role::client});
	size_t offset = 0;
	size_t close_frames = 0;
	while( offset < wire.size() )
	{
		auto parsed = parser.parse(libgs::mutable_buffer(
			wire.data() + offset, wire.size() - offset));
		LIBGS_TEST_CHECK(parsed.has_value());
		LIBGS_TEST_CHECK(parsed->frame_finished);
		offset += parsed->consumed;
		if( parser.header().op == ws::opcode::close )
			close_frames++;
	}
	LIBGS_TEST_CHECK_EQ(close_frames, size_t {1});
}

void test_local_close_sync()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	auto peer_payload = ws::encode_close_payload(ws::close_frame {
		ws::close_code::going_away, "peer"
	});
	LIBGS_TEST_CHECK(peer_payload.has_value());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
		peer_payload->storage.data(), peer_payload->size));
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	libgs::ignore_unused(stream.close(ws::close_frame {1005}, error));
	LIBGS_TEST_CHECK_EQ(error,
		ws::make_error_code(ws::protocol_errc::invalid_close_payload));
	LIBGS_TEST_CHECK(stream.is_open());
	LIBGS_TEST_CHECK(connection->wire().empty());

	auto result = stream.close(ws::close_frame {
		ws::close_code::normal_closure, "local"
	}, error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(result.code, libgs::optional<uint16_t> {1001});
	LIBGS_TEST_CHECK_EQ(result.reason, "peer");
	LIBGS_TEST_CHECK(result.clean);
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
	const std::vector<ws::opcode> expected {ws::opcode::close};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);

	auto retained = stream.wait_closed(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(retained.reason, "peer");
	LIBGS_TEST_CHECK(retained.clean);
}

void test_local_close_async_join_and_drain()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.write_fragment_size = 0;
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	auto peer_payload = ws::encode_close_payload(ws::close_frame {
		ws::close_code::normal_closure, "ack"
	});
	LIBGS_TEST_CHECK(peer_payload.has_value());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
		peer_payload->storage.data(), peer_payload->size));
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);
	auto first_write = stream.write_text("one", libgs::use_future);
	auto second_write = stream.write_text("two", libgs::use_future);
	auto first = stream.close(ws::close_frame {
		ws::close_code::normal_closure, "request"
	}, libgs::use_future);
	// Once closing has begun, later Close parameters are deliberately ignored.
	auto joined = stream.close(ws::close_frame {1005, "invalid but ignored"},
		libgs::use_future);
	auto observed = stream.wait_closed(libgs::use_future);
	context.run();

	LIBGS_TEST_CHECK_EQ(first_write.get(), size_t {3});
	LIBGS_TEST_CHECK_EQ(second_write.get(), size_t {3});
	for( auto *future : {&first, &joined, &observed} )
	{
		auto result = future->get();
		LIBGS_TEST_CHECK_EQ(result.reason, "ack");
		LIBGS_TEST_CHECK(result.clean);
	}
	const std::vector<ws::opcode> expected {
		ws::opcode::text, ws::opcode::text, ws::opcode::close,
	};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});

	// A completed stream retains the result and does not emit another frame.
	const auto wire_size = connection->wire().size();
	auto repeated = stream.close(ws::close_frame {1005}, error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK(repeated.clean);
	LIBGS_TEST_CHECK_EQ(connection->wire().size(), wire_size);
}

void test_local_close_takes_over_read()
{
	libgs::io_context_t context;
	ws::stream stream(context.get_executor());
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	auto peer_payload = ws::encode_close_payload(ws::close_frame {
		ws::close_code::normal_closure, "done"
	});
	LIBGS_TEST_CHECK(peer_payload.has_value());
	std::vector<std::byte> input;
	append_frame(input, ws::opcode::close, true, std::span<const std::byte>(
		peer_payload->storage.data(), peer_payload->size));
	connection->feed(std::move(input));

	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	auto received = stream.read<>(libgs::use_future);
	auto closed = stream.close(libgs::use_future);
	context.run();

	try
	{
		libgs::ignore_unused(received.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(), ws::make_error_code(ws::errc::closing));
	}
	LIBGS_TEST_CHECK(closed.get().clean);
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
}

void test_local_close_immediate_timeout()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.close_timeout = std::chrono::milliseconds::zero();
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto closing = stream.close(libgs::use_future);
	context.run();
	try
	{
		libgs::ignore_unused(closing.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::timed_out));
	}
	LIBGS_TEST_CHECK(connection->wire().empty());
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);

	auto retained = stream.wait_closed(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK(not retained.clean);
	LIBGS_TEST_CHECK(not retained.code);
}

void test_local_close_deadline_cancels_read()
{
	libgs::io_context_t context;
	ws::stream_config config;
	config.close_timeout = std::chrono::milliseconds(1);
	ws::stream stream(context.get_executor(), config);
	auto connection = std::make_shared<memory_connection>(context.get_executor());
	connection->stall_reads();
	libgs::error_code error;
	stream.adopt(std::static_pointer_cast<libgs::http::connection>(connection),
		{.stream_role = ws::role::server}, error);
	LIBGS_TEST_CHECK(not error);

	auto closing = stream.close(libgs::use_future);
	context.run();
	try
	{
		libgs::ignore_unused(closing.get());
		LIBGS_TEST_CHECK(false);
	}
	catch(const std::system_error &exception)
	{
		LIBGS_TEST_CHECK_EQ(exception.code(),
			asio::error::make_error_code(asio::error::timed_out));
	}
	const std::vector<ws::opcode> expected {ws::opcode::close};
	LIBGS_TEST_CHECK_EQ(parse_server_frames(connection->wire()), expected);
	LIBGS_TEST_CHECK_EQ(connection->cancel_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(connection->close_count(), size_t {1});
	LIBGS_TEST_CHECK_EQ(stream.state(), ws::connection_state::closed);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"adopt and lifecycle", test_adopt_and_lifecycle},
		{"adopt validation", test_adopt_validation},
		{"server write and fragmentation", test_server_write_and_fragmentation},
		{"client write masking", test_client_write_is_masked},
		{"write preflight and partial failure", test_preflight_and_partial_failure},
		{"async write", test_async_write},
		{"control write", test_control_write},
		{"write queue fairness and barrier", test_write_queue_fairness_and_barrier},
		{"write queue limits", test_write_queue_limits},
		{"queued write cancellation", test_queued_write_cancellation},
		{"write barrier cancellation", test_write_barrier_cancellation},
		{"detached write barrier error", test_detached_write_barrier_error},
		{"detached write owns payload", test_detached_write_owns_payload},
		{"control failure completes paused data",
			test_control_failure_completes_paused_data},
		{"shutdown aborts write queue", test_shutdown_aborts_write_queue},
		{"read fragmentation and automatic pong",
			test_read_fragmentation_and_automatic_pong},
		{"read pending multiple messages", test_read_pending_multiple_messages},
		{"async read", test_async_read},
		{"async read cancellation preserves parser",
			test_async_read_cancellation_preserves_parser},
		{"async read timeout", test_async_read_timeout},
		{"read limits and utf8", test_read_limits_and_utf8},
		{"protocol failure during async write",
			test_protocol_failure_during_async_write},
		{"protocol failure close deadline",
			test_protocol_failure_close_deadline},
		{"wait ctrl and retention", test_wait_ctrl_and_retention},
		{"wait ctrl manual pong", test_wait_ctrl_manual_pong},
		{"wait ctrl overlap and cancellation",
			test_wait_ctrl_overlap_and_cancellation},
		{"wait ctrl eof", test_wait_ctrl_eof},
		{"wait ctrl terminal transitions", test_wait_ctrl_terminal_transitions},
		{"peer close", test_peer_close},
		{"automatic pong during async write",
			test_automatic_pong_during_async_write},
		{"peer close during async write", test_peer_close_during_async_write},
		{"local close sync", test_local_close_sync},
		{"local close async join and drain", test_local_close_async_join_and_drain},
		{"local close takes over read", test_local_close_takes_over_read},
		{"local close immediate timeout", test_local_close_immediate_timeout},
		{"local close deadline cancels read",
			test_local_close_deadline_cancels_read},
	});
}
