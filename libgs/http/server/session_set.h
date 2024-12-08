
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_HTTP_SERVER_SESSION_SET_H
#define LIBGS_HTTP_SERVER_SESSION_SET_H

#include <libgs/http/server/session.h>
#include <libgs/core/shared_mutex.h>

namespace libgs::http
{

template <typename Session, typename CharT>
concept base_of_session = std::is_base_of_v<basic_session<CharT>, Session>;

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_session_set
{
	LIBGS_DISABLE_COPY(basic_session_set)

public:
	using char_t = CharT;
	using session_t = basic_session<char_t>;
	using string_t = std::basic_string_view<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

public:
	basic_session_set();
	~basic_session_set();

	basic_session_set(basic_session_set &&other) noexcept;
	basic_session_set &operator=(basic_session_set &&other) noexcept;

public:
	template <base_of_session<char_t> Session, typename...Args>
	[[nodiscard]] std::shared_ptr<Session> make(Args&&...args) noexcept
		requires core_concepts::constructible<Session, Args...>;

	template <typename...Args>
	[[nodiscard]] std::shared_ptr<session_t> make(Args&&...args) noexcept
		requires core_concepts::constructible<basic_session<char_t>, Args...>;

	template <base_of_session<char_t> Session, typename...Args>
	[[nodiscard]] std::shared_ptr<Session> get_or_make(string_view_t id, Args&&...args)
		requires core_concepts::constructible<Session, Args...>;

	template <typename...Args>
	[[nodiscard]] std::shared_ptr<session_t> get_or_make(string_view_t id, Args&&...args) noexcept
		requires core_concepts::constructible<basic_session<char_t>, Args...>;

public:
	template <base_of_session<char_t> Session>
	[[nodiscard]] std::shared_ptr<Session> get(string_view_t id);
	[[nodiscard]] std::shared_ptr<session_t> get(string_view_t id);

	template <base_of_session<char_t> Session>
	[[nodiscard]] std::shared_ptr<Session> get_or(string_view_t id);
	[[nodiscard]] std::shared_ptr<session_t> get_or(string_view_t id) noexcept;

public:
	template <typename Rep, typename Period>
	basic_session_set &set_lifecycle(const duration<Rep,Period> &seconds);
	[[nodiscard]] std::chrono::seconds lifecycle() const noexcept;

	basic_session_set &set_cookie_key(string_view_t key);
	[[nodiscard]] string_view_t cookie_key() noexcept;

private:
	class impl;
	impl *m_impl;
};

using session_set = basic_session_set<char>;
using wsession_set = basic_session_set<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/session_set.h>


#endif //LIBGS_HTTP_SERVER_SESSION_SET_H