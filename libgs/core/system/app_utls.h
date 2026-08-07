
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_CORE_SYSTEM_APP_UTILS_H
#define LIBGS_CORE_SYSTEM_APP_UTILS_H

#include <libgs/core/value.h>
#include <map>

namespace libgs::app
{

using path_t = std::filesystem::path;

[[nodiscard]] LIBGS_CORE_API
sys_expected<path_t> file_path() noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<path_t> dir_path() noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
sys_expected<> set_current_directory(const path_t &path) noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<path_t> current_directory() noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<path_t> absolute_path(const path_t &path) noexcept;

[[nodiscard]] LIBGS_CORE_API
bool is_absolute_path(const path_t &path) noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<std::string> getenv(std::string_view key) noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<std::map<std::string,std::string>> getenvs() noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
sys_expected<> setenv(std::string_view key, const libgs::value &value, bool overwrite = true) noexcept;

/*[[nodiscard]]*/ LIBGS_CORE_API
sys_expected<> unsetenv(std::string_view key) noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<std::string> current_user() noexcept;

[[nodiscard]] LIBGS_CORE_API
sys_expected<path_t> home_directory() noexcept;

} //namespace libgs::app
#include <libgs/core/system/detail/app_utls.h>


#endif //LIBGS_CORE_SYSTEM_APP_UTILS_H
