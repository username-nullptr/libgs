
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

#ifndef LIBGS_HTTP_CXX_FILE_OPT_TOKEN_H
#define LIBGS_HTTP_CXX_FILE_OPT_TOKEN_H

#include <libgs/http/global.h>
#include <fstream>

namespace libgs::http
{

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

template <core_concepts::fstream>
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

template <core_concepts::fstream FS>
constexpr auto io_permissions_v = get_io_permissions<FS>::value;

namespace file_optype
{
using type = size_t; constexpr type
	single   = 0x01,
	multiple = 0x02,
	combine  = single | multiple;
}

template <typename>
struct file_opt_token_base;

template <core_concepts::fstream_wkn FS>
struct LIBGS_HTTP_TAPI file_opt_token_base<FS>
{
	using pos_t = file_range::pos_t;
	using fstream_t = std::remove_cvref_t<FS>;

	static constexpr auto permissions = io_permissions_v<fstream_t>;
	static constexpr auto optype = file_optype::single;
	size_t size = 0;

protected:
	void set_size(auto &stream);
};

template <>
struct LIBGS_HTTP_TAPI file_opt_token_base<void> : file_opt_token_base<std::fstream> {};

template <typename, file_optype::type>
struct file_opt_token;

template <typename, file_optype::type>
struct file_opt_token_range;

template <typename T>
struct LIBGS_HTTP_TAPI file_opt_token_range<T,file_optype::single> : file_opt_token_base<T>
{
	file_range range;
	explicit file_opt_token_range(const file_range &range);
};

template <typename T>
struct LIBGS_HTTP_TAPI file_opt_token_range<T,file_optype::multiple> : file_opt_token_base<T>
{
	file_ranges ranges;
	explicit file_opt_token_range(const file_range &range);
	explicit file_opt_token_range(file_ranges ranges);

	file_opt_token<T,file_optype::multiple> &operator| (const file_range &range) &;
	file_opt_token<T,file_optype::multiple> &operator| (file_ranges ranges) &;

	file_opt_token<T,file_optype::multiple> &&operator| (const file_range &range) &&;
	file_opt_token<T,file_optype::multiple> &&operator| (file_ranges ranges) &&;
};

template <>
struct LIBGS_HTTP_VAPI file_opt_token<void,file_optype::single> :
	file_opt_token_range<void,file_optype::single>
{
	std::shared_ptr<fstream_t> stream {new fstream_t()};
	std::string file_name;
	~file_opt_token();

	file_opt_token(std::string_view file_name);
	file_opt_token(std::string_view file_name, const file_range &range);

	file_opt_token<void,file_optype::multiple> operator| (const file_range &range);
	file_opt_token<void,file_optype::multiple> operator| (file_ranges ranges);

	[[nodiscard]] error_code init(int flags) noexcept;
	[[nodiscard]] std::string mime_type();

	file_opt_token(file_opt_token&&) = default;
	file_opt_token(const file_opt_token&) = default;
	file_opt_token &operator=(file_opt_token&&) = default;
	file_opt_token &operator=(const file_opt_token&) = default;
};

// template <core_concepts::fstream_wkn FS>
// struct LIBGS_HTTP_TAPI file_opt_token<FS&&,file_optype::single> :
// 	file_opt_token_range<FS&&,file_optype::single>
// {
// 	using fstream_t = typename file_opt_token_range<FS&&,file_optype::single>::fstream_t;
// 	std::shared_ptr<fstream_t> stream;
// 	~file_opt_token();
//
// 	file_opt_token(fstream_t &&stream);
// 	file_opt_token(fstream_t &&stream, const file_range &range);
//
// 	file_opt_token<FS&&,file_optype::multiple> operator| (const file_range &range);
// 	file_opt_token<FS&&,file_optype::multiple> operator| (file_ranges ranges);
//
// 	[[nodiscard]] error_code init(int flags) noexcept;
// 	[[nodiscard]] std::string mime_type();
//
// 	file_opt_token(file_opt_token&&) = default;
// 	file_opt_token(const file_opt_token&) = default;
// 	file_opt_token &operator=(file_opt_token&&) = default;
// 	file_opt_token &operator=(const file_opt_token&) = default;
// };
//
// template <core_concepts::fstream_wkn FS>
// struct LIBGS_HTTP_TAPI file_opt_token<FS&,file_optype::single> :
// 	file_opt_token_range<FS&,file_optype::single>
// {
// 	using fstream_t = typename file_opt_token_range<FS&,file_optype::single>::fstream_t;
// 	fstream_t *stream = nullptr;
//
// 	file_opt_token(fstream_t &stream);
// 	file_opt_token(fstream_t &stream, const file_range &range);
//
// 	file_opt_token<FS&,file_optype::multiple> operator| (const file_range &range);
// 	file_opt_token<FS&,file_optype::multiple> operator| (file_ranges ranges);
//
// 	[[nodiscard]] error_code init(int flags) noexcept;
// 	[[nodiscard]] std::string mime_type();
//
// 	file_opt_token(file_opt_token&&) = default;
// 	file_opt_token(const file_opt_token&) = default;
// 	file_opt_token &operator=(file_opt_token&&) = default;
// 	file_opt_token &operator=(const file_opt_token&) = default;
// };

template <>
struct LIBGS_HTTP_VAPI file_opt_token<void,file_optype::multiple> :
	file_opt_token_range<void,file_optype::multiple>
{
	std::shared_ptr<fstream_t> stream;
	std::string file_name;
	~file_opt_token();

	file_opt_token(std::string_view file_name);
	file_opt_token(std::string_view file_name, const file_range &range);
	file_opt_token(std::string_view file_name, file_ranges ranges);

	template <typename...Args>
	file_opt_token(std::string_view file_name, Args&&...ranges) requires
		requires { file_ranges{std::forward<Args>(ranges)...}; };

	file_opt_token(file_opt_token<void,file_optype::single> &opt, const file_range &range);
	file_opt_token(file_opt_token<void,file_optype::single> &opt, file_ranges ranges);

	[[nodiscard]] error_code init(int flags) noexcept;
	[[nodiscard]] std::string mime_type();

	file_opt_token(file_opt_token&&) = default;
	file_opt_token(const file_opt_token&) = default;
	file_opt_token &operator=(file_opt_token&&) = default;
	file_opt_token &operator=(const file_opt_token&) = default;
};

// template <core_concepts::fstream_wkn FS>
// struct LIBGS_HTTP_TAPI file_opt_token<FS&&,file_optype::multiple> :
// 	file_opt_token_range<FS&&,file_optype::multiple>
// {
// 	using fstream_t = typename file_opt_token_range<FS&&,file_optype::multiple>::fstream_t;
// 	std::shared_ptr<fstream_t> stream;
// 	~file_opt_token();
//
// 	file_opt_token(fstream_t &&stream);
// 	file_opt_token(fstream_t &&stream, const file_range &range);
// 	file_opt_token(fstream_t &&stream, file_ranges ranges);
//
// 	template <typename...Args>
// 	file_opt_token(fstream_t &&stream, Args&&...ranges) requires
// 		requires { file_ranges{std::forward<Args>(ranges)...}; };
//
// 	file_opt_token(file_opt_token<FS&&,file_optype::single> &opt, const file_range &range);
// 	file_opt_token(file_opt_token<FS&&,file_optype::single> &opt, file_ranges ranges);
//
// 	[[nodiscard]] error_code init(int flags) noexcept;
// 	[[nodiscard]] std::string mime_type();
//
// 	file_opt_token(file_opt_token&&) = default;
// 	file_opt_token(const file_opt_token&) = default;
// 	file_opt_token &operator=(file_opt_token&&) = default;
// 	file_opt_token &operator=(const file_opt_token&) = default;
// };
//
// template <core_concepts::fstream_wkn FS>
// struct LIBGS_HTTP_TAPI file_opt_token<FS&,file_optype::multiple> :
// 	file_opt_token_range<FS&,file_optype::multiple>
// {
// 	using fstream_t = typename file_opt_token_range<FS&,file_optype::multiple>::fstream_t;
// 	fstream_t *stream = nullptr;
//
// 	file_opt_token(fstream_t &stream);
// 	file_opt_token(fstream_t &stream, const file_range &range);
// 	file_opt_token(fstream_t &stream, file_ranges ranges);
//
// 	template <typename...Args>
// 	file_opt_token(fstream_t &stream, Args&&...ranges) requires
// 		requires { file_ranges{std::forward<Args>(ranges)...}; };
//
// 	file_opt_token(file_opt_token<FS&,file_optype::single> &opt, const file_range &range);
// 	file_opt_token(file_opt_token<FS&,file_optype::single> &opt, file_ranges ranges);
//
// 	[[nodiscard]] error_code init(int flags) noexcept;
// 	[[nodiscard]] std::string mime_type();
//
// 	file_opt_token(file_opt_token&&) = default;
// 	file_opt_token(const file_opt_token&) = default;
// 	file_opt_token &operator=(file_opt_token&&) = default;
// 	file_opt_token &operator=(const file_opt_token&) = default;
// };
//
// template <typename...Args>
// [[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt_token(
// 	std::string_view file_name, Args&&...args
// ) noexcept;
//
// template <typename...Args>
// [[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt_token(
// 	core_concepts::fstream_wkn auto &&stream, Args&&...args
// ) noexcept;
//
// template <core_concepts::char_type, typename>
// struct LIBGS_HTTP_TAPI is_basic_file_opt_token : std::false_type {};
//
// template <core_concepts::char_type CharT, typename T>
// struct LIBGS_HTTP_TAPI is_basic_file_opt_token<CharT,file_opt_token<T,file_optype::single>>
// {
// 	static constexpr bool value =
// 		std::is_same_v<CharT, typename file_opt_token<T,file_optype::single>::fstream_t::char_type>;
// };
//
// template <core_concepts::char_type CharT, typename T>
// struct LIBGS_HTTP_TAPI is_basic_file_opt_token<CharT,file_opt_token<T,file_optype::multiple>>
// {
// 	static constexpr bool value =
// 		std::is_same_v<CharT, typename file_opt_token<T,file_optype::single>::fstream_t::char_type>;
// };
//
// template <core_concepts::char_type CharT, typename T>
// constexpr bool is_basic_file_opt_token_v = is_basic_file_opt_token<CharT,T>::value;
//
// template <typename T>
// using is_char_file_opt_token = is_basic_file_opt_token<char,T>;
//
// template <typename T>
// constexpr bool is_char_file_opt_token_v = is_char_file_opt_token<T>::value;
//
// template <typename T>
// using is_wchar_file_opt_token = is_basic_file_opt_token<wchar_t,T>;
//
// template <typename T>
// constexpr bool is_wchar_file_opt_token_v = is_wchar_file_opt_token<T>::value;
//
// template <typename T>
// struct LIBGS_HTTP_TAPI is_file_opt_token : std::disjunction<is_char_file_opt_token<T>, is_wchar_file_opt_token<T>> {};
//
// template <typename T>
// constexpr bool is_file_opt_token_v = is_file_opt_token<T>::value;
//
// namespace concepts
// {
//
// template <typename T, typename CharT,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept basic_file_opt_token =
// 	is_basic_file_opt_token_v<CharT,std::remove_cvref_t<T>> and
// 	!!(std::remove_cvref_t<T>::optype & Types) and
// 	!!(std::remove_cvref_t<T>::permissions & Perms);
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept char_file_opt_token = basic_file_opt_token<T,char,Types,Perms>;
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept wchar_file_opt_token = basic_file_opt_token<T,wchar_t,Types,Perms>;
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept file_opt_token =
// 	char_file_opt_token<T,Types,Perms> or
// 	wchar_file_opt_token<T,Types,Perms>;
//
// template <typename T, typename CharT,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept basic_file_opt_token_param =
// 	core_concepts::char_string_type<T> or
// 	!!(io_permissions_v<std::remove_cvref_t<T>> & Perms) or
// 	basic_file_opt_token<T,CharT,Types,Perms>;
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept char_file_opt_token_param = basic_file_opt_token_param<T,char,Types,Perms>;
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept wchar_file_opt_token_param = basic_file_opt_token_param<T,wchar_t,Types,Perms>;
//
// template <typename T,
// 	file_optype::type Types = file_optype::combine,
// 	io_permission::type Perms = io_permission::read_write
// >
// concept file_opt_token_param =
// 	char_file_opt_token_param<T,Types,Perms> or
// 	wchar_file_opt_token_param<T,Types,Perms>;
//
// } //namespace concepts
//
// [[nodiscard]] LIBGS_HTTP_VAPI auto make_file_opt_token(
// 	concepts::file_opt_token auto &&opt
// ) noexcept;

namespace operators
{

// [[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, const file_range &range);
// [[nodiscard]] LIBGS_HTTP_VAPI auto operator| (std::string_view file_name, file_ranges ranges);

// [[nodiscard]] LIBGS_HTTP_TAPI auto operator| (core_concepts::fstream_wkn auto &&stream, const file_range &range);
// [[nodiscard]] LIBGS_HTTP_TAPI auto operator| (core_concepts::fstream_wkn auto &&stream, file_ranges ranges);

}} //namespace libgs::http::operators
#include <libgs/http/cxx/detail/file_opt_token.h>


#endif //LIBGS_HTTP_CXX_FILE_OPT_TOKEN_H