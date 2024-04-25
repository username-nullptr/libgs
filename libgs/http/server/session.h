
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

#include <libgs/http/basic/types.h>
#include <libgs/http/server/detail/session_base.h>

namespace libgs::http
{

template <concept_char_type CharT>
using basic_session_attributes = std::map<std::basic_string<CharT>, std::any>;

using session_attributes = basic_session_attributes<char>;
using wsession_attributes = basic_session_attributes<wchar_t>;

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_session : protected detail::session_base<CharT>
{
	LIBGS_DISABLE_COPY_MOVE(basic_session)

public:
	template<typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::duration<Rep,Period>;
	using time_point = decltype(std::chrono::system_clock::now());

	using str_type = std::basic_string<CharT>;
	using str_view_type = std::basic_string_view<CharT>;
	using value_type = basic_value<CharT>;

	using attributes_type = basic_session_attributes<CharT>;
	using ptr = std::shared_ptr<basic_session>;

	template <typename Session>	requires std::is_base_of_v<basic_session,Session>
	using dt_ptr = std::shared_ptr<Session>;

public:
	template <typename Rep, typename Period = std::ratio<1>>
	explicit basic_session(const duration<Rep,Period> &seconds);

	basic_session();
	virtual ~basic_session();

public:
	[[nodiscard]] str_view_type id() const noexcept;
	[[nodiscard]] time_point create_time() const noexcept;
	[[nodiscard]] bool is_valid() const noexcept;

public:
	[[nodiscard]] std::any attribute(str_view_type key) const;
	[[nodiscard]] std::any attribute_or(str_view_type key, std::any default_value = {}) const noexcept;
	[[nodiscard]] const attributes_type &attributes() const noexcept;

public:
	basic_session &set_attribute(str_view_type key, const std::any &value);
	basic_session &set_attribute(str_view_type key, std::any &&value);
	basic_session &unset_attribute(str_view_type key);

public:
	[[nodiscard]] std::chrono::seconds lifecycle() const noexcept;
	void invalidate();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &set_lifecycle(const duration<Rep,Period> &seconds);
	basic_session &set_lifecycle();

	template <typename Rep, typename Period = std::ratio<1>>
	basic_session &expand(const duration<Rep,Period> &seconds = {});
	basic_session &expand();

public:
	template <typename Session, typename...Args>
	static dt_ptr<Session> make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	static ptr make(Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename Session, typename...Args>
	static dt_ptr<Session> get(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	static ptr get(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename Session, typename...Args>
	static dt_ptr<Session> get_or_make(const std::string &id, Args&&...args)
		requires requires{ session_type(std::forward<Args>(args)...); };

	template <typename...Args>
	static ptr get_or_make(const std::string &id, Args&&...args) noexcept
		requires requires{ session_type(std::forward<Args>(args)...); };

private:
	class impl;
	impl *m_impl;
};

using session = basic_session<char>;
using wsession = basic_session<wchar_t>;

template <concept_char_type CharT>
using basic_session_ptr = typename basic_session<CharT>::ptr;

using session_ptr = basic_session_ptr<char>;
using wsession_ptr = basic_session_ptr<wchar_t>;

} //namespace libgs::http
//#include <libgs/http/context/detail/session.h>


#include <libgs/core/algorithm/uuid.h>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session<CharT>::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	const str_type m_id = basic_uuid<CharT>::generate();
	std::atomic<uint64_t> m_second;
	std::atomic_bool m_valid = false;
};

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT>::basic_session(const duration<Rep,Period> &seconds) :
	m_impl(new impl())
{
	m_impl->m_second = seconds.count();
}

template <concept_char_type CharT>
basic_session<CharT>::basic_session() :
	m_impl(new impl())
{
	m_impl->m_second = this->global_lifecycle().count();
}

template <concept_char_type CharT>
basic_session<CharT>::~basic_session()
{
	delete m_impl;
}

template <concept_char_type CharT>
std::basic_string_view<CharT> basic_session<CharT>::id() const noexcept
{

}

template <concept_char_type CharT>
typename basic_session<CharT>::time_point basic_session<CharT>::create_time() const noexcept
{

}

template <concept_char_type CharT>
bool basic_session<CharT>::is_valid() const noexcept
{

}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute(str_view_type key) const
{

}

template <concept_char_type CharT>
std::any basic_session<CharT>::attribute_or(str_view_type key, std::any default_value) const noexcept
{

}

template <concept_char_type CharT>
const basic_session_attributes<CharT> &basic_session<CharT>::attributes() const noexcept
{

}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_view_type key, const std::any &value)
{

}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_attribute(str_view_type key, std::any &&value)
{

}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::unset_attribute(str_view_type key)
{

}

template <concept_char_type CharT>
std::chrono::seconds basic_session<CharT>::lifecycle() const noexcept
{

}

template <concept_char_type CharT>
void basic_session<CharT>::invalidate()
{

}

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT> &basic_session<CharT>::set_lifecycle(const duration<Rep,Period> &seconds)
{

}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::set_lifecycle()
{

}

template <concept_char_type CharT>
template <typename Rep, typename Period>
basic_session<CharT> &basic_session<CharT>::expand(const duration<Rep,Period> &seconds)
{

}

template <concept_char_type CharT>
basic_session<CharT> &basic_session<CharT>::expand()
{

}






} //namespace libgs::http


#endif //LIBGS_HTTP_SERVER_SESSION_H
