#ifndef LIBGS_UTILS_APP_CONFIG_H
#define LIBGS_UTILS_APP_CONFIG_H

#include <libgs/utils/global.h>
#include <libgs/core/observer.h>
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

	settings &set_file_name(const path_t &file_path);
	[[nodiscard]] path_t file_name() const noexcept;

public:
	template <typename T = value>
	[[nodiscard]] decltype(auto) get_or(group_key_t gk, T &&def_value = T())
		requires concepts::value_get<T,char>;

	template <typename T = value>
	[[nodiscard]] decltype(auto) get_or (
		concepts::string_p<char> auto &&path, T &&def_value = T()
	) requires concepts::value_get<T,char>;

	template <typename T = value>
	[[nodiscard]] auto get(group_key_t gk)
		requires concepts::value_get<T,char>;

	template <typename T = value>
	[[nodiscard]] auto get (
		concepts::string_p<char> auto &&path
	) requires concepts::value_get<T,char>;

public:
	settings &set (
		const group_key_t &gk, const concepts::value_set<char> auto &value
	) noexcept;

	settings &set (
		const concepts::string_p<char> auto &path,
		const concepts::value_set<char> auto &value
	) noexcept;

public:
    class LIBGS_UTILS_API observer final : public observer_base<observer,
		void(std::string_view,std::string_view,value), void(std::string_view)>
	{
    	LIBGS_DISABLE_COPY_MOVE(observer)

	public:
		explicit observer(std::string name, asio::any_io_executor exec);
    	[[nodiscard]] std::string_view name() const noexcept;

		ptr_t on_changed(std::function<void(std::string_view,value)> func);
		ptr_t on_loaded(std::function<void()> func);

    private:
    	class impl;
    	impl *m_impl = nullptr;
	};
	using observer_ptr = observer::ptr_t;

	template <concepts::sched Exec = io_context_t&>
	[[nodiscard]] observer_ptr make_observer(Exec &&exec = io_context());

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


#endif //LIBGS_UTILS_APP_CONFIG_H
