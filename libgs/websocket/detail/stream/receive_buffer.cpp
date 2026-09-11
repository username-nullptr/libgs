// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/detail/stream/receive_buffer.h>
#include <libgs/websocket/detail/permessage_deflate.h>
#include <libgs/websocket/protocol/detail/utf8.h>
#include <libgs/websocket/protocol/generator.h>

namespace libgs::websocket::detail
{

void receive_buffer::reset(role local_role, const stream_config &config,
	std::span<const extension> extensions, std::vector<std::byte> pending_data)
{
	frame_codec_config codec_config {
		.local_role = local_role, .max_frame_size = config.max_frame_size
	};
	if( not extensions.empty() )
		codec_config.allowed_rsv = reserved_bit::rsv1;

	auto parser = std::make_unique<frame_parser>(codec_config);
	auto read_buffer = std::make_shared<std::vector<std::byte>>(config.read_buffer_size);

	m_parser = std::move(parser);
	m_read_buffer = std::move(read_buffer);
	m_pending_data = std::move(pending_data);

	m_max_message_size = config.max_message_size;
	m_permessage_deflate = not extensions.empty();
	m_message_compressed = false;

	m_pending_offset = 0;
	m_read_size = 0;
	m_read_offset = 0;

	m_message_type.reset();
	m_message_body.clear();
	m_frame_body.clear();
	m_control_body.clear();
}

mutable_buffer receive_buffer::available_data() noexcept
{
	if( m_pending_offset < m_pending_data.size() )
	{
		return {
			m_pending_data.data() + m_pending_offset,
			m_pending_data.size() - m_pending_offset
		};
	}
	if( m_read_offset < m_read_size )
	{
		return {
			m_read_buffer->data() + m_read_offset,
			m_read_size - m_read_offset
		};
	}
	return {};
}

std::shared_ptr<std::vector<std::byte>> receive_buffer::read_storage() const noexcept
{
	return m_read_buffer;
}

error_code receive_buffer::commit_read(size_t size) noexcept
{
	if( not m_read_buffer or size > m_read_buffer->size() )
		return make_error_code(std::errc::io_error);

	m_read_size = size;
	m_read_offset = 0;
	return {};
}

sys_expected<optional<received_event>> receive_buffer::consume() noexcept
{
	auto input = available_data();
	if( input.size() == 0 )
		return optional<received_event>{};

	if( not m_parser )
		return sys_unexpected(make_error_code(std::errc::io_error));

	auto parsed = m_parser->parse(input);
	if( not parsed )
		return sys_unexpected(parsed.error());

	if( parsed->consumed == 0 )
		return sys_unexpected(make_error_code(std::errc::io_error));

	const auto &header = m_parser->header();
	if( parsed->header_ready )
	{
		const bool compressed = header.rsv.test_flag(reserved_bit::rsv1);
		if( compressed and (not m_permessage_deflate or
			header.op == opcode::continuation or is_control_opcode(header.op)) )
			return sys_unexpected(make_error_code(protocol_errc::unexpected_rsv));

		if( header.op == opcode::text or header.op == opcode::binary )
		{
			m_message_type = header.op == opcode::text ?
				message_type::text : message_type::binary;

			m_message_body.clear();
			m_message_compressed = compressed;
		}
		if( is_data_opcode(header.op) or header.op == opcode::continuation )
			m_frame_body.clear();

		else if( is_control_opcode(header.op) )
			m_control_body.clear();

		if( is_data_opcode(header.op) or header.op == opcode::continuation )
		{
			if( header.payload_size > std::numeric_limits<size_t>::max() or
				static_cast<size_t>(header.payload_size) > m_message_body.max_size() - m_message_body.size() )
				return sys_unexpected(make_error_code(errc::message_too_big));

			size_t wire_limit = m_max_message_size;
			if( m_message_compressed and wire_limit != 0 )
			{
				const auto overhead = wire_limit / 8 + 1024;
				wire_limit = overhead > std::numeric_limits<size_t>::max() - wire_limit ?
					std::numeric_limits<size_t>::max() : wire_limit + overhead;
			}
			if( wire_limit != 0 and
				static_cast<size_t>(header.payload_size) >
					wire_limit - std::min(m_message_body.size(), wire_limit) )
				return sys_unexpected(make_error_code(errc::message_too_big));
		}
	}
	if( parsed->payload.size() != 0 )
	{
		if( header.mask )
			apply_mask(parsed->payload, *header.mask, parsed->payload_offset);
		try {
			const auto *begin = static_cast<const std::byte*>(parsed->payload.data());
			if( is_control_opcode(header.op) )
			{
				m_control_body.insert(m_control_body.end(), begin,
					begin + parsed->payload.size());
			}
			else
			{
				m_frame_body.insert(m_frame_body.end(), begin,
					begin + parsed->payload.size());

				m_message_body.insert(m_message_body.end(), begin,
					begin + parsed->payload.size());
			}
		}
		catch(const std::bad_alloc&) {
			return sys_unexpected(make_error_code(std::errc::not_enough_memory));
		}
		catch(...) {
			return sys_unexpected(make_error_code(std::errc::io_error));
		}
	}
	if( m_pending_offset < m_pending_data.size() )
	{
		m_pending_offset += parsed->consumed;
		if( m_pending_offset == m_pending_data.size() )
		{
			m_pending_data.clear();
			m_pending_offset = 0;
		}
	}
	else
	{
		m_read_offset += parsed->consumed;
		if( m_read_offset == m_read_size )
		{
			m_read_offset = 0;
			m_read_size = 0;
		}
	}
	if( not parsed->frame_finished )
		return optional<received_event>{};

	received_event event{.op = header.op};
	if( is_data_opcode(header.op) or header.op == opcode::continuation )
	{
		if( not m_message_type )
			return sys_unexpected(make_error_code(protocol_errc::unexpected_continuation));

		event.frame = data_frame {
			.type = *m_message_type,
			.body = std::move(m_frame_body),
			.continuation = header.op == opcode::continuation,
			.fin = header.fin,
		};
		m_frame_body.clear();
		if( header.fin )
		{
			if( m_message_compressed )
			{
				auto inflated = inflate_message(m_message_body, m_max_message_size);
				if( not inflated )
					return sys_unexpected(inflated.error());
				m_message_body = std::move(*inflated);
			}
			if( *m_message_type == message_type::text )
			{
				const auto text = m_message_body.empty() ? std::string_view{} :
					std::string_view(reinterpret_cast<const char*>(m_message_body.data()), m_message_body.size());

				if( not is_valid_utf8(text) )
					return sys_unexpected(make_error_code(protocol_errc::invalid_utf8));
			}
			event.data = message {
				.type = *m_message_type,
				.body = std::move(m_message_body),
			};
			m_message_type.reset();
			m_message_compressed = false;
			m_message_body.clear();
		}
	}
	else
		event.control = std::move(m_control_body);

	return optional(std::move(event));
}

} //namespace libgs::websocket::detail
