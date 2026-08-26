
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2026 Xiaoqiang <username_nullptr@163.com>                    *
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

#ifndef LIBGS_HTTP_SERVER_SESSION_MANAGER_H
#define LIBGS_HTTP_SERVER_SESSION_MANAGER_H

#include <libgs/http/server/session.h>
#include <libgs/core/shared_mutex.h>

namespace libgs::http
{

class LIBGS_HTTP_VAPI session_manager
{
	LIBGS_DISABLE_COPY(session_manager)

public:
	session_manager();
	~session_manager();

	session_manager(session_manager &&other) noexcept;
	session_manager &operator=(session_manager &&other) noexcept;

public: // Fucking msvc !!!
	template <typename Session, typename...Args>
	[[nodiscard]] std::shared_ptr<Session> make(Args&&...args) requires
		core_concepts::base_of<Session,session> and core_concepts::constructible<Session,Args...>;

	template <typename...Args>
	[[nodiscard]] std::shared_ptr<session> make(Args&&...args) requires
		core_concepts::constructible<session,Args...>;

	template <typename Session, typename...Args>
	[[nodiscard]] std::shared_ptr<Session> get_or_make (
		const core_concepts::text_p<char> auto &id, Args&&...args
	) requires
		core_concepts::base_of<Session,session> and
		core_concepts::constructible<Session,Args...>;

	template <typename...Args>
	[[nodiscard]] std::shared_ptr<session> get_or_make (
		const core_concepts::text_p<char> auto &id, Args&&...args
	) requires
		core_concepts::constructible<session,Args...>;

public: // Fucking msvc !!!
	template <typename Session>
	[[nodiscard]] std::shared_ptr<Session> get(const core_concepts::text_p<char> auto &id) requires
		core_concepts::base_of<Session,session>;

	template <typename Session>
	[[nodiscard]] std::shared_ptr<Session> get_or(const core_concepts::text_p<char> auto &id) requires
		core_concepts::base_of<Session,session>;

	[[nodiscard]] std::shared_ptr<session> get(const core_concepts::text_p<char> auto &id);
	[[nodiscard]] std::shared_ptr<session> get_or(const core_concepts::text_p<char> auto &id) noexcept;

public:
	template <typename Rep, typename Period>
	session_manager &set_lifecycle(const duration<Rep,Period> &seconds);
	[[nodiscard]] std::chrono::seconds lifecycle() const noexcept;

	session_manager &set_cookie_key(core_concepts::text_p<char> auto &&key);
	[[nodiscard]] std::string_view cookie_key() const noexcept;

public:
	template <core_concepts::callable<session_ptr,error_code> Func>
	session_manager &on_error(Func &&func);
	session_manager &unbind_error();

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::http
#include <libgs/http/server/detail/session_manager.h>


#endif //LIBGS_HTTP_SERVER_SESSION_MANAGER_H
