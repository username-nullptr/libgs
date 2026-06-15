
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

#ifndef LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H
#define LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H

#include <libgs/core/shared_mutex.h>

#include "libgs/utils/logger.h"

namespace libgs::utils
{

template <concepts::streamer_type_p Tag>
struct arg_converter<std::vector<std::byte>,Tag>
{
	static constexpr bool valid = true;
	using payload_t = std::vector<std::byte>;
	using target_t = std::remove_cvref_t<Tag>;

	[[nodiscard]] static target_t convert(const payload_t &value) requires valid {
		return *streamer<target_t>::decode(value);
	}
};

namespace sbus
{

template <concepts::subscriber Subscriber>
class LIBGS_UTILS_TAPI cache<Subscriber>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

	template <typename...Args>
	using signal_ptr = std::shared_ptr<signal_t<Args...>>;

public:
	template <typename Exec0>
	explicit impl(Exec0 &&exec) :
		m_subscriber(std::forward<Exec0>(exec))
	{
		m_subscriber.subscribe (
		[this](std::string_view topic, const void *data, size_t size) -> awaitable<void>
		{
			std::unique_lock locker(m_caches_mutex);
			auto &curr = m_caches[std::string(topic)];

			if( curr.size() == size )
			{
				if( memcmp(curr.data(), data, size) == 0 )
					co_return ;
			}
			std::span view {
				static_cast<const std::byte*>(data), size
			};
			auto _prev = std::move(curr);
			auto _curr = curr = { view.begin(), view.end() };
			locker.unlock();

			signal_ptr<payload_t,payload_t> signal {};
			m_signals_mutex.lock();
			{
				auto &obj = m_signals[std::string(topic)];
				if( not obj )
					obj = std::make_shared<signal_t<payload_t,payload_t>>();
				signal = obj;
			}
			m_signals_mutex.unlock();

			co_await signal->emit(_curr, _prev);
			co_await m_signal.emit(topic, _curr, _prev);
			co_return ;
		});
	}
	~impl() {
		m_subscriber.cancel();
	}

public:
	subscriber_t m_subscriber {};
	std::unordered_map<std::string,payload_t> m_caches {};
	spin_shared_mutex m_caches_mutex {};

	signal_t<std::string_view,payload_t,payload_t> m_signal {};
	std::unordered_map<std::string,signal_ptr<payload_t,payload_t>> m_signals {};
	spin_mutex m_signals_mutex {};
};

template <concepts::subscriber Subscriber>
template <typename Exec0>
cache<Subscriber>::cache(Exec0 &&exec)
	requires libgs::concepts::match_sched<Exec0,executor_t> :
	m_impl(std::make_unique<impl>(std::forward<Exec0>(exec)))
{

}

template <concepts::subscriber Subscriber>
cache<Subscriber>::~cache() = default;

template <concepts::subscriber Subscriber>
cache<Subscriber>::payload_t cache<Subscriber>::get(std::string_view topic) const
{
	return m_impl->m_cache[std::string(topic)];
}

template <concepts::subscriber Subscriber>
template <typename T>
T cache<Subscriber>::get(std::string_view topic) const
{
	using type = std::remove_cvref_t<T>;
	if constexpr( concepts::topic_type<type,interface_t> )
	{
		if( topic != type::libgs_sbus_topic_v )
			invalid_argument::loc_throw("Topic does not match.");
	}
	auto payload = m_impl->m_cache[std::string(topic)];

	if constexpr( libgs::concepts::streamer_type<type> )
		return streamer<type>::decode(payload);
	else
		return *reinterpret_cast<const type*>(payload.data());
}

template <concepts::subscriber Subscriber>
template <typename T>
T cache<Subscriber>::get() const requires
	concepts::topic_type<T,interface_t>
{
	using type = std::remove_cvref_t<T>;
	auto payload = get(type::libgs_sbus_topic_v);

	if constexpr( libgs::concepts::streamer_type<type> )
		return streamer<type>::decode(payload);
	else
		return *reinterpret_cast<const type*>(payload.data());
}

template <concepts::subscriber Subscriber>
cache<Subscriber>::template signal_t <
	typename cache<Subscriber>::payload_t, typename cache<Subscriber>::payload_t
>&
cache<Subscriber>::changed(std::string_view topic) noexcept
{
	std::unique_lock locker(m_impl->m_signals_mutex); LIBGS_UNUSED(locker);
	auto &signal = m_impl->m_signals[std::string(topic)];
	if( not signal )
		signal = std::make_shared<signal_t<payload_t,payload_t>>();
	return *signal;
}

template <concepts::subscriber Subscriber>
cache<Subscriber>::template signal_t<std::string_view,
	typename cache<Subscriber>::payload_t, typename cache<Subscriber>::payload_t
>&
cache<Subscriber>::changed() noexcept
{
	return m_impl->m_signal;
}

template <concepts::subscriber Subscriber>
template <typename T>
cache<Subscriber>::template signal_t <
	typename cache<Subscriber>::payload_t, typename cache<Subscriber>::payload_t
>&
cache<Subscriber>::changed() noexcept requires
	concepts::topic_type<T,interface_t>
{
	using type = std::remove_cvref_t<T>;
	return changed(type::libgs_sbus_topic_v);
}

template <concepts::subscriber Subscriber>
cache<Subscriber>::subscriber_t cache<Subscriber>::subscriber() noexcept
{
	return m_impl->m_subscriber;
}

template <concepts::subscriber Subscriber>
cache<Subscriber>::executor_t cache<Subscriber>::get_executor() noexcept
{
	return subscriber().get_executor();
}

}} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_DETAIL_CACHE_H