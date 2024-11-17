
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

#ifndef LIBGS_HTTP_OPT_TOKEN_H
#define LIBGS_HTTP_OPT_TOKEN_H

#include <libgs/http/types.h>
#include <fstream>

namespace libgs::http
{

template <typename Token, typename...Args>
struct LIBGS_HTTP_TAPI is_async_token {
	static constexpr bool value = asio::completion_token_for<Token,void(Args...)>;
};

template <typename Token, typename...Args>
constexpr bool is_async_token_v = is_async_token<Token,Args...>::value;

struct LIBGS_HTTP_VAPI use_sync_type {};
constexpr use_sync_type use_sync = use_sync_type();

template <typename Token = use_sync_type>
struct LIBGS_HTTP_TAPI is_sync_token {
	static constexpr bool value = std::is_same_v<Token,error_code&> or std::is_same_v<Token,use_sync_type>;
};

template <typename Token = use_sync_type>
constexpr bool is_sync_token_v = is_sync_token<Token>::value;

template <typename Token, typename...Args>
struct LIBGS_HTTP_TAPI is_token {
	static constexpr bool value = is_async_token_v<Token,Args...> or is_sync_token_v<Token>;
};

template <typename Token, typename...Args>
constexpr bool is_token_v = is_token<Token, Args...>::value;

template <typename Token = use_sync_type>
struct LIBGS_HTTP_TAPI is_dis_func_token {
	static constexpr bool value = is_token_v<Token,error_code> and not is_function_v<Token>;
};

template <typename Token>
constexpr bool is_dis_func_token_v = is_dis_func_token<Token>::value;

template <typename T>
struct LIBGS_HTTP_TAPI is_fiostream : std::false_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_fiostream<std::basic_fstream<CharT>> : std::true_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_fiostream<std::basic_ifstream<CharT>> : std::true_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_fiostream<std::basic_ofstream<CharT>> : std::true_type {};

template <typename T>
constexpr bool is_fiostream_v = is_fiostream<T>::value;

namespace concepts
{

template <typename Token, typename...Args>
concept async_token = is_async_token_v<Token,Args...>;

template <typename Token, typename...Args>
concept sync_token = is_sync_token_v<Token>;

template <typename Token, typename...Args>
concept token = is_token_v<Token,Args...>;

template <typename Token>
concept dis_func_token = is_dis_func_token_v<Token>;

template <typename T>
concept fiostream = is_fiostream_v<std::remove_reference_t<T>>;

} //namespace concepts

template <typename...Args>
using callback_t = std::function<void(Args...)>;

struct LIBGS_HTTP_VAPI file_range
{
	using pos_t = std::streamoff;
	pos_t begin = 0, total = 0;
};
using file_ranges = std::list<file_range>;

template <typename CT = void>
struct LIBGS_HTTP_VAPI file_opt_base
{
	file_ranges ranges;
	file_opt_base() = default;
	file_opt_base(const file_range &range);
	file_opt_base(file_ranges ranges);

	using cc_t = crtp_derived_t<CT,file_opt_base>;
	cc_t &operator<<(file_range range);
	cc_t &operator<<(file_ranges ranges);
};

template <typename FS>
struct LIBGS_HTTP_VAPI file_opt;

template <typename T = void>
struct LIBGS_HTTP_VAPI file_opt : file_opt_base<file_opt<T>>
{
	using pos_t = file_range::pos_t;
	using fstream_t = std::fstream;
	std::string file_name;
	fstream_t stream;

	file_opt(std::string_view file_name);
	file_opt(std::string_view file_name, const file_range &range);
	file_opt(std::string_view file_name, file_ranges ranges);
};

template <concepts::fiostream FS> requires std::is_lvalue_reference_v<FS>
struct LIBGS_HTTP_TAPI file_opt<FS> : file_opt_base<file_opt<FS>>
{
	using pos_t = file_range::pos_t;
	using fstream_t = FS;
	fstream_t *stream = nullptr;

	file_opt(fstream_t stream);
	file_opt(fstream_t stream, const file_range &range);
	file_opt(fstream_t stream, file_ranges ranges);
};

template <concepts::fiostream FS> requires std::is_rvalue_reference_v<FS>
struct LIBGS_HTTP_TAPI file_opt<FS> : file_opt_base<file_opt<FS>>
{
	using pos_t = file_range::pos_t;
	using fstream_t = FS;
	fstream_t stream;

	file_opt(fstream_t stream);
	file_opt(fstream_t stream, const file_range &range);
	file_opt(fstream_t stream, file_ranges ranges);
};

template <typename...Args>
[[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt(Args&&...args);

template <typename T>
struct LIBGS_HTTP_TAPI is_file_range_opt : std::false_type {};

template <typename FS>
struct LIBGS_HTTP_TAPI is_file_range_opt<file_opt<FS>> : std::true_type {};

template <typename T>
constexpr bool is_file_range_opt_v = is_file_range_opt<T>::value;

template <typename T>
struct LIBGS_HTTP_TAPI is_file_baisc_opt {
	static constexpr bool value = is_char_string_v<T>;
};

template <typename T>
constexpr bool is_file_baisc_opt_v = is_file_baisc_opt<T>::value;

template <typename T>
struct LIBGS_HTTP_TAPI is_file_opt : std::disjunction<is_file_range_opt<T>, is_file_baisc_opt<T>> {};

template <typename T>
constexpr bool is_file_opt_v = is_file_opt<T>::value;

namespace concepts
{

template <typename T>
concept file_opt = is_file_opt_v<T>;

} //namespace concepts

namespace operators
{

[[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, const file_range &range);
[[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, file_ranges ranges);

[[nodiscard]] LIBGS_HTTP_TAPI auto operator| (concepts::fiostream auto &&stream, const file_range &range);
[[nodiscard]] LIBGS_HTTP_TAPI auto operator| (concepts::fiostream auto &&stream, file_ranges ranges);

}} //namespace libgs::http::operators
#include <libgs/http/detail/opt_token.h>

namespace libgs::http::operators
{

template <concepts::token Token = use_sync_type>
void www(const concepts::file_opt auto &opt, Token &&token = use_sync)
{

}

inline void ttt()
{
	www("aaa");

	std::string fn0 = "fn0";
	www(fn0);

	std::string_view fn1 = "fn1";
	www(fn1);

	file_range range {1,1};

	www("aaa" | range);
	www(make_file_opt("aaa", range));

	file_ranges ranges {{1,1}, {2,2}};

	www("aaa" | std::move(ranges));
	www(make_file_opt("aaa", std::move(ranges)));

	std::fstream file0;

	www(file0 | range);
	www(make_file_opt(file0, range));

	std::fstream file1;

	www(std::move(file1) | range);
	www(make_file_opt(std::move(file1), range));
}

} //namespace libgs::http::operators


#endif //LIBGS_HTTP_OPT_TOKEN_H
