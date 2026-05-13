
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H
#define LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H

#include <libgs/core/shared_mutex.h>
#include <libgs/core/execution.h>

#include <iostream>


namespace libgs::utils { namespace detail
{

template <typename Tag, typename T>
[[nodiscard]] LIBGS_UTILS_TAPI static constexpr bool slot_arg_can_auto_cast(const T &arg) noexcept
{
	using target_t = std::remove_cvref_t<Tag>;
	using type = std::remove_cvref_t<T>;

	if constexpr( is_variant_v<type> )
		return std::holds_alternative<target_t>(arg);

	else if constexpr( std::is_same_v<type, std::any> )
	{
		if constexpr( std::is_same_v<target_t, std::any> )
			return true;
		else
			return arg.type() == typeid(target_t);
	}
	else
		return true;
}

template <concepts::function Slot, typename...Args>
[[nodiscard]] LIBGS_UTILS_TAPI static bool slot_args_can_auto_cast(Args&&...args) noexcept
{
	using slot_tr = function_traits<Slot>;
	using indices = std::make_index_sequence<slot_tr::arg_count>;

	return [args = std::make_tuple(std::forward<Args>(args)...)]
	<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
	{
		return ([&]() mutable noexcept
		{
			using arg_t = slot_tr::template arg_type_t<Is>;
			return slot_arg_can_auto_cast<arg_t>(std::get<Is>(args));
		}
		() and ...);
	}
	(indices());
}

template <typename Tag, typename T>
[[nodiscard]] LIBGS_UTILS_TAPI static constexpr decltype(auto) slot_arg_auto_cast(T &&arg)
{
	using target_t = std::remove_cvref_t<Tag>;
	using type = std::remove_cvref_t<T>;

	if constexpr( is_variant_v<type> )
		return std::get<target_t>(std::forward<T>(arg));

	else if constexpr( std::is_same_v<type, std::any> )
	{
		if constexpr( std::is_same_v<target_t, std::any> )
			return std::forward<T>(arg);
		else
			return std::any_cast<target_t>(std::forward<T>(arg));
	}
	else
		return arg_converter<T,Tag>::convert(std::forward<T>(arg));
}

template <concepts::function Signal>
class slot_adapter; // Type erasure

template <typename Ret, typename...Args>
class LIBGS_UTILS_TAPI slot_adapter<Ret(Args...)> :
	public std::enable_shared_from_this<slot_adapter<Ret(Args...)>>
{
	LIBGS_DISABLE_COPY_MOVE(slot_adapter)

public:
	using ptr_t = std::shared_ptr<slot_adapter>;
	using func_t = std::future<void>(Args...);
	slot_adapter() = default;

	template <slot_mode Mode, typename...Args0>
	[[nodiscard]] static ptr_t make(Args0&&...args) noexcept
	{
		auto obj = std::make_shared<slot_adapter>();
		obj->template emplace<Mode>(std::forward<Args0>(args)...);
		return obj;
	}

	template <typename...Args0>
	[[nodiscard]] std::future<void> operator()(Args0&&...args) noexcept {
		return m_func(std::forward<Args0>(args)...);
	}

private:
	template <slot_mode Mode, concepts::function Slot>
	void emplace(Slot &&slot) noexcept {
		emplace<Mode>(io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, concepts::sched Exec, concepts::function Slot>
	void emplace(Exec &&exec, Slot &&slot) noexcept
	{
		if constexpr( Mode == slot_mode::sync )
			m_block = true;

		m_func = [exec = get_executor_helper(std::forward<Exec>(exec)),
			slot = std::forward<Slot>(slot)](Args...args) mutable noexcept
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
			{
				std::promise<void> promise;
				auto future = promise.get_future();
				promise.set_value();
				return future;
			}
			if constexpr( Mode == slot_mode::sync )
				return glob_sync_call(slot, std::move(args)...);

			else if constexpr( Mode == slot_mode::async )
				return glob_async_call(exec, slot, std::move(args)...);

			else /* if constexpr( Mode == slot_mode::backpressure ) */
				return glob_backpressure_call(exec, slot, std::move(args)...);
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> glob_sync_call(Slot &slot, Args0&&...args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;

		[&slot, args = std::make_tuple(std::forward<Args0>(args)...)]
		<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
		{
			auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
			{
				using arg_t = slot_tr::template arg_type_t<I>;
				return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
			};
			slot(get_arg.template operator()<Is>()...);
		}
		(indices());

		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> glob_async_call(auto &exec, Slot slot, Args0&&...args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [slot = std::move(slot), args = std::make_tuple(std::move(args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await slot(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			});
		}
		else
		{
			libgs::post(exec,
			[slot = std::move(slot), args = std::make_tuple(std::move(args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					slot(get_arg.template operator()<Is>()...);
				}
				(indices());
			});
		}
		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> glob_backpressure_call(auto &exec, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			return libgs::dispatch(exec, [slot = std::move(slot), args = std::make_tuple(std::move(args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await slot(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			},
			use_future);
		}
		else
		{
			return libgs::dispatch(exec,
			[slot = std::move(slot), args = std::make_tuple(std::move(args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					slot(get_arg.template operator()<Is>()...);
				}
				(indices());
			},
			use_future);
		}
	}

private:
	template <slot_mode Mode, typename Obj, concepts::function Slot>
	void emplace(Obj &&obj, Slot &&slot) noexcept {
		emplace<Mode>(std::forward<Obj>(obj), io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec, concepts::function Slot>
	void emplace(Obj &&obj, Exec &&exec, Slot &&slot) noexcept
	{
		m_obj = obj.get();
		if constexpr( Mode != slot_mode::async )
			m_block = true;

		using obj_t = std::remove_cvref_t<Obj>::element_type;
		m_is_valid = [obj = std::weak_ptr<obj_t>(obj)] {
			return not obj.expired();
		};
		m_func = [obj = obj.get(), is_valid = m_is_valid,
			exec = get_executor_helper(std::forward<Exec>(exec)),
			slot = std::forward<Slot>(slot)](Args...args) mutable noexcept
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
			{
				std::promise<void> promise;
				auto future = promise.get_future();
				promise.set_value();
				return future;
			}
			using slot_tr = function_traits<Slot>;
			if constexpr( slot_tr::is_member_func )
			{
				if constexpr( Mode == slot_mode::sync )
					return obj_sync_call(obj, slot, std::move(args)...);

				else if constexpr( Mode == slot_mode::async )
					return obj_async_call(exec, obj, is_valid, slot, std::move(args)...);

				else /* if constexpr( Mode == slot_mode::backpressure ) */
					return obj_backpressure_call(exec, obj, is_valid, slot, std::move(args)...);
			}
			else if constexpr( Mode == slot_mode::sync )
				return glob_sync_call(slot, std::move(args)...);

			else if constexpr( Mode == slot_mode::async )
				return glob_async_call(exec, slot, std::move(args)...);

			else /* if constexpr( Mode == slot_mode::backpressure ) */
			return 	glob_backpressure_call(exec, slot, std::move(args)...);
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_sync_call(auto &obj, Slot &slot, Args0&&...args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;

		[&obj, &slot, args = std::make_tuple(std::forward<Args0>(args)...)]
		<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
		{
			auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
			{
				using arg_t = slot_tr::template arg_type_t<I>;
				return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
			};
			(obj->*slot)(get_arg.template operator()<Is>()...);
		}
		(indices());

		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_async_call
	(auto &exec, auto obj, auto is_valid, Slot slot, Args0&&...args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept -> awaitable<void>
			{
				if( not is_valid() )
					co_return ;

				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await (obj->*slot)(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			});
		}
		else
		{
			libgs::post(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept
			{
				if( not is_valid() )
					return ;

				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					(obj->*slot)(get_arg.template operator()<Is>()...);
				}
				(indices());
			});
		}
		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_backpressure_call
	(auto &exec, auto obj, auto is_valid, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			return libgs::dispatch(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept -> awaitable<void>
			{
				if( not is_valid() )
					co_return ;

				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await (obj->*slot)(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			},
			use_future).wait();
		}
		else
		{
			return libgs::dispatch(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept
			{
				if( not is_valid() )
					return ;

				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					(obj->*slot)(get_arg.template operator()<Is>()...);
				}
				(indices());
			},
			use_future).wait();
		}
	}

public:
	const void *m_obj = nullptr;
	std::function<bool()> m_is_valid = []{ return true; };

	bool m_block = false;
	std::function<func_t> m_func {};
};

template <typename Ret, typename...Args>
class LIBGS_UTILS_TAPI slot_adapter<awaitable<Ret>(Args...)> :
	public std::enable_shared_from_this<slot_adapter<awaitable<Ret>(Args...)>>
{
	LIBGS_DISABLE_COPY_MOVE(slot_adapter)

public:
	using ptr_t = std::shared_ptr<slot_adapter>;
	using func_t = awaitable<void>(Args...);
	slot_adapter() = default;

	template <slot_mode Mode, typename...Args0>
	[[nodiscard]] static ptr_t make(Args0&&...args) noexcept
	{
		auto obj = std::make_shared<slot_adapter>();
		obj->template emplace<Mode>(std::forward<Args0>(args)...);
		return obj;
	}

	template <typename...Args0>
	[[nodiscard]] awaitable<void> operator()(Args0&&...args) noexcept {
		co_return co_await m_func(std::forward<Args0>(args)...);
	}

private:
	template <slot_mode Mode, concepts::function Slot>
	void emplace(Slot &&slot) noexcept {
		emplace<Mode>(io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, concepts::sched Exec, concepts::function Slot>
	void emplace(Exec &&exec, Slot &&slot) noexcept
	{
		m_func = [exec = get_executor_helper(std::forward<Exec>(exec)),
			slot = std::forward<Slot>(slot)](Args...args) mutable noexcept -> awaitable<void>
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
				co_return ;

			if constexpr( Mode == slot_mode::sync )
				co_await glob_sync_call(slot, std::move(args)...);

			else if constexpr( Mode == slot_mode::async )
				glob_async_call(exec, slot, std::move(args)...);

			else /* if constexpr( Mode == slot_mode::backpressure ) */
				co_await glob_backpressure_call(exec, slot, std::move(args)...);
			co_return ;
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> glob_sync_call(Slot &slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await [&slot, args = std::make_tuple(std::forward<Args0>(args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
			{
				auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
				{
					using arg_t = slot_tr::template arg_type_t<I>;
					return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
				};
				co_await slot(get_arg.template operator()<Is>()...);
				co_return ;
			}
			(indices());
		}
		else
		{
			[&slot, args = std::make_tuple(std::forward<Args0>(args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
			{
				auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
				{
					using arg_t = slot_tr::template arg_type_t<I>;
					return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
				};
				slot(get_arg.template operator()<Is>()...);
			}
			(indices());
		}
		co_return ;
	}

	template <typename Slot, typename...Args0>
	static void glob_async_call(auto &exec, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [slot = std::move(slot), args = std::make_tuple(std::move(args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await slot(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			});
		}
		else
		{
			libgs::post(exec,
			[slot = std::move(slot), args = std::make_tuple(std::move(args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					slot(get_arg.template operator()<Is>()...);
				}
				(indices());
			});
		}
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> glob_backpressure_call(auto &exec, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await libgs::dispatch(exec, [slot = std::move(slot), args = std::make_tuple(std::move(args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await slot(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			},
			use_awaitable);
		}
		else
		{
			co_await libgs::dispatch(exec,
			[slot = std::move(slot), args = std::make_tuple(std::move(args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					slot(get_arg.template operator()<Is>()...);
				}
				(indices());
			},
			use_awaitable);
		}
		co_return ;
	}

private:
	template <slot_mode Mode, typename Obj, concepts::function Slot>
	void emplace(Obj &&obj, Slot &&slot) noexcept {
		emplace<Mode>(std::forward<Obj>(obj), io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec, concepts::function Slot>
	void emplace(Obj &&obj, Exec &&exec, Slot &&slot) noexcept
	{
		using obj_t = std::remove_cvref_t<Obj>::element_type;
		m_obj = obj.get();

		m_is_valid = [obj = std::weak_ptr<obj_t>(obj)] {
			return not obj.expired();
		};
		m_func = [obj = obj.get(), is_valid = m_is_valid,
			exec = get_executor_helper(std::forward<Exec>(exec)),
			slot = std::forward<Slot>(slot)](Args...args) mutable noexcept -> awaitable<void>
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
				co_return ;

			using slot_tr = function_traits<Slot>;
			if constexpr( slot_tr::is_member_func )
			{
				if constexpr( Mode == slot_mode::sync )
					co_await obj_sync_call(obj, slot, std::move(args)...);

				else if constexpr( Mode == slot_mode::async )
					obj_async_call(exec, obj, is_valid, slot, std::move(args)...);

				else /* if constexpr( Mode == slot_mode::backpressure ) */
					co_await obj_backpressure_call(exec, obj, is_valid, slot, std::move(args)...);
			}
			else if constexpr( Mode == slot_mode::sync )
				co_await glob_sync_call(slot, std::move(args)...);

			else if constexpr( Mode == slot_mode::async )
				glob_async_call(exec, slot, std::move(args)...);

			else /* if constexpr( Mode == slot_mode::backpressure ) */
				co_await glob_backpressure_call(exec, slot, std::move(args)...);
			co_return ;
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> obj_sync_call(auto &obj, Slot &slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await [&obj, &slot, args = std::make_tuple(std::forward<Args0>(args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
			{
				auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
				{
					using arg_t = slot_tr::template arg_type_t<I>;
					return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
				};
				co_await (obj->*slot)(get_arg.template operator()<Is>()...);
				co_return ;
			}
			(indices());
		}
		else
		{
			[&obj, &slot, args = std::make_tuple(std::forward<Args0>(args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
			{
				auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
				{
					using arg_t = slot_tr::template arg_type_t<I>;
					return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
				};
				(obj->*slot)(get_arg.template operator()<Is>()...);
			}
			(indices());
		}
		co_return ;
	}

	template <typename Slot, typename...Args0>
	static void obj_async_call(auto &exec, auto obj, auto is_valid, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept -> awaitable<void>
			{
				if( not is_valid() )
					co_return ;

				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await (obj->*slot)(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			});
		}
		else
		{
			libgs::post(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept
			{
				if( not is_valid() )
					return ;

				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					(obj->*slot)(get_arg.template operator()<Is>()...);
				}
				(indices());
			});
		}
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> obj_backpressure_call
	(auto &exec, auto obj, auto is_valid, Slot slot, Args0&&...args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await libgs::dispatch(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept -> awaitable<void>
			{
				if( not is_valid() )
					co_return ;

				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await (obj->*slot)(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			},
			use_awaitable);
		}
		else
		{
			co_await libgs::dispatch(exec, [obj = std::move(obj), is_valid = std::move(is_valid),
				slot = std::move(slot), args = std::make_tuple(std::move(args)...)
			]() mutable noexcept -> awaitable<void>
			{
				if( not is_valid() )
					co_return ;

				co_return co_await [&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept -> awaitable<void>
				{
					auto get_arg = [&]<size_t I>() mutable noexcept -> decltype(auto)
					{
						using arg_t = slot_tr::template arg_type_t<I>;
						return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
					};
					co_await (obj->*slot)(get_arg.template operator()<Is>()...);
					co_return ;
				}
				(indices());
			},
			use_awaitable);
		}
		co_return ;
	}

public:
	const void *m_obj = nullptr;
	std::function<bool()> m_is_valid = []{ return true; };
	std::function<func_t> m_func {};
};

} //namespace detail

template <typename T, typename Tag>
const Tag &arg_converter<T,Tag>::convert(const t_t &value) requires valid
{
	return static_cast<const Tag&>(value);
}

template <typename T, typename Tag>
Tag &&arg_converter<T,Tag>::convert(t_t &&value) requires valid
{
	return static_cast<Tag&&>(value);
}

template <typename T, typename Tag>
Tag &arg_converter<T,Tag>::convert(t_t &value) requires valid
{
	return static_cast<Tag&>(value);
}

template <typename Derived, concepts::std_func_temp Func>
class LIBGS_UTILS_TAPI signal_base<Derived,Func>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using adapter = detail::slot_adapter<function_t>;
	using adapter_ptr = adapter::ptr_t;

	struct slot_info
	{
		const void *func = nullptr;
		adapter_ptr slot {};
	};
	using slot_info_ptr = std::shared_ptr<slot_info>;
	impl() = default;

public:
	template <slot_mode Mode, typename Func0>
	void connect(Func0 &&func) noexcept
		requires is_global_slot_v<Mode,Func0> {
		connect<Mode>(io_context(), std::forward<Func0>(func));
	}

	template <slot_mode Mode, concepts::sched Exec0, typename Func0>
	void connect(Exec0 &&exec, Func0 &&func) noexcept
		requires is_global_slot_v<Mode,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [func](const auto &info) {
			return info->func == &func and info->slot->m_obj == nullptr;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(
			std::forward<Exec0>(exec), std::forward<Func0>(func)
		);
	}

	template <slot_mode Mode, typename Obj, typename Func0>
	void connect(Obj &&observer, Func0 &&func) noexcept
		requires is_obj_slot_v<Mode,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(
			std::forward<Obj>(observer), std::forward<Func0>(func)
		);
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename Func0>
	void connect(Obj &&observer, Exec0 &&exec, Func0 &&func) noexcept
		requires is_obj_slot_v<Mode,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it == m_slots.end() )
		{
			m_slots.emplace_back(std::make_shared<slot_info>());
			it = m_slots.end() - 1;
		}
		(*it)->func = reinterpret_cast<const void*>(&func);
		(*it)->slot = adapter::template make<Mode>(std::forward<Obj>(observer),
			std::forward<Exec0>(exec), std::forward<Func0>(func)
		);
	}

public:
	template <typename Func0>
	void disconnect(Func0 &&func) noexcept
		requires is_global_slot_v<slot_mode::async,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [func](const auto &info){
			return info->func == &func and info->slot->m_obj == nullptr;
		});
		if( it != m_slots.end() )
			m_slots.erase(it);
	}

	template <typename Obj, typename Func0>
	void disconnect(const Obj &observer, Func0 &&func) noexcept
		requires is_obj_slot_v<slot_mode::async,Obj,Func0>
	{
		auto it = std::ranges::find_if(m_slots, [&observer, &func](const auto &info) {
			return info->slot->m_obj == observer.get() and info->func == &func;
		});
		if( it != m_slots.end() )
			m_slots.erase(it);
	}

public:
	template <typename...Args>
	[[nodiscard]] awaitable<void> emit(Args&&...args) const requires
		is_awaitable_v<typename func_traits_t::return_type>
	{
		if( m_block	)
			co_return ;

		auto exec = co_await asio::this_coro::executor;
		using co_spawn_t = decltype (
			asio::co_spawn(exec, (*m_slots[0]->slot)(args...), deferred)
		);
		std::vector<co_spawn_t> slots {};
		m_mutex.lock_shared();

		for(auto it=m_slots.begin(); it!=m_slots.end();)
		{
			if( (*it)->slot->m_is_valid() )
			{
				slots.emplace_back (
					asio::co_spawn(exec, (*(*it)->slot)(args...), deferred)
				);
				++it;
			}
			else
				it = m_slots.erase(it);
		}
		m_mutex.unlock_shared();

		if( slots.empty() )
			co_return ;

		auto [unused, exs] = co_await asio::experimental::make_parallel_group(std::move(slots))
			.async_wait(asio::experimental::wait_for_all(), use_awaitable);

		std::exception_ptr first_ex {};
		for(auto &ex : exs)
		{
			if( not ex )
				continue;
			else if( first_ex )
				throw asio::multiple_exceptions(first_ex);
			first_ex = ex;
		}
		if( first_ex )
			std::rethrow_exception(first_ex);
		co_return ;
	}

	template <typename...Args>
	void emit(Args&&...args) const noexcept
		requires (not is_awaitable_v<typename func_traits_t::return_type>)
	{
		if( m_block	)
			return ;

		std::vector<adapter_ptr> nonblock_slots {};
		std::vector<adapter_ptr> block_slots {};

		m_mutex.lock_shared();
		for(auto it=m_slots.begin(); it!=m_slots.end();)
		{
			if( (*it)->slot->m_is_valid() )
			{
				if( (*it)->slot->m_block )
					block_slots.emplace_back((*it)->slot);
				else
					nonblock_slots.emplace_back((*it)->slot);
				++it;
			}
			else
				it = m_slots.erase(it);
		}
		m_mutex.unlock_shared();
		std::vector<std::future<void>> futures {};

		for(auto &slot : nonblock_slots)
			futures.emplace_back((*slot)(args...));

		for(auto &slot : block_slots)
			futures.emplace_back((*slot)(args...));

		for(auto &future : futures)
			future.wait();
	}

public:
	std::atomic_bool m_block { false };
	mutable std::deque<slot_info_ptr> m_slots;
	mutable spin_shared_mutex m_mutex;
};

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::signal_base() :
	m_impl(new impl())
{

}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::~signal_base()
{
	delete m_impl;
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Slots&&...funcs)
	noexcept requires is_global_slots_v<Mode,Slots...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(std::forward<Slots>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Slots&&...funcs)
	requires is_obj_slots_v<Mode,Obj,Slots...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::connect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(observer, std::forward<Slots>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Exec0 &&exec, Slots&&...funcs)
	noexcept requires (Mode != slot_mode::sync) and is_global_slots_v<Mode,Slots...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(exec, std::forward<Slots>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Exec0 &&exec, Slots&&...funcs)
	requires (Mode != slot_mode::sync) and is_obj_slots_v<Mode,Obj,Slots...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::connect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->template connect<Mode>(observer, exec, std::forward<Slots>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Slots&&...funcs)
	noexcept requires is_global_slots_def_v<Slots...>
{
	using indices = std::make_index_sequence<sizeof...(Slots)>;
	[this, funcs = std::make_tuple(std::forward<Slots>(funcs)...)]
	<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
	{
		([&]() mutable noexcept
		{
			using slot_t = std::tuple_element_t<Is,decltype(funcs)>;
			using slot_ret = function_traits<slot_t>::return_type;
			using sig_ret = func_traits_t::return_type;

			auto &func = std::get<Is>(funcs);
			if constexpr( is_awaitable_v<slot_ret> )
			{
				if constexpr( is_awaitable_v<sig_ret> )
					connect<slot_mode::sync>(std::move(func));
				else
					connect<slot_mode::async>(std::move(func));
			}
			else
				connect<slot_mode::sync>(std::move(func));
			return true;
		}
		() and ...);
	}
	(indices());
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Slots&&...funcs)
	requires is_obj_slots_def_v<Obj,Slots...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::connect: observer is nullptr");

	using indices = std::make_index_sequence<sizeof...(Slots)>;
	[this, observer = std::forward<Obj>(observer),
		funcs = std::make_tuple(std::forward<Slots>(funcs)...)
	]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept
	{
		([&]() mutable noexcept
		{
			using slot_t = std::tuple_element_t<Is,decltype(funcs)>;
			using slot_ret = function_traits<slot_t>::return_type;
			using sig_ret = func_traits_t::return_type;

			auto &func = std::get<Is>(funcs);
			if constexpr( is_awaitable_v<slot_ret> )
			{
				if constexpr( is_awaitable_v<sig_ret> )
					connect<slot_mode::sync>(observer, std::move(func));
				else
					connect<slot_mode::async>(observer, std::move(func));
			}
			else
				connect<slot_mode::sync>(observer, std::move(func));
			return true;
		}
		() and ...);
	}
	(indices());
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Exec0 &&exec, Slots&&...funcs)
	noexcept requires is_global_slots_v<slot_mode::async,Slots...>
{
	return connect<slot_mode::async>(
		std::forward<Exec0>(exec), std::forward<Slots>(funcs)...
	);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Exec0 &&exec, Slots&&...funcs)
	requires is_obj_slots_v<slot_mode::async,Obj,Slots...>
{
	return connect<slot_mode::async>(std::forward<Obj>(observer),
		std::forward<Exec0>(exec), std::forward<Slots>(funcs)...
	);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(Slots&&...funcs)
	noexcept requires is_global_slots_def_v<Slots...>
{
	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->disconnect(std::forward<Slots>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, typename...Func0>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer, Func0&&...funcs)
	requires is_obj_slots_def_v<Obj,Func0...>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::disconnect: observer is nullptr");

	m_impl->m_mutex.lock();
	(void) std::initializer_list<int> {(
		m_impl->disconnect(observer, std::forward<Func0>(funcs)),
	0)...};
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect() noexcept
{
	m_impl->m_mutex.lock();
	m_impl->m_slots.clear();
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer)
	requires is_observer_v<Obj>
{
	if( not observer )
		throw std::invalid_argument("libgs::utils::signal::disconnect: observer is nullptr");

	m_impl->m_mutex.lock();
	for(auto it=m_impl->m_slots.begin(); it!=m_impl->m_slots.end();)
	{
		if( (*it)->slot->m_obj == observer.get() )
			it = m_impl->m_slots.erase(it);
	}
	m_impl->m_mutex.unlock();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Args>
auto signal_base<Derived,Func>::emit(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	return m_impl->emit(std::forward<Args>(args)...);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Args>
auto signal_base<Derived,Func>::operator()(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	return emit(std::forward<Args>(args)...);
}

template <typename Derived, concepts::std_func_temp Func>
void signal_base<Derived,Func>::block(bool block) noexcept
{
	m_impl->m_block = block;
}

template <typename Derived, concepts::std_func_temp Func>
bool signal_base<Derived,Func>::is_blocked() const noexcept
{
	return m_impl->m_block;
}

} //namespace libgs::utils

// Fuck MicroSoft ...
#endif //LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H