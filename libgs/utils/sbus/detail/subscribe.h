
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_UTILS_UTILS_SBUS_DETAIL_SUBSCRIBE_H
#define LIBGS_UTILS_UTILS_SBUS_DETAIL_SUBSCRIBE_H

namespace libgs::utils::sbus
{

template <concepts::interface Interface, libgs::concepts::exec Exec>
template <libgs::concepts::match_sched<Exec> Exec0>
basic_subscriber<Interface,Exec>::basic_subscriber(Exec0 &&exec) :
	m_interface(std::make_shared<interface_t>()),
	m_exec(libgs::get_executor_helper(std::forward<Exec0>(exec)))
{
	constexpr bool has_interface =
		requires(interface_t &interface) { interface.init(); };

	if constexpr( has_interface )
		m_interface->init();
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec>::~basic_subscriber() = default;

template <concepts::interface Interface, libgs::concepts::exec Exec>
uint64_t basic_subscriber<Interface,Exec>::subscribe
(std::string_view topic, concepts::subscribe_func<1> auto &&callback)
{
	using Func = decltype(callback);
	using func_t = std::remove_cvref_t<Func>;

	using func_tr_t = function_traits<func_t>;
	using return_t = func_tr_t::return_type;

	using arg_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<0>>;
	if constexpr( concepts::topic_type<arg_t> )
	{
		if( topic != arg_t::libgs_sbus_topic_v )
			invalid_argument::loc_throw("Topic does not match.");
	}
	if constexpr( is_awaitable_v<return_t> )
	{
		return subscribe(topic, [func = std::forward<Func>(callback)]
		(const void *data, size_t size) -> awaitable<void>
		{
			if constexpr( libgs::concepts::streamer_type<arg_t> )
			{
				std::span view {
					static_cast<const std::byte*>(data), size
				};
				co_await func(streamer<arg_t>::decode({view.begin(), view.end()}));
			}
			else if constexpr( std::is_same_v<arg_t, const_buffer> or
				std::is_same_v<arg_t, asio::const_buffer> )
				co_await func(asio::buffer(data, size));
			else
				co_await func(*static_cast<const arg_t*>(data));
			co_return ;
		});
	}
	else
	{
		return subscribe(topic, [func = std::forward<Func>(callback)]
		(const void *data, size_t size)
		{
			if constexpr( libgs::concepts::streamer_type<arg_t> )
			{
				std::span view {
					static_cast<const std::byte*>(data), size
				};
				func(streamer<arg_t>::decode({view.begin(), view.end()}));
			}
			else if constexpr( std::is_same_v<arg_t, const_buffer> or
				std::is_same_v<arg_t, asio::const_buffer> )
				func(asio::buffer(data, size));
			else
				func(*static_cast<const arg_t*>(data));
		});
	}
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
uint64_t basic_subscriber<Interface,Exec>::subscribe(concepts::subscribe_func<2> auto &&callback)
{
	using Func = decltype(callback);
	using func_t = std::remove_cvref_t<Func>;

	using func_tr_t = function_traits<func_t>;
	using return_t = func_tr_t::return_type;

	using arg1_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<1>>;
	if constexpr( is_awaitable_v<return_t> )
	{
		return subscribe([func = std::forward<Func>(callback)]
		(const std::string_view &topic, const void *data, size_t size) -> awaitable<void>
		{
			if constexpr( libgs::concepts::streamer_type<arg1_t> )
			{
				std::span view {
					static_cast<const std::byte*>(data), size
				};
				co_await func(topic, streamer<arg1_t>::decode({view.begin(), view.end()}));
			}
			else if constexpr( std::is_same_v<arg1_t, const_buffer> or
				std::is_same_v<arg1_t, asio::const_buffer> )
				co_await func(topic, asio::buffer(data, size));
			else
				co_await func(topic, *static_cast<const arg1_t*>(data));
			co_return ;
		});
	}
	else
	{
		return subscribe([func = std::forward<Func>(callback)]
		(const std::string_view &topic, const void *data, size_t size)
		{
			if constexpr( libgs::concepts::streamer_type<arg1_t> )
			{
				std::span view {
					static_cast<const std::byte*>(data), size
				};
				func(topic, streamer<arg1_t>::decode({view.begin(), view.end()}));
			}
			else if constexpr( std::is_same_v<arg1_t, const_buffer> or
				std::is_same_v<arg1_t, asio::const_buffer> )
				func(topic, asio::buffer(data, size));
			else
				func(topic, *static_cast<const arg1_t*>(data));
		});
	}
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
uint64_t basic_subscriber<Interface,Exec>::subscribe
(std::string_view topic, libgs::concepts::callable<const void*,size_t> auto &&callback)
{
	using Func = decltype(callback);
	using func_t = std::remove_cvref_t<Func>;

	constexpr bool has_interface = requires(interface_t &interface) {
		static_cast<uint64_t>(interface.subscribe (
			topic, [](const void *, size_t) {}
		));
	};
	if constexpr( has_interface )
	{
		return m_interface->subscribe(topic,
		[exec = m_exec, func = std::forward<Func>(callback)]
		(const void *data, size_t size) noexcept
		{
			using func_tr_t = function_traits<func_t>;
			using return_t = func_tr_t::return_type;

			if constexpr( is_awaitable_v<return_t> )
			{
				dispatch(exec, [&]() -> awaitable<void>
				{
					co_await func(data, size);
					co_return ;
				},
				use_sync);
			}
			else
				dispatch(exec, [&]{ func(data, size); }, use_sync);
		});
	}
	else
	{
		return m_interface->subscribe(
		[exec = m_exec, topic, func = std::forward<Func>(callback)]
		(const std::string_view &t, const void *data, size_t size) noexcept
		{
			if( t != topic )
				return ;

			using func_tr_t = function_traits<func_t>;
			using return_t = func_tr_t::return_type;

			if constexpr( is_awaitable_v<return_t> )
			{
				dispatch(exec, [&]() -> awaitable<void>
				{
					co_await func(data, size);
					co_return ;
				},
				use_sync);
			}
			else
				dispatch(exec, [&]{ func(data, size); }, use_sync);
		});
	}
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
uint64_t basic_subscriber<Interface,Exec>::subscribe
(libgs::concepts::callable<std::string_view,const void*,size_t> auto &&callback)
{
	using Func = decltype(callback);
	using func_t = std::remove_cvref_t<Func>;

	return m_interface->subscribe(
	[exec = m_exec, func = std::forward<Func>(callback)]
	(std::string_view topic, const void *data, size_t size) noexcept
	{
		using func_tr_t = function_traits<func_t>;
		using return_t = func_tr_t::return_type;

		if constexpr( is_awaitable_v<return_t> )
		{
			dispatch(exec, [&]() -> awaitable<void>
			{
				co_await func(topic, data, size);
				co_return ;
			},
			use_sync);
		}
		else
		{
			dispatch(exec, [&]{
				func(topic, data, size);
			}, use_sync);
		}
	});
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
uint64_t basic_subscriber<Interface,Exec>::subscribe(concepts::subscribe_type_func auto &&callback)
{
	using Func = decltype(callback);
	using func_t = std::remove_cvref_t<Func>;

	using func_tr_t = function_traits<func_t>;
	using arg_t = std::remove_cvref_t<typename func_tr_t::template arg_type_t<0>>;

	return subscribe(arg_t::libgs_sbus_topic_v, std::forward<Func>(callback));
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec> &basic_subscriber<Interface,Exec>::cancel_topic(const std::string_view &topic)
{
	m_interface->cancel_topic(topic);
	return *this;
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec> &basic_subscriber<Interface,Exec>::cancel_sid(uint64_t sid)
{
	m_interface->cancel_sid(sid);
	return *this;
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec> &basic_subscriber<Interface,Exec>::cancel()
{
	m_interface->cancel();
	return *this;
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec>::interface_ptr basic_subscriber<Interface,Exec>::interface() noexcept
{
	return m_interface;
}

template <concepts::interface Interface, libgs::concepts::exec Exec>
basic_subscriber<Interface,Exec>::executor_t basic_subscriber<Interface,Exec>::get_executor() noexcept
{
	return m_exec;
}

template <concepts::subscriber Subscriber,
	libgs::concepts::match_sched<typename Subscriber::executor_t> Exec0, typename...Args>
std::pair<Subscriber,uint64_t> subscribe(Exec0 &&exec, Args&&...args) requires requires
	{ Subscriber(std::forward<Exec0>(exec)).subscribe(std::forward<Args>(args)...); }
{
	Subscriber obj(std::forward<Exec0>(exec));
	auto sid = obj.subscribe(std::forward<Args>(args)...);
	return { std::move(obj), sid };
}

template <concepts::subscriber Subscriber, typename...Args>
std::pair<Subscriber,uint64_t> subscribe(Args&&...args) requires requires
	{ Subscriber().subscribe(std::forward<Args>(args)...); }
{
	Subscriber obj;
	auto sid = obj.subscribe(std::forward<Args>(args)...);
	return { std::move(obj), sid };
}

template <libgs::concepts::match_sched<local_subscriber::executor_t> Exec0, typename...Args>
std::pair<local_subscriber,uint64_t> subscribe(Exec0 &&exec, Args&&...args) requires requires
	{ local_subscriber(std::forward<Exec0>(exec)).subscribe(std::forward<Args>(args)...); }
{
	return subscribe<local_subscriber>(std::forward<Exec0>(exec), std::forward<Args>(args)...);
}

template <typename...Args>
std::pair<local_subscriber,uint64_t> subscribe(Args&&...args) requires requires
	{ local_subscriber().subscribe(std::forward<Args>(args)...); }
{
	return subscribe<local_subscriber>(std::forward<Args>(args)...);
}

} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_DETAIL_SUBSCRIBE_H
