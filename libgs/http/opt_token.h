
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
	static constexpr bool value =
		std::is_same_v<Token,error_code&> or
		std::is_same_v<std::remove_cvref_t<Token>,use_sync_type>;
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

template <typename>
struct LIBGS_HTTP_TAPI is_fstream : std::false_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_fstream<std::basic_fstream<CharT>> : std::true_type {};

template <typename T>
constexpr bool is_fstream_v = is_fstream<T>::value;

template <typename>
struct LIBGS_HTTP_TAPI is_ofstream : std::false_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_ofstream<std::basic_ofstream<CharT>> : std::true_type {};

template <typename T>
constexpr bool is_ofstream_v = is_ofstream<T>::value;

template <typename>
struct LIBGS_HTTP_TAPI is_ifstream : std::false_type {};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI is_ifstream<std::basic_ifstream<CharT>> : std::true_type {};

template <typename T>
constexpr bool is_ifstream_v = is_ifstream<T>::value;

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
concept fstream = is_fstream_v<T> or is_ofstream_v<T> or is_ifstream_v<T>;

template <typename T>
concept fstream_wkn = fstream<std::remove_reference_t<T>>;

} //namespace concepts

template <typename...Args>
using callback_t = std::function<void(Args...)>;

struct LIBGS_HTTP_VAPI file_range
{
	using pos_t = std::streamoff;
	pos_t begin = 0, total = 0;
};
using file_ranges = std::list<file_range>;

namespace io_permission
{
using type = size_t; constexpr type
	read  = 0x01,
	write = 0x02,
	read_write = read | write;
}

template <concepts::fstream>
struct get_io_permissions;

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI get_io_permissions<std::basic_fstream<CharT>> {
	static constexpr auto value = io_permission::read_write;
};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI get_io_permissions<std::basic_ofstream<CharT>> {
	static constexpr auto value = io_permission::write;
};

template <core_concepts::char_type CharT>
struct LIBGS_HTTP_TAPI get_io_permissions<std::basic_ifstream<CharT>> {
	static constexpr auto value = io_permission::read;
};

template <concepts::fstream FS>
constexpr auto io_permissions_v = get_io_permissions<FS>::value;

namespace file_optype
{
using type = size_t; constexpr type
	single   = 0x01,
	multiple = 0x02,
	combine  = single | multiple;
}

template <concepts::fstream_wkn FS>
struct file_opt_base
{
	using pos_t = file_range::pos_t;
	using fstream_t = std::remove_cvref_t<FS>;

	static constexpr auto permissions = io_permissions_v<fstream_t>;
	static constexpr auto optype = file_optype::single;
	error_code error;
};

template <concepts::fstream_wkn, file_optype::type>
struct file_opt;

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI file_opt<FS,file_optype::single> : file_opt_base<FS>
{
	using fstream_t = typename file_opt_base<FS>::fstream_t;
	std::shared_ptr<fstream_t> stream;
	file_range range;
	~file_opt();

	file_opt(fstream_t &&stream);
	file_opt(fstream_t &&stream, const file_range &range);

	file_opt(std::string_view file_name);
	file_opt(std::string_view file_name, const file_range &range);

	file_opt<FS,file_optype::multiple> operator| (const file_range &range);
	file_opt<FS,file_optype::multiple> operator| (file_ranges ranges);

	file_opt(file_opt&&) = default;
	file_opt(const file_opt&) = default;
	file_opt &operator=(file_opt&&) = default;
	file_opt &operator=(const file_opt&) = default;
};

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI file_opt<FS&,file_optype::single> : file_opt_base<FS&>
{
	using fstream_t = typename file_opt_base<FS&>::fstream_t;
	fstream_t *stream = nullptr;
	file_range range;

	file_opt(fstream_t &stream);
	file_opt(fstream_t &stream, const file_range &range);

	file_opt<FS&,file_optype::multiple> operator| (const file_range &range);
	file_opt<FS&,file_optype::multiple> operator| (file_ranges ranges);

	file_opt(file_opt&&) = default;
	file_opt(const file_opt&) = default;
	file_opt &operator=(file_opt&&) = default;
	file_opt &operator=(const file_opt&) = default;
};

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI file_opt<FS,file_optype::multiple> : file_opt_base<FS>
{
	using fstream_t = typename file_opt_base<FS>::fstream_t;
	std::shared_ptr<fstream_t> stream;
	file_ranges ranges;
	~file_opt();

	file_opt(fstream_t &&stream);
	file_opt(fstream_t &&stream, const file_range &range);
	file_opt(fstream_t &&stream, file_ranges ranges);

	template <typename...Args>
	file_opt(fstream_t &&stream, Args&&...ranges) requires
		requires { file_ranges{std::forward<Args>(ranges)...}; };

	file_opt(std::string_view file_name);
	file_opt(std::string_view file_name, const file_range &range);
	file_opt(std::string_view file_name, file_ranges ranges);

	template <typename...Args>
	file_opt(std::string_view file_name, Args&&...ranges) requires
		requires { file_ranges{std::forward<Args>(ranges)...}; };

	file_opt(file_opt<FS,file_optype::single> &opt, const file_range &range);
	file_opt(file_opt<FS,file_optype::single> &opt, file_ranges ranges);

	file_opt &operator| (const file_range &range) &;
	file_opt &operator| (file_ranges ranges) &;

	file_opt &&operator| (const file_range &range) &&;
	file_opt &&operator| (file_ranges ranges) &&;

	file_opt(file_opt&&) = default;
	file_opt(const file_opt&) = default;
	file_opt &operator=(file_opt&&) = default;
	file_opt &operator=(const file_opt&) = default;
};

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI file_opt<FS&,file_optype::multiple> : file_opt_base<FS&>
{
	using fstream_t = typename file_opt_base<FS&>::fstream_t;
	fstream_t *stream = nullptr;
	file_ranges ranges;

	file_opt(fstream_t &stream);
	file_opt(fstream_t &stream, const file_range &range);
	file_opt(fstream_t &stream, file_ranges ranges);

	template <typename...Args>
	file_opt(fstream_t &stream, Args&&...ranges) requires
		requires { file_ranges{std::forward<Args>(ranges)...}; };

	file_opt(file_opt<FS&,file_optype::single> &opt, const file_range &range);
	file_opt(file_opt<FS&,file_optype::single> &opt, file_ranges ranges);

	file_opt &operator| (const file_range &range);
	file_opt &operator| (file_ranges ranges);

	file_opt(file_opt&&) = default;
	file_opt(const file_opt&) = default;
	file_opt &operator=(file_opt&&) = default;
	file_opt &operator=(const file_opt&) = default;
};

template <typename...Args>
[[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt(std::string_view file_name, Args&&...args) noexcept;

template <typename...Args>
[[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt(concepts::fstream_wkn auto &&stream, Args&&...args) noexcept;

template <typename>
struct LIBGS_HTTP_TAPI is_file_opt : std::false_type {};

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI is_file_opt<file_opt<FS, file_optype::single>> : std::true_type {};

template <concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI is_file_opt<file_opt<FS, file_optype::multiple>> : std::true_type {};

template <typename T>
constexpr bool is_file_opt_v = is_file_opt<T>::value;

namespace concepts
{

template <
	typename T,
	file_optype::type Types = file_optype::combine,
	io_permission::type Perms = io_permission::read_write
>
concept file_opt =
	is_file_opt_v<std::remove_cvref_t<T>> and
	!!(std::remove_cvref_t<T>::optype & Types) and
	!!(std::remove_cvref_t<T>::permissions & Perms);

template <
	typename T,
	file_optype::type Types = file_optype::combine,
	io_permission::type Perms = io_permission::read_write
>
concept file_opt_param =
	core_concepts::char_string_type<T> or
	!!(io_permissions_v<std::remove_cvref_t<T>> & Perms) or
	file_opt<T,Types,Perms>;

} //namespace concepts

[[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt(concepts::file_opt auto &&opt) noexcept;

namespace operators
{

[[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, const file_range &range);
[[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, file_ranges ranges);

[[nodiscard]] LIBGS_HTTP_TAPI auto operator| (concepts::fstream_wkn auto &&stream, const file_range &range);
[[nodiscard]] LIBGS_HTTP_TAPI auto operator| (concepts::fstream_wkn auto &&stream, file_ranges ranges);

}} //namespace libgs::http::operators
#include <libgs/http/detail/opt_token.h>


#endif //LIBGS_HTTP_OPT_TOKEN_H
