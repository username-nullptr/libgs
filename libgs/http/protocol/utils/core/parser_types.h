// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H
#define LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H

#include <libgs/http/protocol/types.h>
#include <libgs/http/utils/file_opt_token.h>

namespace libgs::http
{

template <protocol_model>
class parser {};

#define LIBGS_HTTP_PARSER_ERRNO \
X_MACRO( RLTL  , 10000 , "Request line too long."      ) \
X_MACRO( HLTL  , 10001 , "Header line too long."       ) \
X_MACRO( IREQL , 10002 , "Invalid request line."       ) \
X_MACRO( IRPYL , 10003 , "Invalid reply line."         ) \
X_MACRO( IHM   , 10004 , "Invalid http method."        ) \
X_MACRO( IHP   , 10005 , "Invalid http path."          ) \
X_MACRO( IHSC  , 10006 , "Invalid http status code."   ) \
X_MACRO( IHL   , 10007 , "Invalid header line."        ) \
X_MACRO( ICL   , 10008 , "Invalid cookie line."        ) \
X_MACRO( IDE   , 10009 , "The inserted data is empty." ) \
X_MACRO( SFE   , 10010 , "Size format error."          ) \
X_MACRO( RE    , 10011 , "This request is ended."      )

enum class parse_errno
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_PARSER_ERRNO
#undef X_MACRO
};

enum class stage {
	header, body, finished
};

} //namespace libgs::http


#endif //LIBGS_HTTP_PROTOCOL_UTILS_CORE_PARSER_TYPES_H
