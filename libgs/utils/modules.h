// SPDX-FileCopyrightText: 2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_MODULES_H
#define LIBGS_UTILS_MODULES_H

#include <libgs/core/string_vector.h>
#include <libgs/core/string_set.h>
#include <libgs/utils/global.h>

namespace libgs::utils
{

class LIBGS_UTILS_API modules
{
	LIBGS_DISABLE_COPY_MOVE(modules)

public:
	struct dependency
	{
		string_set children {};
		string_set parents {};
	};
	template <typename Func>
	static constexpr bool init_func_v =
		concepts::callable_ret<Func,bool> or concepts::callable_ret<Func,bool,string_vector> or
		concepts::callable_void<Func> or concepts::callable_void<Func,string_vector>;

	template <typename Func>
	static void reg_init(std::string name, dependency depy, Func &&func)
		requires init_func_v<Func>;

	template <typename Func>
	static void reg_init(std::string name, Func &&func)
		requires init_func_v<Func>;

public:
	struct unexpected
	{
		std::vector<std::string> failures;
		std::vector<std::string> unregistered;
		std::vector<std::string> children;
	};

	template <typename Func>
	static constexpr bool init_callable_v =
		concepts::callable<Func,unexpected> or concepts::callable<Func>;

	template <typename Token>
	static constexpr bool init_token_v =
		std::is_same_v<std::remove_cvref_t<Token>, use_sync_t> or
		std::is_same_v<std::remove_cvref_t<Token>, use_future_t> or
		std::is_same_v<std::remove_cvref_t<Token>, detached_t> or
		init_callable_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(int argc, const char **argv, Token &&token = {})
		requires init_token_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(const string_vector &args, Token &&token = {})
		requires init_token_v<Token>;

	template <typename Token = use_sync_t>
	static auto do_init(Token &&token = {})
		requires init_token_v<Token>;

	template <typename Func>
	static auto do_init(int argc, const char **argv,
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

	template <typename Func>
	static auto do_init(const string_vector &args,
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

	template <typename Func>
	static auto do_init (
		concepts::sched auto &&exec, Func &&callback
	) requires init_callable_v<Func>;

public:
	[[nodiscard]] static std::string sprint() noexcept;
};

#define LIBGS_UTILS_MODULE_INIT(_name, ...) \
	LIBGS_REGISTRATION { \
		libgs::utils::modules::reg_init(_name, __VA_ARGS__); \
	}

} //namespace libgs::utils
#include <libgs/utils/detail/modules.h>


#endif //LIBGS_UTILS_MODULES_H
