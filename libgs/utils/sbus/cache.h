
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
#include <libgs/utils/sbus/publish.h>
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

public:
	cache &set(std::string_view topic, const void *data, size_t size);

	template <libgs::concepts::any_string_p...Args>
	cache &set(std::string_view topic, Args&&...args)
		requires (sizeof...(Args) > 0);

	template <concepts::unregistered_type_p...Args>
	cache &set(std::string_view topic, Args&&...args)
		requires (sizeof...(Args) > 0);

	template <concepts::topic_type...Args>
	cache &set(Args&&...args)
		requires (sizeof...(Args) > 0);

public:
	template <concepts::topic_type T>
	[[nodiscard]] optional<T> get() const;

	template <typename T>
	[[nodiscard]] optional<T> get(std::string_view topic) const;
	[[nodiscard]] payload_t get(std::string_view topic) const;

	[[nodiscard]] std::map<std::string,payload_t> get() const noexcept;

public:
	[[nodiscard]] signal_t<payload_t,payload_t> &changed(std::string_view topic) noexcept;
	[[nodiscard]] signal_t<std::string_view,payload_t,payload_t> &changed() noexcept;

	template <concepts::topic_type T>
	[[nodiscard]] signal_t<payload_t,payload_t> &changed() noexcept;

public:
	template <typename T = payload_t>
	struct changed_result
	{
		using type = T;
		T current {};
		T previous {};
	};

	template <typename Token, typename T = payload_t>
	static constexpr bool is_token_v =
		libgs::concepts::tf_opt_token<Token,sys_expected<changed_result<T>>> and
		not is_detached_v<Token>;

	template <concepts::topic_type T, typename Token = use_sync_t>
	auto wait_changed(Token &&token = use_sync) noexcept
		requires is_token_v<Token,optional<T>>;

	template <typename Token = use_sync_t>
	auto wait_changed(std::string_view topic, Token &&token = use_sync) noexcept
		requires is_token_v<Token>;

	template <typename T, typename Token = use_sync_t>
	auto wait_changed(std::string_view topic, Token &&token = use_sync) noexcept
		requires is_token_v<Token,T>;

public:
	[[nodiscard]] subscriber_t subscriber() noexcept;
	[[nodiscard]] executor_t get_executor() noexcept;

private:
	class impl;
	std::shared_ptr<impl> m_impl {};
};

using local_cache = cache<local_subscriber>;

} //namespace libgs::utils::sbus
#include <libgs/utils/sbus/detail/cache.h>


#endif //LIBGS_UTILS_UTILS_SBUS_CACHE_H