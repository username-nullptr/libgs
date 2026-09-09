// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_UTILS_SBUS_SUBSCRIBE_H
#define LIBGS_UTILS_UTILS_SBUS_SUBSCRIBE_H

#include <libgs/utils/sbus/interface.h>
#include <libgs/core/execution.h>

namespace libgs::utils::sbus { namespace concepts
{

template <typename Func, size_t Count>
concept subscribe_func = []() consteval -> bool
{
	using func_t = std::remove_cvref_t<Func>;
	if constexpr( libgs::is_function_v<func_t> )
	{
		using func_tr_t = function_traits<func_t>;
		if constexpr( Count == 1 and func_tr_t::arg_count == Count )
		{
			using arg_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<0>>;
			return not std::is_pointer_v<arg_t>;
		}
		else if constexpr( Count == 2 and func_tr_t::arg_count == Count )
		{
			using arg0_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<0>>;
			using arg1_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<1>>;

			return libgs::concepts::constructible<arg0_t,std::string_view> and
				not topic_type<arg1_t> and not std::is_pointer_v<arg1_t>;
		}
		else
			return false;
	}
	else
		return false;
}();

template <typename Func>
concept subscribe_type_func = []() consteval -> bool
{
	using func_t = std::remove_cvref_t<Func>;
	if constexpr( libgs::is_function_v<func_t> )
	{
		using func_tr_t = function_traits<func_t>;
		if constexpr( func_tr_t::arg_count == 1 )
		{
			using arg_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<0>>;
			return topic_type<arg_t>;
		}
		else
			return false;
	}
	else
		return false;
}();

} //namespace concepts

template <concepts::interface Interface,
		  libgs::concepts::exec Exec = asio::any_io_executor>
class LIBGS_UTILS_TAPI basic_subscriber
{
public:
	using interface_t = Interface;
	using interface_ptr = std::shared_ptr<Interface>;
	using executor_t = Exec;

public:
	template <libgs::concepts::match_sched<Exec> Exec0 = io_context_t&>
	explicit basic_subscriber(Exec0 &&exec = io_context());
	~basic_subscriber();

public:
	uint64_t subscribe(std::string_view topic, concepts::subscribe_func<1> auto &&callback);
	uint64_t subscribe(concepts::subscribe_func<2> auto &&callback);

	uint64_t subscribe(std::string_view topic, libgs::concepts::callable<const void*,size_t> auto &&callback);
	uint64_t subscribe(libgs::concepts::callable<std::string_view,const void*,size_t> auto &&callback);

	uint64_t subscribe(concepts::subscribe_type_func auto &&callback);

	basic_subscriber &cancel_topic(const std::string_view &topic);
	basic_subscriber &cancel_sid(uint64_t sid);
	basic_subscriber &cancel();

public:
	template <typename...Args>
	basic_subscriber(Args&&...args) requires (
		(not std::is_same_v<std::remove_cvref_t<Args>,basic_subscriber> and ...) and
		requires(basic_subscriber &obj) { obj.subscribe(std::forward<Args>(args)...); }
	){ subscribe(std::forward<Args>(args)...); }

	[[nodiscard]] interface_ptr interface() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	interface_ptr m_interface {};
	executor_t m_exec {};
};

template <concepts::interface Interface>
using subscriber = basic_subscriber<Interface>;

template <libgs::concepts::exec Exec = asio::any_io_executor>
using basic_local_subscriber = basic_subscriber<local_interface,Exec>;

using local_subscriber = basic_local_subscriber<>;

namespace concepts
{

template <typename>
struct is_subscriber {};

template <interface Interface, libgs::concepts::exec Exec>
struct is_subscriber<basic_subscriber<Interface,Exec>> : std::true_type {};

template <typename T>
constexpr bool is_subscriber_v = is_subscriber<T>::value;

template <typename T>
concept subscriber = is_subscriber_v<T>;

} //namespace concepts

template <concepts::subscriber Subscriber,
	libgs::concepts::match_sched<typename Subscriber::executor_t> Exec0, typename...Args>
LIBGS_UTILS_TAPI std::pair<Subscriber,uint64_t> subscribe(Exec0 &&exec, Args&&...args) requires requires {
	Subscriber(std::forward<Exec0>(exec)).subscribe(std::forward<Args>(args)...);
};

template <concepts::subscriber Subscriber, typename...Args>
LIBGS_UTILS_TAPI std::pair<Subscriber,uint64_t> subscribe(Args&&...args) requires requires {
	Subscriber().subscribe(std::forward<Args>(args)...);
};

template <libgs::concepts::match_sched<local_subscriber::executor_t> Exec0, typename...Args>
LIBGS_UTILS_TAPI std::pair<local_subscriber,uint64_t> subscribe(Exec0 &&exec, Args&&...args) requires requires {
	local_subscriber(std::forward<Exec0>(exec)).subscribe(std::forward<Args>(args)...);
};

template <typename...Args>
LIBGS_UTILS_TAPI std::pair<local_subscriber,uint64_t> subscribe(Args&&...args) requires requires {
	local_subscriber().subscribe(std::forward<Args>(args)...);
};

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/subscribe.h>


#endif //LIBGS_UTILS_UTILS_SBUS_SUBSCRIBE_H
