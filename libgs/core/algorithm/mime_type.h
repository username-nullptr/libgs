
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

#ifndef LIBGS_CORE_ALGORITHM_MIME_TYPE_H
#define LIBGS_CORE_ALGORITHM_MIME_TYPE_H

#include <libgs/core/global.h>
#include <unordered_map>
#include <map>

namespace libgs
{

using suffix_type_map = std::unordered_map<std::string, std::string>;
using mime_head_map = std::map<std::string, std::string>;

LIBGS_CORE_API void set_suffix_map(suffix_type_map map);
LIBGS_CORE_API void insert_suffix_map(suffix_type_map map);

LIBGS_CORE_API void set_signatures_map(mime_head_map map);
LIBGS_CORE_API void insert_signatures_map(mime_head_map map);

LIBGS_CORE_API void set_signatures_map_offset4(mime_head_map map);
LIBGS_CORE_API void insert_signatures_map_offset4(mime_head_map map);

[[nodiscard]] LIBGS_CORE_API
std::string get_mime_type(std::string_view file_name, bool magic_first = false);

[[nodiscard]] LIBGS_CORE_API
bool is_text_file(std::string_view file_name);

[[nodiscard]] LIBGS_CORE_API
bool is_binary_file(std::string_view file_name);

[[nodiscard]] LIBGS_CORE_API
std::string get_text_file_encoding(std::string_view file_name);

} //namespace libgs


#endif //LIBGS_CORE_ALGORITHM_MIME_TYPE_H
