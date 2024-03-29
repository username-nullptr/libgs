
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

#include <libgs/http/container.h>
#include <libgs/core/string_list.h>
#include <libgs/core/algorithm/uuid.h>
#include <libgs/core/shared_mutex.h>
#include <set>

namespace libgs::http
{

template <concept_char_type CharT>
using basic_session_attributes = std::unordered_map<std::basic_string<CharT>, std::any>;

using session_attributes = basic_session_attributes<char>;
using wsession_attributes = basic_session_attributes<wchar_t>;

template <concept_char_type CharT>
class LIBGS_HTTP_TAPI basic_session
{
	LIBGS_DISABLE_COPY_MOVE(basic_session)

public:
	using duration = std::chrono::seconds;
	using str_type = std::basic_string<CharT>;
	using value_type = basic_value<CharT>;
	using attributes_type = basic_session_attributes<CharT>;
	using ptr = std::shared_ptr<basic_session>;

public:
	explicit basic_session(const duration &seconds = {});
	virtual ~basic_session();

public:
	[[nodiscard]] str_type id() const;
	[[nodiscard]] uint64_t create_time() const;
	[[nodiscard]] bool is_valid() const;

public:
	[[nodiscard]] std::any attribute(const str_type &key) const;
	[[nodiscard]] std::any attribute_or(const str_type &key, std::any default_value = {}) const noexcept;

	template <typename T>
	[[nodiscard]] T attribute(const str_type &key)
		requires is_dsame_v<T, std::any>;

	template <typename T>
	[[nodiscard]] T attribute_or(const str_type &key, T default_value = {}) const noexcept
		requires is_dsame_v<T, std::any>;

public:
	basic_session &set_attribute(str_type key, const std::any &v);
	basic_session &set_attribute(str_type key, std::any &&v);
	basic_session &unset_attribute(const str_type &key);

public:
	[[nodiscard]] size_t attribute_count() const;
	[[nodiscard]] session_attributes attributes() const;
	[[nodiscard]] string_list attribute_key_list() const;
	[[nodiscard]] std::set<str_type> attribute_key_set() const;

public:
	[[nodiscard]] uint64_t lifecycle() const;
	basic_session &set_lifecycle(const duration &seconds = {});
	basic_session &expand(const duration &seconds = {});
	void invalidate();

public:
	[[nodiscard]] static ptr make_shared(const duration &seconds = {});
	[[nodiscard]] static ptr get(const str_type &id);
	static void set(basic_session *obj);

	template <class Sesn>
	[[nodiscard]] static std::shared_ptr<Sesn> get(const std::string &id)
		requires std::is_base_of_v<basic_session, Sesn>;

public:
	static void set_global_lifecycle(const duration &seconds);
	[[nodiscard]] static duration global_lifecycle();

protected:
	class impl;
	impl *m_impl;
};

using session = basic_session<char>;
using wsession = basic_session<wchar_t>;

template <concept_char_type CharT>
using basic_session_ptr = typename basic_session<CharT>::ptr;

using session_ptr = basic_session_ptr<char>;
using wsession_ptr = basic_session_ptr<wchar_t>;

template <class Sesn = session>
std::shared_ptr<Sesn> make_session(const std::chrono::seconds &seconds = {})
	requires std::is_base_of_v<session,Sesn> or std::is_base_of_v<wsession,Sesn>;

} //namespace libgs::http
#include <libgs/http/context/detail/session.h>


#endif //LIBGS_HTTP_CONTEXT_SESSION_H
