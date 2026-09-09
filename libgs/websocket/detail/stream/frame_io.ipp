// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP
#define LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP

// Synchronous writes and outgoing frame preparation.

template <core_concepts::exec Exec>
size_t stream_impl<Exec>::write(message_type type,
	std::span<const const_buffer> buffers, error_code &error) noexcept
{
	error.clear();
	if(m_state == connection_state::idle)
	{
		error = make_error_code(errc::not_open);
		return 0;
	}
	if(m_state == connection_state::closing)
	{
		error = make_error_code(errc::closing);
		return 0;
	}
	if(m_state == connection_state::closed)
	{
		error = make_error_code(errc::closed);
		return 0;
	}
	if(m_state == connection_state::failed)
	{
		error = m_error ? m_error : make_error_code(std::errc::io_error);
		return 0;
	}
	if(type != message_type::text and type != message_type::binary)
	{
		error = make_error_code(std::errc::invalid_argument);
		return 0;
	}
	if(send_engine_busy())
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}

	std::vector<std::byte> body;
	try
	{
		size_t body_size = 0;
		for(const auto &buffer : buffers)
		{
			if(buffer.size() > body.max_size() - body_size)
			{
				error = make_error_code(std::errc::value_too_large);
				return 0;
			}
			body_size += buffer.size();
		}
		body.reserve(body_size);
		for(const auto &buffer : buffers)
		{
			if(buffer.size() == 0)
				continue;
			const auto *data = static_cast<const std::byte *>(buffer.data());
			if(data == nullptr)
			{
				error = make_error_code(std::errc::invalid_argument);
				return 0;
			}
			body.insert(body.end(), data, data + buffer.size());
		}
	}
	catch(const std::bad_alloc &)
	{
		error = make_error_code(std::errc::not_enough_memory);
		return 0;
	}
	catch(...)
	{
		error = make_error_code(std::errc::io_error);
		return 0;
	}
	if(m_config.max_message_size != 0 and
		body.size() > m_config.max_message_size)
	{
		error = make_error_code(errc::message_too_big);
		return 0;
	}

	if(type == message_type::text)
	{
		const auto text = body.empty() ? std::string_view{} : std::string_view(reinterpret_cast<const char *>(body.data()), body.size());
		if(not detail::is_valid_utf8(text))
		{
			error = make_error_code(protocol_errc::invalid_utf8);
			return 0;
		}
	}

	const auto first_opcode = type == message_type::text ? opcode::text : opcode::binary;
	const auto fragment_size = m_config.write_fragment_size;
	size_t body_offset = 0;
	size_t body_transferred = 0;
	bool first_frame = true;

	do
	{
		const auto remaining = body.size() - body_offset;
		const auto payload_size = fragment_size == 0 ? remaining : std::min(remaining, fragment_size);
		const bool final_frame = payload_size == remaining;

		frame_header frame{
			.fin = final_frame,
			.op = first_frame ? first_opcode : opcode::continuation,
			.payload_size = payload_size,
		};
		if(m_role == role::client)
		{
			masking_key key;
			auto random = detail::secure_random_bytes(mutable_buffer(
				key.bytes.data(), key.bytes.size()));
			if(not random)
			{
				error = random.error();
				if(body_offset != 0)
					fail(error);
				return body_transferred;
			}
			frame.mask = key;
		}

		auto encoded = encode_frame_header(frame, frame_codec_config{
													  .local_role = m_role,
													  .max_frame_size = m_config.max_frame_size,
												  });
		if(not encoded)
		{
			error = encoded.error();
			if(body_offset != 0)
				fail(error);
			return body_transferred;
		}

		const auto *payload_data = body.empty() ? nullptr : body.data() + body_offset;
		std::vector<std::byte> masked_payload;
		if(frame.mask)
		{
			try
			{
				masked_payload.resize(payload_size);
			}
			catch(const std::bad_alloc &)
			{
				error = make_error_code(std::errc::not_enough_memory);
				if(body_offset != 0)
					fail(error);
				return body_transferred;
			}
			if(auto copied = mask_copy(mutable_buffer(
										   masked_payload.data(), masked_payload.size()),
				   const_buffer(payload_data, payload_size), *frame.mask);
				not copied)
			{
				error = copied.error();
				if(body_offset != 0)
					fail(error);
				return body_transferred;
			}
			payload_data = masked_payload.data();
		}

		const std::array<const_buffer, 2> wire{
			encoded->buffer(), const_buffer(payload_data, payload_size)};
		error_code write_error;
		const auto wire_transferred = m_connection->write(wire, write_error);
		const auto header_size = static_cast<size_t>(encoded->size);
		const auto payload_transferred = wire_transferred > header_size ? std::min(payload_size, wire_transferred - header_size) : 0;
		body_transferred += payload_transferred;

		if(write_error or wire_transferred != header_size + payload_size)
		{
			error = write_error ? write_error : make_error_code(std::errc::io_error);
			fail(error);
			return body_transferred;
		}

		body_offset += payload_size;
		first_frame = false;
	} while(body_offset < body.size());

	error.clear();
	return body_transferred;
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::prepare_control_frame(
	opcode op, const const_buffer &payload) const noexcept
	-> sys_expected<prepared_frame>
{
	try
	{
		if(op != opcode::ping and op != opcode::pong and op != opcode::close)
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		if(payload.size() > 125)
			return sys_unexpected(make_error_code(
				protocol_errc::control_payload_too_large));
		if(payload.size() != 0 and payload.data() == nullptr)
			return sys_unexpected(make_error_code(std::errc::invalid_argument));

		frame_header header{.op = op, .payload_size = payload.size()};
		if(m_role == role::client)
		{
			masking_key key;
			auto random = detail::secure_random_bytes(mutable_buffer(
				key.bytes.data(), key.bytes.size()));
			if(not random)
				return sys_unexpected(random.error());
			header.mask = key;
		}

		auto encoded = encode_frame_header(header, frame_codec_config{.local_role = m_role, .max_frame_size = m_config.max_frame_size});
		if(not encoded)
			return sys_unexpected(encoded.error());

		auto wire = std::make_shared<std::vector<std::byte>>(
			encoded->size + payload.size());
		std::memcpy(wire->data(), encoded->buffer().data(), encoded->size);
		if(header.mask)
		{
			auto copied = mask_copy(mutable_buffer(
										wire->data() + encoded->size, payload.size()),
				payload, *header.mask);
			if(not copied)
				return sys_unexpected(copied.error());
		}
		else if(payload.size() != 0)
			std::memcpy(wire->data() + encoded->size,
				payload.data(), payload.size());

		return prepared_frame{
			.wire = std::move(wire),
			.header_size = encoded->size,
			.payload_size = payload.size(),
		};
	}
	catch(const std::bad_alloc &)
	{
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...)
	{
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
}

template <core_concepts::exec Exec>
auto stream_impl<Exec>::prepare_frames(message_type type,
	std::span<const const_buffer> buffers) const noexcept
	-> sys_expected<std::vector<prepared_frame>>
{
	try
	{
		if(type != message_type::text and type != message_type::binary)
			return sys_unexpected(make_error_code(std::errc::invalid_argument));
		std::vector<std::byte> body;
		size_t body_size = 0;
		for(const auto &buffer : buffers)
		{
			if(buffer.size() > body.max_size() - body_size)
				return sys_unexpected(make_error_code(std::errc::value_too_large));
			body_size += buffer.size();
		}
		body.reserve(body_size);
		for(const auto &buffer : buffers)
		{
			if(buffer.size() == 0)
				continue;
			const auto *data = static_cast<const std::byte *>(buffer.data());
			if(data == nullptr)
				return sys_unexpected(make_error_code(std::errc::invalid_argument));
			body.insert(body.end(), data, data + buffer.size());
		}
		if(m_config.max_message_size != 0 and
			body.size() > m_config.max_message_size)
			return sys_unexpected(make_error_code(errc::message_too_big));
		if(type == message_type::text and not detail::is_valid_utf8(body.empty() ? std::string_view{} : std::string_view(reinterpret_cast<const char *>(body.data()), body.size())))
			return sys_unexpected(make_error_code(protocol_errc::invalid_utf8));

		std::vector<prepared_frame> frames;
		size_t offset = 0;
		bool first = true;
		do
		{
			const auto remaining = body.size() - offset;
			const auto payload_size = m_config.write_fragment_size == 0 ? remaining : std::min(remaining, m_config.write_fragment_size);
			frame_header header{
				.fin = payload_size == remaining,
				.op = first ? (type == message_type::text ? opcode::text : opcode::binary) : opcode::continuation,
				.payload_size = payload_size,
			};
			if(m_role == role::client)
			{
				masking_key key;
				auto random = detail::secure_random_bytes(mutable_buffer(
					key.bytes.data(), key.bytes.size()));
				if(not random)
					return sys_unexpected(random.error());
				header.mask = key;
			}
			auto encoded = encode_frame_header(header, frame_codec_config{.local_role = m_role, .max_frame_size = m_config.max_frame_size});
			if(not encoded)
				return sys_unexpected(encoded.error());
			auto wire = std::make_shared<std::vector<std::byte>>(
				encoded->size + payload_size);
			std::memcpy(wire->data(), encoded->buffer().data(), encoded->size);
			const auto *payload = body.empty() ? nullptr : body.data() + offset;
			if(header.mask)
			{
				auto copied = mask_copy(mutable_buffer(
											wire->data() + encoded->size, payload_size),
					const_buffer(payload, payload_size), *header.mask);
				if(not copied)
					return sys_unexpected(copied.error());
			}
			else if(payload_size != 0)
				std::memcpy(wire->data() + encoded->size, payload, payload_size);
			frames.push_back({std::move(wire), encoded->size, payload_size});
			offset += payload_size;
			first = false;
		} while(offset < body.size());
		return frames;
	}
	catch(const std::bad_alloc &)
	{
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...)
	{
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
}

template <core_concepts::exec Exec>
size_t stream_impl<Exec>::write_prepared(const prepared_frame &frame,
	error_code &error) noexcept
{
	if(send_engine_busy())
	{
		error = make_error_code(std::errc::operation_in_progress);
		return 0;
	}
	const std::array<const_buffer, 1> wire{
		const_buffer(frame.wire->data(), frame.wire->size())};
	const auto wire_size = m_connection->write(wire, error);
	const auto payload_size = wire_size > frame.header_size ? std::min(frame.payload_size, wire_size - frame.header_size) : 0;
	if(not error and wire_size != frame.wire->size())
		error = make_error_code(std::errc::io_error);
	return payload_size;
}

#endif // LIBGS_WEBSOCKET_DETAIL_STREAM_FRAME_IO_IPP
