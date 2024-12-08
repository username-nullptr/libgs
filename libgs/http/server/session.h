
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

#ifndef LIBGS_HTTP_SERVER_SESSION_H
#define LIBGS_HTTP_SERVER_SESSION_H

#include <libgs/http/types.h>

namespace libgs::http
{

template <core_concepts::char_type CharT>
using basic_session_attributes = std::map<std::basic_string<CharT>, std::any>;

using session_attributes = basic_session_attributes<char>;
using wsession_attributes = basic_session_attributes<wchar_t>;

template <core_concepts::char_type CharT>
class LIBGS_HTTP_TAPI basic_session :
	public std::enable_shared_from_this<basic_session<CharT>>
{
	LIBGS_DISABLE_COPY_MOVE(basic_session)

public:
	using char_t = CharT;
	using executor_t = asio::any_io_executor;

	template <typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::duration<Rep,Period>;
	using time_point = decltype(std::chrono::system_clock::now());

	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;
	using value_t = basic_value<char_t>;
	using attributes_t = basic_session_attributes<char_t>;

public:
	template <typename Rep, typename Period = std::ratio<1>>
	explicit basic_session(const duration<Rep,Period> &seconds, const executor_t &exec = execution::get_executor());

	explicit basic_session(const executor_t &exec = execution::get_executor());
	virtual ~basic_session();

public:
	[[nodiscard]] string_view_t id() const noexcept;
	[[nodiscard]] time_point create_time() const noexcept;
	[[nodiscard]] bool is_valid() const noexcept;

public:
	[[nodiscard]] std::any attribute(string_view_t key) const;
	[[nodiscard]] std::any attribute_or(string_view_t key, std::any default_value = {}) const noexcept;
	[[nodiscard]] const attributes_t &attributes() const noexcept;

public:
	basic_session &set_attribute(string_view_t key, const std::any &value);
	basic_session &set_attribute(string_view_t key, std::any &&value);
	basic_session &unset_attribute(string_view_t key);

public:
	[[nodiscard]] std::chrono::seconds lifecycle() const noexcept;
	void invalidate();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &set_lifecycle(const duration<Rep,Period> &seconds);

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &expand(const duration<Rep,Period> &seconds);
	basic_session &expand();

public:
	template <core_concepts::callable Func>
	basic_session &on_timeout(Func &&func);
	basic_session &unbind_timeout();

private:
	class impl;
	impl *m_impl;
};

using session = basic_session<char>;
using wsession = basic_session<wchar_t>;

template <core_concepts::char_type CharT>
using basic_session_ptr = std::shared_ptr<basic_session<CharT>>;

using session_ptr = basic_session_ptr<char>;
using wsession_ptr = basic_session_ptr<wchar_t>;

} //namespace libgs::http
#include <libgs/http/server/detail/session.h>


#endif //LIBGS_HTTP_SERVER_SESSION_H
