// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_UTILS_OPT_TOKEN_H
#define LIBGS_HTTP_UTILS_OPT_TOKEN_H

#include <libgs/http/utils/file_opt_token.h>

namespace libgs::http
{

template <core_concepts::character CharT>
struct LIBGS_HTTP_TAPI basic_path_opt_token
{
	using char_t = CharT;
	using string_view_t = std::basic_string_view<char_t>;
	std::vector<string_view_t> paths;

	template <core_concepts::string_p<CharT> Str>
	basic_path_opt_token(Str &&path);

	template <core_concepts::string<CharT> Str>
	basic_path_opt_token(std::vector<Str> &&path_values);

	template <core_concepts::string<CharT> Str>
	basic_path_opt_token(std::initializer_list<Str> path_values);

	template <core_concepts::string_p<CharT>...Str>
	basic_path_opt_token(Str&&...path_args);
};

using path_opt_token  = basic_path_opt_token<char>;
using wpath_opt_token = basic_path_opt_token<wchar_t>;

template <typename...Args>
using callback_t = std::function<void(Args...)>;

} //namespace libgs::http::operators
#include <libgs/http/utils/detail/opt_token.h>


#endif //LIBGS_HTTP_UTILS_OPT_TOKEN_H
