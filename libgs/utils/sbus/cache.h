
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

#ifndef LIBGS_UTILS_UTILS_SBUS_CACHE_H
#define LIBGS_UTILS_UTILS_SBUS_CACHE_H

#include <libgs/utils/sbus/subscribe.h>
#include <libgs/utils/signal_slot.h>

namespace libgs::utils::sbus
{

template <concepts::subscriber Subscriber>
class LIBGS_UTILS_TAPI cache
{
	LIBGS_DISABLE_COPY_MOVE(cache)

public:
	using subscriber_t = Subscriber;
	using executor_t = subscriber_t::executor_t;

	using interface_t = subscriber_t::interface_t;
	using payload_t = std::vector<std::byte>;

	template <typename...Args>
	using signal_t = signal<awaitable<void>(Args...)>;

public:
	template <typename Exec0 = io_context_t&>
	explicit cache(Exec0 &&exec = io_context()) requires
		libgs::concepts::match_sched<Exec0,executor_t>;

	~cache();
	[[nodiscard]] payload_t get(std::string_view topic) const;

	template <typename T>
	[[nodiscard]] T get(std::string_view topic) const;

	template <typename T>
	[[nodiscard]] T get() const requires
		concepts::topic_type<T,interface_t>;

public:
	[[nodiscard]] signal_t<payload_t,payload_t> &changed(std::string_view topic) noexcept;
	[[nodiscard]] signal_t<std::string_view,payload_t,payload_t> &changed() noexcept;

	template <typename T>
	[[nodiscard]] signal_t<payload_t,payload_t> &changed() noexcept
		requires concepts::topic_type<T,interface_t>;

public:
	[[nodiscard]] subscriber_t subscriber() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	std::unique_ptr<impl> m_impl {};
};

using local_cache = cache<local_subscriber>;

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/cache.h>


#endif //LIBGS_UTILS_UTILS_SBUS_CACHE_H