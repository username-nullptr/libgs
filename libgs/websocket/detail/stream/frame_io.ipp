// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP

namespace libgs::websocket::detail
{

template <core_concepts::exec Exec>
size_t stream_impl<Exec>::write
(message_type type, std::span<const const_buffer> buffers, error_code &error) noexcept
{
	error.clear();
	if( m_state == connection_state::idle )
	{
		error = make_error_code(errc::not_open);
		return 0;
	}
	if( m_state == connection_state::closing )
	{
		error = make_error_code(errc::closing);
		return 0;
	}
	if( m_state == connection_state::closed )
	{
		error = make_error_code(errc::closed);
		return 0;
	}
	if( m_state == connection_state::failed )
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return 0;
	}
	if( type != message_type::text and type != message_type::binary )
	{
		error = make_error_code(std::errc::invalid_argument);
		return 0;
	}
	if( send_engine_busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}
	auto prepared = prepare_frames(type, buffers);
	if( not prepared )
	{
		error = prepared.error();
		return 0;
	}
	size_t body_transferred = 0;
	for(const auto &frame : *prepared)
	{
		body_transferred += write_prepared(frame, error);
		if( error )
		{
			fail(error);
			return body_transferred;
		}
	}
	error.clear();
	return body_transferred;
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::prepare_control_frame
(opcode op, const const_buffer &payload, bool borrow_payload) const noexcept -> sys_expected<prepared_frame>
{
	try {
		if( op != opcode::ping and op != opcode::pong and op != opcode::close )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		if( payload.size() > 125 )
			return sys_unexpected(make_error_code(protocol_errc::control_payload_too_large));

		if( payload.size() != 0 and payload.data() == nullptr )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		frame_header header{.op = op, .payload_size = payload.size()};
		if( m_role == role::client )
		{
			masking_key key;
			auto random = secure_random_bytes(mutable_buffer(key.bytes.data(), key.bytes.size()));

			if( not random )
				return sys_unexpected(random.error());
			header.mask = key;
		}
		auto encoded = encode_frame_header(header, frame_codec_config {
			.local_role = m_role, .max_frame_size = m_config.max_frame_size
		});
		if( not encoded )
			return sys_unexpected(encoded.error());

		const bool borrowed = borrow_payload and
			not header.mask and payload.size() != 0;

		auto wire = std::make_shared<std::vector<std::byte>>(
			encoded->size + (borrowed ? 0 : payload.size())
		);
		std::memcpy(wire->data(), encoded->buffer().data(), encoded->size);
		if( borrowed )
		{
			prepared_frame result;
			result.wire = std::move(wire);

			result.buffers = {
				const_buffer(result.wire->data(), result.wire->size()),
				payload
			};
			result.header_size = encoded->size;
			result.payload_size = payload.size();
			return result;
		}
		if( header.mask )
		{
			auto copied = mask_copy (
				mutable_buffer(wire->data() + encoded->size, payload.size()),
				payload, *header.mask
			);
			if( not copied )
				return sys_unexpected(copied.error());
		}
		else if( payload.size() != 0 )
			std::memcpy(wire->data() + encoded->size, payload.data(), payload.size());

		prepared_frame result;
		result.wire = std::move(wire);
		result.buffers = {
			const_buffer(result.wire->data(), result.wire->size())
		};
		result.header_size = encoded->size;
		result.payload_size = payload.size();
		return result;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::prepare_frames(message_type type, std::span<const const_buffer> buffers)
	const noexcept -> sys_expected<std::vector<prepared_frame>>
{
	try {
		if( type != message_type::text and type != message_type::binary )
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		size_t body_size = 0;
		utf8_validator utf8;

		for(const auto &buffer : buffers)
		{
			if( buffer.size() > std::numeric_limits<size_t>::max() - body_size )
				return sys_unexpected(make_error_code(std::errc::value_too_large));

			if( buffer.size() == 0 )
				continue;

			const auto *data = static_cast<const std::byte *>(buffer.data());
			if( data == nullptr )
				return sys_unexpected(make_error_code(std::errc::invalid_argument));

			body_size += buffer.size();
			if( type == message_type::text and
				not utf8.consume(std::string_view(reinterpret_cast<const char*>(data), buffer.size())) )
				return sys_unexpected(make_error_code(protocol_errc::invalid_utf8));
		}
		if( m_config.max_message_size != 0 and body_size > m_config.max_message_size )
			return sys_unexpected(make_error_code(errc::message_too_big));

		if( type == message_type::text and not utf8.complete() )
			return sys_unexpected(make_error_code(protocol_errc::invalid_utf8));

		std::vector<prepared_frame> frames;
		const auto fragment_size = m_config.write_fragment_size;

		const auto frame_count = body_size == 0 or
			fragment_size == 0 ? 1 : 1 + (body_size - 1) / fragment_size;

		if( frame_count > frames.max_size() )
			return sys_unexpected(make_error_code(std::errc::value_too_large));
		frames.reserve(frame_count);

		size_t offset = 0;
		size_t buffer_index = 0;
		size_t buffer_offset = 0;
		bool first = true;
		do {
			const auto remaining = body_size - offset;
			const auto payload_size = fragment_size == 0 ?
				remaining : std::min(remaining, fragment_size);

			frame_header header {
				.fin = payload_size == remaining,
				.op = first ?
					(type == message_type::text ? opcode::text : opcode::binary) :
					opcode::continuation,
				.payload_size = payload_size,
			};
			if( m_role == role::client )
			{
				masking_key key;
				auto random = secure_random_bytes(mutable_buffer(key.bytes.data(), key.bytes.size()));
				if( not random )
					return sys_unexpected(random.error());
				header.mask = key;
			}
			auto encoded = encode_frame_header(header, frame_codec_config {
				.local_role = m_role, .max_frame_size = m_config.max_frame_size
			});
			if( not encoded )
				return sys_unexpected(encoded.error());

			prepared_frame prepared;
			prepared.header_size = encoded->size;
			prepared.payload_size = payload_size;

			prepared.wire = std::make_shared<std::vector<std::byte>>(
				encoded->size + (header.mask ? payload_size : 0)
			);
			std::memcpy(prepared.wire->data(), encoded->buffer().data(), encoded->size);

			const auto payload_buffer_count = std::min(buffers.size(), payload_size);
			prepared.buffers.reserve(header.mask ? 1 : payload_buffer_count + 1);

			prepared.buffers.emplace_back(prepared.wire->data(),
				header.mask ? prepared.wire->size() : encoded->size
			);
			size_t frame_offset = 0;
			while(frame_offset < payload_size)
			{
				while(buffer_index < buffers.size() and buffer_offset == buffers[buffer_index].size())
				{
					++buffer_index;
					buffer_offset = 0;
				}
				if( buffer_index == buffers.size() )
					return sys_unexpected(make_error_code(std::errc::io_error));

				const auto &source = buffers[buffer_index];
				const auto available = source.size() - buffer_offset;

				const auto size = std::min(payload_size - frame_offset, available);
				const auto *data = static_cast<const std::byte*>(source.data()) + buffer_offset;

				if( header.mask )
				{
					auto copied = mask_copy (
						mutable_buffer(prepared.wire->data() + encoded->size + frame_offset, size),
						const_buffer(data, size), *header.mask, frame_offset
					);
					if( not copied )
						return sys_unexpected(copied.error());
				}
				else
					prepared.buffers.emplace_back(data, size);

				frame_offset += size;
				buffer_offset += size;
			}
			frames.emplace_back(std::move(prepared));
			offset += payload_size;
			first = false;
		}
		while( offset < body_size );
		return frames;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {}
	return sys_unexpected(make_error_code(std::errc::io_error));
}

template <core_concepts::exec Exec>
size_t stream_impl<Exec>::write_prepared(const prepared_frame &frame, error_code &error) noexcept
{
	if( send_engine_busy() )
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}
	const auto wire_size = m_connection->write (
		std::span<const const_buffer>(frame.buffers), error
	);
	const auto payload_size = wire_size > frame.header_size ?
		std::min(frame.payload_size, wire_size - frame.header_size) : 0;

	if( not error and wire_size != frame.header_size + frame.payload_size )
		error = make_error_code(std::errc::io_error);
	return payload_size;
}

} //namespace libgs::websocket::detail


#endif //LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP
