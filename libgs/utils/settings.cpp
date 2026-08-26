
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025 Xiaoqiang <username_nullptr@163.com>                         *
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

#include "settings.h"
#include "logger.h"

using namespace std::chrono_literals;
using namespace libgs::operators;

namespace libgs::utils
{

settings::settings(std::string name) :
	m_impl(new impl(std::move(name)))
{
	m_impl->m_ini.set_sync_on_delete(true);
	m_impl->m_ini.set_sync_period(5s);
}

settings::~settings()
{
	delete m_impl;
}

namespace {
struct LIBGS_DECL_HIDDEN no_deleter {
	void operator()(settings*) const {}
};
} //namespace

using settings_ptr = std::unique_ptr<settings, no_deleter>;

static std::map<std::string, settings_ptr> g_instances;
static spin_shared_mutex g_instances_lock;

settings &settings::instance(std::string_view name, bool create)
{
	std::string _name(name.data(), name.size());
	spin_shared_unique_lock locker(g_instances_lock);

	if( auto it = g_instances.find(_name); it != g_instances.end() )
		return *it->second;

	else if( create )
	{
		settings_ptr object(new settings(_name), no_deleter());
		it = g_instances.emplace(std::move(_name), std::move(object)).first;
		return *it->second;
	}
	locker.unlock();

	runtime_error::loc_throw(std::format (
		"libgs::utils::settings::instance: Instance '{}' does not exist.", name
	));
	// return {};
}

settings &settings::instance()
{
	return instance("default");
}

static std::map<std::filesystem::path, const settings*> g_file_paths;
static spin_mutex g_file_paths_lock;

sys_expected<> settings::load(const path_t &file_path)
{
	if( not file_path.empty() )
	{
		spin_unique_lock locker(g_file_paths_lock);
		auto [it, inserted] = g_file_paths.emplace(file_path, this);

		if( not inserted and it->second != this )
		{
			runtime_error::loc_throw(std::format (
				"settings::set_file_name: File '{}' is already used by another instance.",
				file_path.string()
			));
		}
		g_file_paths.erase(it);
		g_file_paths.emplace(file_path, this);
		locker.unlock();
	}
	std::error_code error;
	m_impl->m_ini_lock.lock();
	m_impl->m_ini.load_or(file_path, error);
	auto _file_name = m_impl->m_ini.file_name();
	m_impl->m_ini_lock.unlock();

	if( error )
	{
		libgs_utils_clog_error("LibGS.Utils",
			"settings: load file '{}' failed: '{}'.",
			_file_name, error
		);
		return sys_unexpected(error);
	}
	loaded();
	return {};
}

sys_expected<> settings::sync()
{
	m_impl->m_ini_lock.lock_shared();
	libgs::ini ini(get_executor(), m_impl->m_ini.file_name());

	for(auto &[group, map] : m_impl->m_ini)
	{
		for(auto &[key, value] : map)
			ini[group][key] = value;
	}
	m_impl->m_ini_lock.unlock_shared();

	std::error_code error;
	static std::mutex mutex;

	mutex.lock();
	ini.sync(error);
	mutex.unlock();

	if( error )
	{
		libgs_utils_clog_error("LibGS.Utils",
			"settings: sync file '{}' failed: '{}'.",
			file_name(), error
		);
		return sys_unexpected(error);
	}
	return {};
}

std::vector<std::string> settings::names() noexcept
{
	std::vector<std::string> names;
	spin_shared_shared_lock locker(g_instances_lock);

	names.reserve(g_instances.size());
	for(auto &key : g_instances | std::views::keys)
		names.push_back(key);
	return names;
}

std::filesystem::path settings::file_name() const noexcept
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.file_name();
}

optional<value> settings::get(const group_key_t &gk)
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read(gk);
}

std::string_view settings::name() const noexcept
{
	return m_impl->m_name;
}

const settings::ini_t &settings::ini() const noexcept
{
	return m_impl->m_ini;
}

settings::ini_t &settings::ini() noexcept
{
	return m_impl->m_ini;
}

} //namespace libgs::utils
