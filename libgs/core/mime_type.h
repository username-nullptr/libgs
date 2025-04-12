
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_MIME_TYPE_H
#define LIBGS_CORE_MIME_TYPE_H

#include <libgs/core/global.h>
#include <unordered_map>
#include <map>

namespace libgs::mime_type
{

using path_t = std::filesystem::path;
using suffix_type_map = std::unordered_map<std::string, std::string>;
using mime_head_map = std::map<std::string, std::string>;

[[nodiscard]] LIBGS_CORE_API suffix_type_map &suffix_map();
[[nodiscard]] LIBGS_CORE_API mime_head_map &signatures_map();
[[nodiscard]] LIBGS_CORE_API mime_head_map &signatures_map_offset4();

[[nodiscard]] LIBGS_CORE_API
std::string get(const path_t &file_name, bool magic_first = false);

[[nodiscard]] LIBGS_CORE_API
bool is_text(const path_t &file_name);

[[nodiscard]] LIBGS_CORE_API
bool is_binary(const path_t &file_name);

[[nodiscard]] LIBGS_CORE_API
std::string text_encoding(const path_t &file_name);

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI std::string get(FS &stream)
	requires is_fstream_v<char,FS> or is_ifstream_v<char,FS>;

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI bool is_text(FS &stream)
	requires is_fstream_v<char,FS> or is_ifstream_v<char,FS>;

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI bool is_binary(FS &stream)
	requires is_fstream_v<char,FS> or is_ifstream_v<char,FS>;

template <typename FS>
[[nodiscard]] LIBGS_CORE_TAPI std::string text_encoding(FS &stream)
	requires is_fstream_v<char,FS> or is_ifstream_v<char,FS>;

} //namespace libgs::mime_type
#include <libgs/core/detail/mime_type.h>


#endif //LIBGS_CORE_MIME_TYPE_H
