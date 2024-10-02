
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

#ifndef LIBGS_CORE_APP_UTILS_H
#define LIBGS_CORE_APP_UTILS_H

#include <libgs/core/global.h>
#include <optional>
#include <map>

namespace libgs::app
{

[[nodiscard]] LIBGS_CORE_API
std::string file_path();

[[nodiscard]] LIBGS_CORE_API
std::string file_path(error_code &error) noexcept;

[[nodiscard]] LIBGS_CORE_API
std::string dir_path();

[[nodiscard]] LIBGS_CORE_API
std::string dir_path(error_code &error) noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
bool set_current_directory(std::string_view path);

/*[[nodiscard]]*/ LIBGS_CORE_API
bool set_current_directory(error_code &error, std::string_view path) noexcept;

[[nodiscard]] LIBGS_CORE_API
std::string current_directory();

[[nodiscard]] LIBGS_CORE_API
std::string current_directory(error_code &error) noexcept;

[[nodiscard]] LIBGS_CORE_API
std::string absolute_path(std::string_view path);

[[nodiscard]] LIBGS_CORE_API
std::string absolute_path(error_code &error, std::string_view path) noexcept;

[[nodiscard]] LIBGS_CORE_API
bool is_absolute_path(std::string_view path) noexcept;

[[nodiscard]] LIBGS_CORE_API
std::optional<std::string> getenv(std::string_view key);

[[nodiscard]] LIBGS_CORE_API
std::optional<std::string> getenv(error_code &error, std::string_view key) noexcept;

[[nodiscard]] LIBGS_CORE_API
std::map<std::string, std::string> getenvs();

[[nodiscard]] LIBGS_CORE_API
std::map<std::string, std::string> getenvs(error_code &error) noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
bool setenv(std::string_view key, std::string_view value, bool overwrite = true);

/*[[nodiscard]]*/ LIBGS_CORE_API
bool setenv(error_code &error, std::string_view key, std::string_view value, bool overwrite = true) noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
bool unsetenv(std::string_view key);

/*[[nodiscard]]*/ LIBGS_CORE_API
bool unsetenv(error_code &error, std::string_view key) noexcept;

template <typename Arg0, typename...Args> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(std::string_view key, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

template <typename Arg0, typename...Args> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(error_code &error, std::string_view key, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args) noexcept;

template <typename Arg0, typename...Args> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(std::string_view key, bool overwrite, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args);

template <typename Arg0, typename...Args> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(error_code &error, std::string_view key, bool overwrite, std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args) noexcept;

template <concepts::char_string_type T> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(std::string_view key, T &&value, bool overwrite = true);

template <concepts::char_string_type T> /* [[nodiscard]] */ LIBGS_CORE_TAPI
bool setenv(error_code &error, std::string_view key, T &&value, bool overwrite = true) noexcept;

} //namespace libgs::app
#include <libgs/core/detail/app_utls.h>


#endif //LIBGS_CORE_APP_UTILS_H
