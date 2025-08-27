
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

	observer::trigger<0> (
		name(), gk.group + "/" + gk.key, value
	);
	return *this;
}

settings &settings::set
(const concepts::string_p<char> auto &path, const concepts::value_set<char> auto &value) noexcept
{
	m_impl->m_ini_lock.lock();
	m_impl->m_ini.write(path, value);
	m_impl->m_ini_lock.unlock();

	observer::trigger<0> (
		name(), path, value
	);
	return *this;
}

template <concepts::sched Exec>
settings::observer_ptr settings::make_observer(Exec &&exec)
{
	return observer::make(std::string(name()),
		get_executor_helper(std::forward<Exec>(exec))
	);
}

} //namespace libgs::utils


#endif //LIBGS_UTILS_DETAIL_SETTINGS_H
