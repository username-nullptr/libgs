// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H
#define LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H

#include <libgs/core/shared_mutex.h>
#include <libgs/core/execution.h>

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
[[nodiscard]] LIBGS_UTILS_TAPI static bool slot_args_can_auto_cast(Args&&...call_args) noexcept
{
	using slot_tr = function_traits<Slot>;
	using indices = std::make_index_sequence<slot_tr::arg_count>;

	return [args = std::make_tuple(std::forward<Args>(call_args)...)]
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
	void emplace(Exec &&executor_arg, Slot &&slot_arg) noexcept
	{
		if constexpr( Mode == slot_mode::sync )
			m_block = true;

		m_func = [exec = get_executor_helper(std::forward<Exec>(executor_arg)),
			slot = std::forward<Slot>(slot_arg)](Args...args) mutable noexcept
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
	[[nodiscard]] static std::future<void> glob_sync_call(Slot &slot_fn, Args0&&...call_args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;

		[&slot_fn, args = std::make_tuple(std::forward<Args0>(call_args)...)]
		<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
			slot_fn(get_slot_arg<slot_tr, Is>(args)...);
		} (indices());

		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> glob_async_call(auto &exec, Slot slot_fn, Args0&&...call_args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec,
			[slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> glob_backpressure_call(auto &exec, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			return libgs::dispatch(exec, [slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			},
			use_future);
		}
		else
		{
			return libgs::dispatch(exec,
			[slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			},
			use_future);
		}
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> guarded_glob_async_call
	(auto &exec, auto weak_observer, Slot slot_fn, Args0&&...call_args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				auto guard = weak.lock();
				if( not guard )
					co_return ;
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept
			{
				auto guard = weak.lock();
				if( not guard )
					return ;
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
		promise.set_value();
		return future;
	}

private:
	template <slot_mode Mode, typename Obj, concepts::function Slot>
	void emplace(Obj &&obj, Slot &&slot) noexcept {
		emplace<Mode>(std::forward<Obj>(obj), io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec, concepts::function Slot>
	void emplace(Obj &&observer, Exec &&executor_arg, Slot &&slot_arg) noexcept
	{
		m_obj = observer.get();
		if constexpr( Mode != slot_mode::async )
			m_block = true;

		using obj_t = std::remove_cvref_t<Obj>::element_type;
		auto weak = std::weak_ptr<obj_t>(observer);

		m_is_valid = [obj = weak] {
			return not obj.expired();
		};
		m_func = [obj = std::move(weak),
			exec = get_executor_helper(std::forward<Exec>(executor_arg)),
			slot = std::forward<Slot>(slot_arg)
		](Args...args) mutable noexcept
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
			{
				std::promise<void> promise;
				auto future = promise.get_future();
				promise.set_value();
				return future;
			}
			using slot_tr = function_traits<Slot>;
			if constexpr( Mode == slot_mode::async )
			{
				if constexpr( slot_tr::is_member_func )
					return obj_async_call(exec, obj, slot, std::move(args)...);
				else
					return guarded_glob_async_call(exec, obj, slot, std::move(args)...);
			}
			else
			{
				auto object = obj.lock();
				if( not object )
				{
					std::promise<void> promise;
					auto future = promise.get_future();
					promise.set_value();
					return future;
				}
				if constexpr( slot_tr::is_member_func )
				{
					if constexpr( Mode == slot_mode::sync )
						return obj_sync_call(std::move(object), slot, std::move(args)...);
					else
						return obj_backpressure_call(exec, std::move(object), slot, std::move(args)...);
				}
				else if constexpr( Mode == slot_mode::sync )
					return glob_sync_call(slot, std::move(args)...);
				else
					return glob_backpressure_call(exec, slot, std::move(args)...);
			}
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_sync_call
	(auto object_ptr, Slot &slot_fn, Args0&&...call_args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;

		[&object_ptr, &slot_fn, args = std::make_tuple(std::forward<Args0>(call_args)...)]
		<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
			(object_ptr.get()->*slot_fn)(get_slot_arg<slot_tr, Is>(args)...);
		} (indices());

		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_async_call
	(auto &exec, auto weak_observer, Slot slot_fn, Args0&&...call_args) noexcept
	{
		std::promise<void> promise;
		auto future = promise.get_future();

		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				auto obj = weak.lock();
				if( not obj )
					co_return ;
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return (obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept
			{
				auto obj = weak.lock();
				if( not obj )
					return ;
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					(obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
		promise.set_value();
		return future;
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static std::future<void> obj_backpressure_call
	(auto &exec, auto object_ptr, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			auto future = libgs::dispatch(exec, [obj = std::move(object_ptr),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return (obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			},
			use_future);
			future.wait();
			return future;
		}
		else
		{
			auto future = libgs::dispatch(exec, [obj = std::move(object_ptr),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					(obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			},
			use_future);
			future.wait();
			return future;
		}
	}

private:
	template <typename SlotTr, size_t I>
	[[nodiscard]] static decltype(auto) get_slot_arg(auto &args) noexcept
	{
		using arg_t = SlotTr::template arg_type_t<I>;
		return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
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
	void emplace(Exec &&executor_arg, Slot &&slot_arg) noexcept
	{
		m_func = [exec = get_executor_helper(std::forward<Exec>(executor_arg)),
			slot = std::forward<Slot>(slot_arg)](Args...args) mutable noexcept -> awaitable<void>
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
	[[nodiscard]] static awaitable<void> glob_sync_call(Slot &slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			auto args = std::make_tuple(std::forward<Args0>(call_args)...);
			co_await [&slot_fn, &args]<size_t...Is>
			(std::index_sequence<Is...>) mutable noexcept {
				return slot_fn(get_slot_arg<slot_tr, Is>(args)...);
			} (indices());
		}
		else
		{
			[&slot_fn, args = std::make_tuple(std::forward<Args0>(call_args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
				slot_fn(get_slot_arg<slot_tr, Is>(args)...);
			} (indices());
		}
		co_return ;
	}

	template <typename Slot, typename...Args0>
	static void glob_async_call(auto &exec, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec,
			[slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> glob_backpressure_call(auto &exec, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await libgs::dispatch(exec, [slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]
			() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			},
			use_awaitable);
		}
		else
		{
			co_await libgs::dispatch(exec,
			[slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)]() mutable noexcept
			{
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			},
			use_awaitable);
		}
		co_return ;
	}

	template <typename Slot, typename...Args0>
	static void guarded_glob_async_call
	(auto &exec, auto weak_observer, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				auto guard = weak.lock();
				if( not guard )
					co_return ;
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept
			{
				auto guard = weak.lock();
				if( not guard )
					return ;
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					slot(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
	}

private:
	template <slot_mode Mode, typename Obj, concepts::function Slot>
	void emplace(Obj &&obj, Slot &&slot) noexcept {
		emplace<Mode>(std::forward<Obj>(obj), io_context(), std::forward<Slot>(slot));
	}

	template <slot_mode Mode, typename Obj, concepts::sched Exec, concepts::function Slot>
	void emplace(Obj &&observer, Exec &&executor_arg, Slot &&slot_arg) noexcept
	{
		using obj_t = std::remove_cvref_t<Obj>::element_type;
		m_obj = observer.get();
		auto weak = std::weak_ptr<obj_t>(observer);

		m_is_valid = [obj = weak] {
			return not obj.expired();
		};
		m_func = [obj = std::move(weak),
			exec = get_executor_helper(std::forward<Exec>(executor_arg)),
			slot = std::forward<Slot>(slot_arg)](Args...args) mutable noexcept -> awaitable<void>
		{
			if( not slot_args_can_auto_cast<Slot>(args...) )
				co_return ;

			using slot_tr = function_traits<Slot>;
			if constexpr( Mode == slot_mode::async )
			{
				if constexpr( slot_tr::is_member_func )
					obj_async_call(exec, obj, slot, std::move(args)...);
				else
					guarded_glob_async_call(exec, obj, slot, std::move(args)...);
			}
			else
			{
				auto object = obj.lock();
				if( not object )
					co_return ;

				if constexpr( slot_tr::is_member_func )
				{
					if constexpr( Mode == slot_mode::sync )
						co_await obj_sync_call(std::move(object), slot, std::move(args)...);
					else
						co_await obj_backpressure_call(exec, std::move(object), slot, std::move(args)...);
				}
				else if constexpr( Mode == slot_mode::sync )
					co_await glob_sync_call(slot, std::move(args)...);
				else
					co_await glob_backpressure_call(exec, slot, std::move(args)...);
			}
			co_return ;
		};
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> obj_sync_call(auto object_ptr, Slot &slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			auto args = std::make_tuple(std::forward<Args0>(call_args)...);
			co_await [&object_ptr, &slot_fn, &args]<size_t...Is>
			(std::index_sequence<Is...>) mutable noexcept {
				return (object_ptr.get()->*slot_fn)(get_slot_arg<slot_tr, Is>(args)...);
			} (indices());
		}
		else
		{
			[&object_ptr, &slot_fn, args = std::make_tuple(std::forward<Args0>(call_args)...)]
			<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
				(object_ptr.get()->*slot_fn)(get_slot_arg<slot_tr, Is>(args)...);
			} (indices());
		}
		co_return ;
	}

	template <typename Slot, typename...Args0>
	static void obj_async_call(auto &exec, auto weak_observer, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				auto obj = weak.lock();
				if( not obj )
					co_return ;
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return (obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			});
		}
		else
		{
			libgs::post(exec, [weak = std::move(weak_observer),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept
			{
				auto obj = weak.lock();
				if( not obj )
					return ;
				[&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					(obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
			});
		}
	}

	template <typename Slot, typename...Args0>
	[[nodiscard]] static awaitable<void> obj_backpressure_call
	(auto &exec, auto object_ptr, Slot slot_fn, Args0&&...call_args) noexcept
	{
		using slot_tr = function_traits<Slot>;
		using indices = std::make_index_sequence<slot_tr::arg_count>;
		using return_t = function_traits<Slot>::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			co_await libgs::dispatch(exec, [obj = std::move(object_ptr),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				co_await [&]<size_t...Is>(std::index_sequence<Is...>) mutable noexcept {
					return (obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			},
			use_awaitable);
		}
		else
		{
			co_await libgs::dispatch(exec, [obj = std::move(object_ptr),
				slot = std::move(slot_fn), args = std::make_tuple(std::move(call_args)...)
			]() mutable noexcept -> awaitable<void>
			{
				[&]<size_t...Is>
				(std::index_sequence<Is...>) mutable noexcept {
					(obj.get()->*slot)(get_slot_arg<slot_tr, Is>(args)...);
				} (indices());
				co_return ;
			},
			use_awaitable);
		}
		co_return ;
	}

private:
	template <typename SlotTr, size_t I>
	[[nodiscard]] static decltype(auto) get_slot_arg(auto &args) noexcept
	{
		using arg_t = SlotTr::template arg_type_t<I>;
		return slot_arg_auto_cast<arg_t>(std::move(std::get<I>(args)));
	}

public:
	const void *m_obj = nullptr;
	std::function<bool()> m_is_valid = []{ return true; };
	std::function<func_t> m_func {};
};

} //namespace detail

template <typename T, typename Tag>
decltype(auto) arg_converter<T,Tag>::convert(const t_t &value) requires valid
{
	if constexpr( std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Tag>> )
		return static_cast<const tag_t&>(value);
	else
		return static_cast<tag_t>(value);
}

template <typename T, typename Tag>
decltype(auto) arg_converter<T,Tag>::convert(t_t &&value) requires valid
{
	if constexpr( std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Tag>> )
		return static_cast<tag_t&&>(value);
	else
		return static_cast<tag_t>(std::move(value));
}

template <typename T, typename Tag>
decltype(auto) arg_converter<T,Tag>::convert(t_t &value) requires valid
{
	if constexpr( std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Tag>> )
		return static_cast<tag_t&>(value);
	else
		return static_cast<tag_t>(value);
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
	[[nodiscard]] awaitable<void> emit(std::shared_ptr<impl> self, Args&&...args) const noexcept
		requires is_awaitable_v<typename func_traits_t::return_type>
	{
		return emit_with_signature(std::move(self),
			std::make_index_sequence<func_traits_t::arg_count>{},
			std::forward<Args>(args)...
		);
	}

private:
	template <size_t...Is, typename...Args>
	[[nodiscard]] static awaitable<void> emit_with_signature
	(std::shared_ptr<impl> self, std::index_sequence<Is...>, Args&&...args)
	{
		return co_emit<typename func_traits_t::template arg_type_t<Is>...>(
			std::move(self), std::forward<Args>(args)...
		);
	}

	template <typename...Args>
	[[nodiscard]] static awaitable<void> co_emit(std::shared_ptr<impl> self, Args...args)
	{
		if( self->m_block )
			co_return ;

		auto exec = co_await asio::this_coro::executor;
		using co_spawn_t = decltype (
			asio::co_spawn(exec, (*self->m_slots[0]->slot)(args...), deferred)
		);
		std::vector<adapter_ptr> adapters {};
		std::vector<co_spawn_t> slots {};
		{
			std::lock_guard lock(self->m_mutex);
			for(auto it=self->m_slots.begin(); it!=self->m_slots.end();)
			{
				if( (*it)->slot->m_is_valid() )
				{
					adapters.emplace_back((*it)->slot);
					++it;
				}
				else
					it = self->m_slots.erase(it);
			}
		}
		if( adapters.empty() )
			co_return ;

		slots.reserve(adapters.size());
		for(auto &slot : adapters)
		{
			slots.emplace_back (
				asio::co_spawn(exec, (*slot)(args...), deferred)
			);
		}
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

public:
	template <typename...Args>
	void emit(Args&&...args) const noexcept
		requires (not is_awaitable_v<typename func_traits_t::return_type>)
	{
		if( m_block	)
			return ;

		std::vector<adapter_ptr> nonblock_slots {};
		std::vector<adapter_ptr> block_slots {};
		{
			std::lock_guard lock(m_mutex);
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
		}
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
	m_impl(std::make_shared<impl>())
{

}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::~signal_base() = default;

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Slots&&...slots)
	noexcept requires is_global_slots_v<Mode,Slots...>
{
	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->template connect<Mode>(std::forward<Slots>(slots)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Slots&&...funcs)
	requires is_obj_slots_v<Mode,Obj,Slots...>
{
	if( not observer )
		invalid_argument::loc_throw("libgs::utils::signal::connect: observer is nullptr");

	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->template connect<Mode>(observer, std::forward<Slots>(funcs)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Exec0 &&exec, Slots&&...funcs)
	noexcept requires (Mode != slot_mode::sync) and is_global_slots_v<Mode,Slots...>
{
	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->template connect<Mode>(exec, std::forward<Slots>(funcs)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <slot_mode Mode, typename Obj, concepts::sched Exec0, typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Obj &&observer, Exec0 &&exec, Slots&&...funcs)
	requires (Mode != slot_mode::sync) and is_obj_slots_v<Mode,Obj,Slots...>
{
	if( not observer )
		invalid_argument::loc_throw("libgs::utils::signal::connect: observer is nullptr");

	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->template connect<Mode>(observer, exec, std::forward<Slots>(funcs)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Slots>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::connect(Slots&&...slots)
	noexcept requires is_global_slots_def_v<Slots...>
{
	using indices = std::make_index_sequence<sizeof...(Slots)>;
	[this, funcs = std::make_tuple(std::forward<Slots>(slots)...)]
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
signal_base<Derived,Func>::connect(Obj &&observer_arg, Slots&&...slots)
	requires is_obj_slots_def_v<Obj,Slots...>
{
	if( not observer_arg )
		invalid_argument::loc_throw("libgs::utils::signal::connect: observer is nullptr");

	using indices = std::make_index_sequence<sizeof...(Slots)>;
	[this, observer = std::forward<Obj>(observer_arg),
		funcs = std::make_tuple(std::forward<Slots>(slots)...)
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
	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->disconnect(std::forward<Slots>(funcs)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj, typename...Func0>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer, Func0&&...funcs)
	requires is_obj_slots_def_v<Obj,Func0...>
{
	if( not observer )
		invalid_argument::loc_throw("libgs::utils::signal::disconnect: observer is nullptr");

	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	(void) std::initializer_list<int> {(
		state->disconnect(observer, std::forward<Func0>(funcs)),
	0)...};
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect() noexcept
{
	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);
	state->m_slots.clear();
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename Obj>
signal_base<Derived,Func>::derived_t&
signal_base<Derived,Func>::disconnect(const Obj &observer)
	requires is_observer_v<Obj>
{
	if( not observer )
		invalid_argument::loc_throw("libgs::utils::signal::disconnect: observer is nullptr");

	auto state = m_impl;
	std::lock_guard lock(state->m_mutex);

	std::erase_if(state->m_slots, [&observer](const auto &item) {
		return item->slot->m_obj == observer.get();
	});
	return static_cast<derived_t&>(*this);
}

template <typename Derived, concepts::std_func_temp Func>
template <typename...Args>
auto signal_base<Derived,Func>::emit(Args&&...args) const noexcept
	requires is_callable_v<Args...>
{
	auto state = m_impl;
	if constexpr( is_awaitable_v<typename func_traits_t::return_type> )
		return state->emit(state, std::forward<Args>(args)...);
	else
		return state->emit(std::forward<Args>(args)...);
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
	auto state = m_impl;
	state->m_block = block;
}

template <typename Derived, concepts::std_func_temp Func>
bool signal_base<Derived,Func>::is_blocked() const noexcept
{
	auto state = m_impl;
	return state->m_block;
}

} //namespace libgs::utils

// Fuck MicroSoft ...
#endif //LIBGS_UTILS_DETAIL_SIGNAL_SLOT_H
