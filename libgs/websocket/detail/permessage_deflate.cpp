// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/detail/permessage_deflate.h>

#if LIBGS_WEBSOCKET_ZLIB_SUPPORT
# include <zlib.h>
#endif

namespace libgs::websocket::detail
{

bool supported_extension_set(std::span<const extension> extensions) noexcept
{
	return extensions.empty() or (
		extensions.size() == 1 and
		is_permessage_deflate_extension(extensions.front()) and
		permessage_deflate_available_v
	);
}

sys_expected<std::vector<std::byte>>
deflate_message(std::span<const const_buffer> buffers) noexcept
{
#if !LIBGS_WEBSOCKET_ZLIB_SUPPORT
	ignore_unused(buffers);
	return sys_unexpected(make_error_code(errc::unsupported_extension));

#else //LIBGS_WEBSOCKET_ZLIB_SUPPORT
	try {
		z_stream stream {};
		if( deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			-15, 8, Z_DEFAULT_STRATEGY) != Z_OK )
			return sys_unexpected(make_error_code(std::errc::io_error));

		struct guard_t
		{
			z_stream &stream;
			~guard_t() {
				deflateEnd(&stream);
			}
		}
		guard {stream};

		std::vector<std::byte> result;
		std::array<std::byte, 16 * 1024> output {};

		auto pump = [&](int flush) -> bool
		{
			do {
				stream.next_out = reinterpret_cast<Bytef*>(output.data());
				stream.avail_out = static_cast<uInt>(output.size());
				const auto code = deflate(&stream, flush);

				if( code != Z_OK )
					return false;

				const auto produced = output.size() - stream.avail_out;
				result.insert(result.end(), output.begin(), output.begin() + produced);
			}
			while( stream.avail_in != 0 or stream.avail_out == 0 );
			return true;
		};

		for(const auto &buffer : buffers)
		{
			if( buffer.size() != 0 and buffer.data() == nullptr )
				return sys_unexpected(make_error_code(std::errc::invalid_argument));

			auto *input = static_cast<const std::byte*>(buffer.data());
			auto remaining = buffer.size();

			while( remaining != 0 )
			{
				const auto size = static_cast<uInt>(std::min<size_t>(
					remaining, std::numeric_limits<uInt>::max())
				);
				stream.next_in = reinterpret_cast<Bytef*>(const_cast<std::byte*>(input));
				stream.avail_in = size;

				if( not pump(Z_NO_FLUSH) )
					return sys_unexpected(make_error_code(std::errc::io_error));

				input += size;
				remaining -= size;
			}
		}
		stream.next_in = nullptr;
		stream.avail_in = 0;

		if( not pump(Z_SYNC_FLUSH) )
			return sys_unexpected(make_error_code(std::errc::io_error));

		constexpr std::array trailer {
			std::byte {0x00}, std::byte {0x00},
			std::byte {0xFF}, std::byte {0xFF}
		};
		if( result.size() < trailer.size() or
			not std::equal(trailer.begin(), trailer.end(), result.end() - trailer.size()) )
			return sys_unexpected(make_error_code(std::errc::io_error));

		result.resize(result.size() - trailer.size());
		return result;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
#endif //LIBGS_WEBSOCKET_ZLIB_SUPPORT
}

sys_expected<std::vector<std::byte>> inflate_message
(std::span<const std::byte> payload, size_t max_message_size) noexcept
{
#if !LIBGS_WEBSOCKET_ZLIB_SUPPORT
	ignore_unused(payload, max_message_size);
	return sys_unexpected(make_error_code(errc::unsupported_extension));

#else //LIBGS_WEBSOCKET_ZLIB_SUPPORT
	try {
		if( payload.empty() )
			return sys_unexpected(make_error_code(
				protocol_errc::invalid_compressed_payload));

		z_stream stream {};
		if( inflateInit2(&stream, -15) != Z_OK )
			return sys_unexpected(make_error_code(std::errc::io_error));

		struct guard_t
		{
			z_stream &stream;
			~guard_t() {
				inflateEnd(&stream);
			}
		}
		guard {stream};

		std::vector input(payload.begin(), payload.end());
		input.insert(input.end(), {
			std::byte {0x00}, std::byte {0x00},
			std::byte {0xFF}, std::byte {0xFF}
		});
		std::vector<std::byte> result;
		std::array<std::byte, 16 * 1024> output {};
		size_t offset = 0;
		bool drained_full_output = false;

		while( offset < input.size() )
		{
			const auto input_size = static_cast<uInt>(std::min<size_t>(
				input.size() - offset, std::numeric_limits<uInt>::max())
			);
			stream.next_in = reinterpret_cast<Bytef*>(input.data() + offset);
			stream.avail_in = input_size;
			do {
				stream.next_out = reinterpret_cast<Bytef*>(output.data());
				stream.avail_out = static_cast<uInt>(output.size());

				const auto code = inflate(&stream, Z_SYNC_FLUSH);
				const auto produced = output.size() - stream.avail_out;
				if( code == Z_BUF_ERROR )
				{
					if( not drained_full_output or produced != 0 or stream.avail_in != 0 )
					{
						return sys_unexpected(make_error_code (
							protocol_errc::invalid_compressed_payload)
						);
					}
					break;
				}
				if( code != Z_OK )
				{
					return sys_unexpected(make_error_code (
						protocol_errc::invalid_compressed_payload)
					);
				}
				if( max_message_size != 0 and
					produced > max_message_size - std::min(result.size(), max_message_size) )
					return sys_unexpected(make_error_code(errc::message_too_big));

				result.insert(result.end(), output.begin(), output.begin() + produced);
				drained_full_output = stream.avail_in == 0 and stream.avail_out == 0;
			}
			while( stream.avail_in != 0 or stream.avail_out == 0 );
			offset += input_size;
		}
		return result;
	}
	catch(const std::bad_alloc&) {
		return sys_unexpected(make_error_code(std::errc::not_enough_memory));
	}
	catch(...) {
		return sys_unexpected(make_error_code(std::errc::io_error));
	}
#endif //LIBGS_WEBSOCKET_ZLIB_SUPPORT
}

} //namespace libgs::websocket::detail
