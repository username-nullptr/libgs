
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

#ifndef LIBGS_HTTP_SERVER_DETAIL_SESSION_BASE_H
#define LIBGS_HTTP_SERVER_DETAIL_SESSION_BASE_H

#include <libgs/http/global.h>
#include <libgs/core/shared_mutex.h>
#include <map>

namespace libgs::http
{

template <concept_char_type CharT>
class basic_session;

namespace detail
{

class session_duration_base
{
	LIBGS_DISABLE_COPY_MOVE(session_duration_base)

public:
	template<typename Rep, typename Period = std::ratio<1>>
	using duration = std::chrono::duration<Rep,Period>;

public:
	[[nodiscard]] static std::chrono::seconds global_lifecycle() noexcept;

	template <typename Rep, typename Period = std::ratio<1>>
	static void set_global_lifecycle(const duration<Rep,Period> &seconds) noexcept
	{
		namespace sc = std::chrono;
		g_lifecycle = sc::duration_cast<sc::seconds>(seconds).count();

		if( g_lifecycle == 0 )
			g_lifecycle = 1;
	}

private:
	static std::atomic<uint64_t> g_lifecycle;
};

template <concept_char_type CharT>
class session_base;

template <>
class LIBGS_HTTP_API session_base<char> : public session_duration_base
{
	LIBGS_DISABLE_COPY_MOVE(session_base)
	using ptr = std::shared_ptr<basic_session<char>>;
	friend class basic_session<char>;

private: //
	session_base() = default;
	static std::map<std::string_view, ptr> session_map;
	static shared_mutex rwlock;
};

template <> class LIBGS_HTTP_API session_base<wchar_t> : public session_duration_base
{
	LIBGS_DISABLE_COPY_MOVE(session_base)
	using ptr = std::shared_ptr<basic_session<wchar_t>>;
	friend class basic_session<wchar_t>;

private: //
	session_base() = default;
	static std::map<std::wstring_view, ptr> session_map;
	static shared_mutex rwlock;
};

}} //namespace libgs::http::detail


#endif //LIBGS_HTTP_SERVER_DETAIL_SESSION_BASE_H

