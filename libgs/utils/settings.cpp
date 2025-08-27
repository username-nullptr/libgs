
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

#include "settings.h"

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

struct LIBGS_DECL_HIDDEN no_deleter {
	void operator()(settings*) const {}
};
using settings_ptr = std::unique_ptr<settings, no_deleter>;

static std::map<std::string, settings_ptr> g_instances;
static spin_shared_mutex g_instances_lock;

std::vector<std::string> settings::names() noexcept
{
	std::vector<std::string> names;
	spin_shared_shared_lock locker(g_instances_lock);
	for( auto &pair : g_instances )
		names.push_back(pair.first);
	return names;
}

settings &settings::instance(std::string_view name, bool create)
{
	std::string _name(name.data(), name.size());
	spin_shared_unique_lock locker(g_instances_lock);

	auto [it, inserted] = g_instances.emplace(_name, nullptr);
	if( not inserted )
		return *it->second;

	else if( create )
	{
		auto obj = new settings(std::move(_name));
		it->second = settings_ptr(obj, no_deleter());
		return *it->second;
	}
	locker.unlock();

	throw runtime_error (
		"libsepp::settings::instance: Instance '{}' is not exist.", name
	);
	// return {};
}

settings &settings::instance()
{
	return instance("default");
}


static std::map<std::filesystem::path, const settings*> g_file_paths;
static spin_mutex g_file_paths_lock;

settings &settings::set_file_name(const path_t &file_path)
{
	spin_unique_lock locker(g_file_paths_lock);
	auto [it, inserted] = g_file_paths.emplace(file_path, this);

	if( not inserted and it->second != this )
	{
		throw runtime_error (
			"settings::set_file_name: File '{}' is already used by another instance.",
			file_path.string()
		);
	}
	g_file_paths.erase(it);
	g_file_paths.emplace(file_path, this);
	locker.unlock();

	m_impl->m_ini_lock.lock();
	m_impl->m_ini.set_file_name(file_path);
	m_impl->m_ini_lock.unlock();
	return *this;
}

std::filesystem::path settings::file_name() const noexcept
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.file_name();
}

optional<value> settings::get(group_key_t gk)
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read(std::move(gk));
}

class LIBGS_DECL_HIDDEN settings::observer::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(std::string name) :
		m_name(std::move(name)) {}
	std::string m_name;
};

settings::observer::observer(std::string name, asio::any_io_executor exec) :
	observer_base<observer, void(std::string_view,std::string_view,value), void(std::string_view)>(exec),
	m_impl(new impl(std::move(name)))
{

}

std::string_view settings::observer::name() const noexcept
{
	return m_impl->m_name;
}

settings::observer::ptr_t settings::observer::on_changed
(std::function<void(std::string_view,value)> func)
{
	on_triggered<0>([name = m_impl->m_name, func = std::move(func)]
	(std::string_view _name, std::string_view path, const value &val)
	{
		if( _name == name )
			func(path, val);
	});
	return shared_from_this();
}

settings::observer::ptr_t settings::observer::on_loaded(std::function<void()> func)
{
	on_triggered<1>([name = m_impl->m_name, func = std::move(func)](std::string_view _name)
	{
		if( _name == name )
			func();
	});
	return shared_from_this();
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
