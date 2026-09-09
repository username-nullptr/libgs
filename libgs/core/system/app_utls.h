// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
