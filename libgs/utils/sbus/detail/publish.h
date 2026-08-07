
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

#ifndef LIBGS_UTILS_UTILS_SBUS_DETAIL_PUBLISH_H
#define LIBGS_UTILS_UTILS_SBUS_DETAIL_PUBLISH_H

namespace libgs::utils::sbus { namespace detail
{

template <concepts::interface Interface, libgs::concepts::any_string_p Str>
void publish(std::string_view topic, Str &&str)
{
	auto view = strtls::to_view(std::forward<Str>(str));
	Interface::publish(topic, view.data(), view.size());
}

template <concepts::interface Interface, concepts::unregistered_type_p T>
void publish(std::string_view topic, const T &value)
{
	using value_t = std::remove_cvref_t<T>;
	if constexpr( libgs::concepts::streamer_type<value_t> )
	{
		auto buffer = streamer<value_t>::encode(value);
		Interface::publish(topic, buffer.data(), buffer.size());
	}
	else if constexpr( std::is_same_v<value_t, const_buffer> or
		std::is_same_v<value_t, asio::const_buffer> )
		Interface::publish(topic, value.data(), value.size());
	else
		Interface::publish(topic, &value, sizeof(value_t));
}

template <concepts::interface Interface>
void publish(concepts::topic_type auto &&value)
{
	using Value = decltype(value);
	using value_t = std::remove_cvref_t<Value>;

	if constexpr( libgs::concepts::streamer_type<value_t> )
	{
		auto buffer = streamer<value_t>::encode(value);
		Interface::publish(value_t::libgs_sbus_topic_v, buffer.data(), buffer.size());
	}
	else
		Interface::publish(value_t::libgs_sbus_topic_v, &value, sizeof(value_t));
}

} //namespace detail

template <concepts::interface Interface>
void publish(std::string_view topic, const void *buffer, size_t size)
{
	Interface::publish(topic, buffer, size);
}

template <concepts::interface Interface, libgs::concepts::any_string_p...Args>
void publish(std::string_view topic, Args&&...args) requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(detail::publish<Interface>(topic, std::forward<Args>(args)), 0) ...
	};
}

template <concepts::interface Interface, concepts::unregistered_type_p...Args>
void publish(std::string_view topic, Args&&...args) requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(detail::publish<Interface>(topic, std::forward<Args>(args)), 0) ...
	};
}

template <concepts::interface Interface, concepts::topic_type...Args>
void publish(Args&&...args) requires (sizeof...(Args) > 0)
{
	(void) std::initializer_list<int> {
		(detail::publish<Interface>(std::forward<Args>(args)), 0) ...
	};
}

template <typename T>
void publish(std::string_view topic, T &&value) requires
(not std::is_pointer_v<std::remove_cvref_t<T>> and not concepts::topic_type<T>)
{
	publish<local_interface>(topic, std::forward<T>(value));
}

template <libgs::concepts::any_string_p...Args>
void publish(std::string_view topic, Args&&...args) requires (sizeof...(Args) > 0)
{
	publish<local_interface>(topic, std::forward<Args>(args)...);
}

template <concepts::unregistered_type_p...Args>
void publish(std::string_view topic, Args&&...args) requires (sizeof...(Args) > 0)
{
	publish<local_interface>(topic, std::forward<Args>(args)...);
}

template <concepts::topic_type...Args>
void publish(Args&&...args) requires (sizeof...(Args) > 0)
{
	publish<local_interface>(std::forward<Args>(args)...);
}

} //namespace libgs::utils::sbus


#endif //LIBGS_UTILS_UTILS_SBUS_DETAIL_PUBLISH_H
