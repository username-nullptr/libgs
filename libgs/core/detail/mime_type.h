
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_DETAIL_MIME_TYPE_H
#define LIBGS_CORE_DETAIL_MIME_TYPE_H

namespace libgs::mime_type { namespace detail
{

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI bool is_text(FS &stream)
	requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	char buf[0x4000] = {0};
	stream.read(buf, sizeof(buf));

	auto buf_size = static_cast<size_t>(stream.gcount());
	std::string data(buf, buf_size);

	constexpr auto FF = "\xFF\xFE";

	// UTF16 byte order marks
	if( constexpr auto FE = "\xFE\xFF";
		data.starts_with(FE) or data.starts_with(FF) )
		return true;

	// Check the first 128 bytes (see shared-mime spec)
	const char *p = buf;
	const char *e = p + ( 128 < buf_size ? 128 : buf_size );

	for(; p<e; p++)
	{
		if( static_cast<char>(*p) < 32 and *p != 9 and *p != 10 and *p != 13 )
			return false;
	}
	return true;
}

[[nodiscard]] LIBGS_CORE_API std::string search
(const mapping::mime_head_map &mimes, const char *buf, size_t size);

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI std::string from_magic(FS &stream)
	requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	if( not stream.is_open() )
		return "unknown";

	constexpr size_t buf_len = 0xFF;
	char buf[buf_len] = {0};
	stream.read(buf, buf_len);

	auto size = static_cast<size_t>(stream.gcount());
	if( size < buf_len )
	{
		stream.clear();
		stream.seekg(0, std::ios_base::beg);
		if( is_text(stream) )
			return "text/plain";
	}
	auto mime_type = search(mapping::signatures(), buf, size);
	if( mime_type.empty() and size > 4 )
		mime_type = search(mapping::signatures_offset4(), buf + 4, size - 4);
	return mime_type;
}

} //namespace detail

template <typename FS>
std::string get(FS &stream) requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	return detail::from_magic(stream);
}

template <typename FS>
bool is_text(FS &stream) requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	if( stream.is_open() )
		return detail::is_text(stream);
	return false;
}

template <typename FS>
bool is_binary(FS &stream) requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	return not is_text(stream);
}

template <typename FS>
std::string text_encoding(FS &stream) requires is_fstream_v<FS,char> or is_ifstream_v<FS,char>
{
	std::string result = "unknown";
	if( not stream.is_open() )
		return result;

	char first_byte = 0;
	stream.get(first_byte);

	char second_byte = 0;
	if( stream.eof() )
		second_byte = 'A';
	else
		stream.get(second_byte);

	char third_byte = 0;
	if( stream.eof() )
		third_byte = 'A';
	else
		stream.get(third_byte);

	char fourth_byte = 0;
	if( stream.eof() )
		fourth_byte = 'A';
	else
		stream.get(fourth_byte);

	if( static_cast<uint8_t>(first_byte) == 0xEF and
		static_cast<uint8_t>(second_byte) == 0xBB and
		static_cast<uint8_t>(third_byte) == 0xBF )
		result = "UTF-8";

	else if( static_cast<uint8_t>(first_byte) == 0xFF and
			 static_cast<uint8_t>(second_byte) == 0xFE)
		result = "UTF-16LE";

	else if( static_cast<uint8_t>(first_byte) == 0xFE and
			 static_cast<uint8_t>(second_byte) == 0xFF )
		result =  "UTF-16BE";

	else if( static_cast<uint8_t>(first_byte) == 0xFF and
		static_cast<uint8_t>(second_byte) == 0xFE and
		static_cast<uint8_t>(third_byte) == 0x0 and
		static_cast<uint8_t>(fourth_byte) == 0x0)
		result = "UTF-32LE";

	else if( static_cast<uint8_t>(first_byte) == 0x00 and
			 static_cast<uint8_t>(second_byte) == 0x00 and
			 static_cast<uint8_t>(third_byte) == 0xFE and
			 static_cast<uint8_t>(fourth_byte) == 0xFF)
		result = "UTF-32BE";

	return result;
}

} //namespace libgs::mime_type


#endif //LIBGS_CORE_DETAIL_MIME_TYPE_H
