// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_DETAIL_SETTINGS_H
#define LIBGS_UTILS_DETAIL_SETTINGS_H

#include <libgs/core/shared_mutex.h>

namespace libgs::utils
{

class LIBGS_UTILS_API settings::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(std::string name) :
		m_name(std::move(name)) {}

	ini_t m_ini;
	mutable spin_shared_mutex m_ini_lock;
	std::string m_name;
};

optional<value> settings::get(concepts::string_p<char> auto &&path)
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read(std::forward<decltype(path)>(path));
}

settings &settings::set(const group_key_t &gk, const concepts::value_set<char> auto &value) noexcept
{
	m_impl->m_ini_lock.lock();
	m_impl->m_ini.write(gk, value);
	m_impl->m_ini_lock.unlock();

	changed(gk.group + "/" + gk.key, value);
	return *this;
}

settings &settings::set
(const concepts::string_p<char> auto &path, const concepts::value_set<char> auto &value) noexcept
{
	m_impl->m_ini_lock.lock();
	m_impl->m_ini.write(path, value);
	m_impl->m_ini_lock.unlock();

	changed(path, value);
	return *this;
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_SETTINGS_H
