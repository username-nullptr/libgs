/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
*                                                                                   *
*   This file is part of LIBGS                                                      *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining a copy    *
*   of this software and associated documentation files (the "Software"), to deal   *
*   in the Software without restriction, including without limitation the rights    *
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
*   copies of the Software, and to permit persons to whom the Software is           *
*   furnished to do so, subject to the following conditions:                        *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_COMPRESSION_H
#define LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_COMPRESSION_H

#include <libgs/http_nt/global.h>

namespace libgs::http_nt
{

constexpr size_t default_max_decoded_body_size_v = 64 * 1024 * 1024;

[[nodiscard]] LIBGS_HTTP_NT_API double content_coding_quality (
	std::string_view value, std::string_view coding
) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API bool
is_precompressed_mime_type(std::string_view value) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API bool
is_compressible_mime_type(std::string_view value) noexcept;

class LIBGS_HTTP_NT_API gzip_encoder final
{
	LIBGS_DISABLE_COPY(gzip_encoder)

public:
	explicit gzip_encoder(int level = -1) noexcept;
	~gzip_encoder();

	gzip_encoder(gzip_encoder &&other) noexcept;
	gzip_encoder &operator=(gzip_encoder &&other) noexcept;

	[[nodiscard]] sys_expected<std::string>
	append(std::string_view data, bool finish = false) noexcept;

	[[nodiscard]] bool finished() const noexcept;

private:
	class impl;
	impl *m_impl;
};

class LIBGS_HTTP_NT_API gzip_decoder final
{
	LIBGS_DISABLE_COPY(gzip_decoder)

public:
	explicit gzip_decoder (
		size_t max_output_size = default_max_decoded_body_size_v
	) noexcept;

	~gzip_decoder();

	gzip_decoder(gzip_decoder &&other) noexcept;
	gzip_decoder &operator=(gzip_decoder &&other) noexcept;

	[[nodiscard]] sys_expected<std::string>
	append(std::string_view data, bool finish = false) noexcept;

	[[nodiscard]] bool finished() const noexcept;
	[[nodiscard]] size_t output_size() const noexcept;

private:
	class impl;
	impl *m_impl;
};

[[nodiscard]] LIBGS_HTTP_NT_API sys_expected<std::string>
gzip_compress(std::string_view data, int level = -1) noexcept;

[[nodiscard]] LIBGS_HTTP_NT_API sys_expected<std::string>
gzip_decompress (std::string_view data, size_t max_output_size = default_max_decoded_body_size_v) noexcept;

} //namespace libgs::http_nt

#endif //LIBGS_HTTP_NT_PROTOCOL_UTILS_CORE_COMPRESSION_H
