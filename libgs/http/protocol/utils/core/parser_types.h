// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H

#include <libgs/http/protocol/types.h>
#include <libgs/http/utils/file_opt_token.h>

namespace libgs::http
{

constexpr size_t default_parser_buffer_size = 4 * 1024;

template <protocol_model>
class parser {};

enum class stage {
	header, body, finished
};

#define LIBGS_HTTP_PARSE_ERRC_TABLE \
X_MACRO( req_line_too_long    , 10000 , "Request line too long."      ) \
X_MACRO( header_line_too_long , 10001 , "Header line too long."       ) \
X_MACRO( invalid_req_line     , 10002 , "Invalid request line."       ) \
X_MACRO( invalid_reply_line   , 10003 , "Invalid reply line."         ) \
X_MACRO( invalid_method       , 10004 , "Invalid http method."        ) \
X_MACRO( invalid_path         , 10005 , "Invalid http path."          ) \
X_MACRO( invalid_status_code  , 10006 , "Invalid http status code."   ) \
X_MACRO( invalid_header_line  , 10007 , "Invalid header line."        ) \
X_MACRO( invalid_cookie_line  , 10008 , "Invalid cookie line."        ) \
X_MACRO( inserted_data_empty  , 10009 , "The inserted data is empty." ) \
X_MACRO( invalid_size_format  , 10010 , "Size format error."          ) \
X_MACRO( request_end          , 10011 , "This request is ended."      )

enum class parse_errc
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_PARSE_ERRC_TABLE
#undef X_MACRO
};

[[nodiscard]] LIBGS_HTTP_API
const error_category_t &parse_error_category() noexcept;

[[nodiscard]] LIBGS_HTTP_API
error_code make_error_code(parse_errc value) noexcept;

} //namespace libgs::http

namespace std
{

template <>
struct is_error_code_enum<libgs::http::parse_errc> : true_type {};

} //namespace std


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H
