
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

#ifndef LIBGS_CORE_APPLICATION_H
#define LIBGS_CORE_APPLICATION_H

#include <libgs/core/global.h>
#include <optional>
#include <map>

namespace libgs::app
{

[[nodiscard]] LIBGS_CORE_API
std::string file_path();

[[nodiscard]] LIBGS_CORE_API
std::string dir_path();

/*[[nodiscard]]*/ LIBGS_CORE_API
bool set_current_directory(const std::string &path);

[[nodiscard]] LIBGS_CORE_API
std::string current_directory();

[[nodiscard]] LIBGS_CORE_API
std::string absolute_path(const std::string &path);

[[nodiscard]] LIBGS_CORE_API
bool is_absolute_path(const std::string &path);

[[nodiscard]] LIBGS_CORE_API
std::optional<std::string> getenv(const std::string &key);

[[nodiscard]] LIBGS_CORE_API
std::map<std::string, std::string> getenvs();

/*[[nodiscard]]*/ LIBGS_CORE_API
bool setenv(const std::string &key, const std::string &value, bool overwrite = true);

/*[[nodiscard]]*/ LIBGS_CORE_API
bool unsetenv(const std::string &key);

template <typename Arg0, typename...Args> /* [[nodiscard]] */
bool setenv(const std::string &key, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

template <typename Arg0, typename...Args> /* [[nodiscard]] */
bool setenv(const std::string &key, bool overwrite, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

template <concept_char_string_type T> /* [[nodiscard]] */
bool setenv(const std::string &key, T &&value, bool overwrite = true);

} //namespace libgs::app
#include <libgs/core/detail/app_utls.h>


#endif //LIBGS_CORE_APPLICATION_H
