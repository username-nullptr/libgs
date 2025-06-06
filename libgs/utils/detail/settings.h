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

template <typename T>
decltype(auto) settings::get_or(group_key_t gk, T &&def_value)
	requires concepts::value_get<T,char>
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read_or(std::move(gk), std::forward<T>(def_value));
}

template <typename T>
decltype(auto) settings::get_or(concepts::string_p<char> auto &&path, T &&def_value)
	requires concepts::value_get<T,char>
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read_or (
		std::forward<decltype(path)>(path), std::forward<T>(def_value)
	);
}

template <typename T>
auto settings::get(group_key_t gk)
	requires concepts::value_get<T,char>
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read<T>(std::move(gk));
}

template <typename T>
auto settings::get(concepts::string_p<char> auto &&path)
	requires concepts::value_get<T,char>
{
	spin_shared_shared_lock locker(m_impl->m_ini_lock); LIBGS_UNUSED(locker);
	return m_impl->m_ini.read<T>(std::forward<decltype(path)>(path));
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
