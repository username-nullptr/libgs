
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

#ifndef LIBGS_HTTP_CONTEXT_SESSION_H
#define LIBGS_HTTP_CONTEXT_SESSION_H

#include <libgs/http/basic/types.h>

namespace libgs::http
{

template <concept_char_type CharT>
using basic_session_attributes = std::map<std::basic_string<CharT>, std::any>;

using session_attributes = basic_session_attributes<char>;
using wsession_attributes = basic_session_attributes<wchar_t>;

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_session
{
	LIBGS_DISABLE_COPY_MOVE(basic_session)

public:
	template<typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::duration<Rep,Period>;
	using time_point = decltype(std::chrono::system_clock::now());

	using str_type = std::basic_string<CharT>;
	using value_type = basic_value<CharT>;
	using attributes_type = basic_session_attributes<CharT>;

public:
	template <typename Rep, typename Period = std::ratio<1>>
	explicit basic_session(const duration<Rep,Period> &seconds);

	basic_session();
	virtual ~basic_session();

public:
	[[nodiscard]] str_type id() const;
	[[nodiscard]] time_point create_time() const;
	[[nodiscard]] bool is_valid() const;

public:
	[[nodiscard]] std::any attribute(const str_type &key) const;
	[[nodiscard]] std::any attribute_or(const str_type &key, std::any default_value = {}) const noexcept;
	[[nodiscard]] const attributes_type &attributes() const;

public:
	basic_session &set_attribute(str_type key, const std::any &value);
	basic_session &set_attribute(str_type key, std::any &&value);
	basic_session &unset_attribute(const str_type &key);

public:
	[[nodiscard]] std::chrono::seconds lifecycle() const;
	void invalidate();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &set_lifecycle(const duration<Rep,Period> &seconds);
	basic_session &set_lifecycle();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &expand(const duration<Rep,Period> &seconds = {});
	basic_session &expand();

public:
	template <typename Rep, typename Period = std::ratio<1>>
	static void set_global_lifecycle(const duration<Rep,Period> &seconds);
	[[nodiscard]] static std::chrono::seconds global_lifecycle();

protected:
	class impl;
	impl *m_impl;
};

using session = basic_session<char>;
using wsession = basic_session<wchar_t>;

template <concept_char_type CharT>
using basic_session_ptr = std::shared_ptr<basic_session<CharT>>;

using session_ptr = basic_session_ptr<char>;
using wsession_ptr = basic_session_ptr<wchar_t>;

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_session_pool
{
	LIBGS_DISABLE_COPY(basic_session_pool)

public:
	template<typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::seconds;

	using session_type = basic_session<CharT>;
	using session_ptr_type = basic_session_ptr<CharT>;

	template <typename Session>	requires std::is_base_of_v<session_type,Session>
	using c_session_ptr = std::shared_ptr<Session>;

public:
	basic_session_pool();
	~basic_session_pool();

	basic_session_pool(basic_session_pool &&other);
	basic_session_pool &operator=(basic_session_pool &&other);

public:
	template <typename Session, typename...Args>
	c_session_ptr<Session> make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	session_ptr_type make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename Session, typename...Args>
	c_session_ptr<Session> get(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	session_ptr_type get(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename Session, typename...Args>
	c_session_ptr<Session> get_or_make(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	session_ptr_type get_or_make(const std::string &id, Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };
};

using session_pool = basic_session_pool<char>;
using wsession_pool = basic_session_pool<wchar_t>;

} //namespace libgs::http
//#include <libgs/http/context/detail/session.h>


#endif //LIBGS_HTTP_CONTEXT_SESSION_H
