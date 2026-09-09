// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_SETTINGS_H
#define LIBGS_UTILS_SETTINGS_H

#include <libgs/utils/signal_slot.h>
#include <libgs/core/ini.h>

namespace libgs::utils
{

class LIBGS_UTILS_API settings
{
	LIBGS_DISABLE_COPY_MOVE(settings)
	explicit settings(std::string name);
	~settings();

public:
	using ini_t = libgs::ini;
	using path_t = ini_t::path_t;
	using group_key_t = ini_t::group_key;

	[[nodiscard]] static settings &instance(std::string_view name, bool create = true);
	[[nodiscard]] static settings &instance();

	sys_expected<> load(const path_t &file_path = {});
	sys_expected<> sync();

	[[nodiscard]] static std::vector<std::string> names() noexcept;
	[[nodiscard]] path_t file_name() const noexcept;

public:
	[[nodiscard]] optional<value> get(const group_key_t &gk);
	[[nodiscard]] optional<value> get(concepts::string_p<char> auto &&path);

public:
	settings &set (
		const group_key_t &gk,
		const concepts::value_set<char> auto &value
	) noexcept;

	settings &set (
		const concepts::string_p<char> auto &path,
		const concepts::value_set<char> auto &value
	) noexcept;

public:
	signal<void(std::string_view,value)> changed;
	signal<void()> loaded;

public:
	[[nodiscard]] std::string_view name() const noexcept;
	[[nodiscard]] const ini_t &ini() const noexcept;
	[[nodiscard]] ini_t &ini() noexcept;

private:
	class impl;
	impl *m_impl = nullptr;
};

} //namespace libgs::utils
#include <libgs/utils/detail/settings.h>


#endif //LIBGS_UTILS_SETTINGS_H
