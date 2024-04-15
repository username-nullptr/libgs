
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

#ifndef LIBGS_HTTP_BASIC_TYPES_H
#define LIBGS_HTTP_BASIC_TYPES_H

#include <libgs/http/basic/header.h>
#include <libgs/http/basic/cookie.h>

#if defined(__WINNT) || defined(_WINDOWS)
# undef DELETE
#endif //_WINDOWS

namespace libgs::http
{

#define LIBGS_HTTP_STATUS_TABLE \
X_MACRO( continue_request                , 100 , "Continue"                        ) \
X_MACRO( switching_protocols             , 101 , "Switching Protocols"             ) \
X_MACRO( processing                      , 102 , "Processing"                      ) \
X_MACRO( ok                              , 200 , "OK"                              ) \
X_MACRO( created                         , 201 , "Created"                         ) \
X_MACRO( accepted                        , 202 , "Accepted"                        ) \
X_MACRO( non_authoritative_information   , 203 , "Non-Authoritative Information"   ) \
X_MACRO( no_content                      , 204 , "No Content"                      ) \
X_MACRO( reset_content                   , 205 , "Reset Content"                   ) \
X_MACRO( partial_content                 , 206 , "Partial Content"                 ) \
X_MACRO( multi_status                    , 207 , "Multi-Status"                    ) \
X_MACRO( already_reported                , 208 , "Already Reported"                ) \
X_MACRO( im_used                         , 226 , "IM Used"                         ) \
X_MACRO( multiple_choices                , 300 , "Multiple Choices"                ) \
X_MACRO( moved_permanently               , 301 , "Moved Permanently"               ) \
X_MACRO( found                           , 302 , "Found"                           ) \
X_MACRO( see_other                       , 303 , "See Other"                       ) \
X_MACRO( not_modified                    , 304 , "Not Modified"                    ) \
X_MACRO( use_proxy                       , 305 , "Use Proxy"                       ) \
X_MACRO( temporary_redirect              , 307 , "Temporary Redirect"              ) \
X_MACRO( permanent_redirect              , 308 , "Permanent Redirect"              ) \
X_MACRO( bad_request                     , 400 , "Bad Request"                     ) \
X_MACRO( unauthorized                    , 401 , "Unauthorized"                    ) \
X_MACRO( payment_required                , 402 , "Payment Required"                ) \
X_MACRO( forbidden                       , 403 , "Forbidden"                       ) \
X_MACRO( not_found                       , 404 , "Not Found"                       ) \
X_MACRO( method_not_allowed              , 405 , "Method Not Allowed"              ) \
X_MACRO( not_acceptable                  , 406 , "Not Acceptable"                  ) \
X_MACRO( proxy_authentication_required   , 407 , "Proxy Authentication Required"   ) \
X_MACRO( request_timeout                 , 408 , "Request Timeout"                 ) \
X_MACRO( conflict                        , 409 , "Conflict"                        ) \
X_MACRO( gone                            , 410 , "Gone"                            ) \
X_MACRO( length_required                 , 411 , "Length Required"                 ) \
X_MACRO( precondition_failed             , 412 , "Precondition Failed"             ) \
X_MACRO( payload_too_large               , 413 , "Payload Too Large"               ) \
X_MACRO( uri_too_long                    , 414 , "URI Too Long"                    ) \
X_MACRO( unsupported_media_type          , 415 , "Unsupported Media Type"          ) \
X_MACRO( range_not_satisfiable           , 416 , "Range Not Satisfiable"           ) \
X_MACRO( expectation_failed              , 417 , "Expectation Failed"              ) \
X_MACRO( misdirected_request             , 421 , "Misdirected Request"             ) \
X_MACRO( unprocessable_entity            , 422 , "Unprocessable Entity"            ) \
X_MACRO( locked                          , 423 , "Locked"                          ) \
X_MACRO( failed_dependency               , 424 , "Failed Dependency"               ) \
X_MACRO( upgrade_required                , 426 , "Upgrade Required"                ) \
X_MACRO( precondition_required           , 428 , "Precondition Required"           ) \
X_MACRO( tooMany_requests                , 429 , "Too Many Requests"               ) \
X_MACRO( request_header_fields_too_large , 431 , "Request Header Fields Too Large" ) \
X_MACRO( unavailable_for_legal_reasons   , 451 , "Unavailable For Legal Reasons"   ) \
X_MACRO( internal_server_error           , 500 , "Internal Server Error"           ) \
X_MACRO( not_implemented                 , 501 , "Not Implemented"                 ) \
X_MACRO( bad_gateway                     , 502 , "Bad Gateway"                     ) \
X_MACRO( service_unavailable             , 503 , "Service Unavailable"             ) \
X_MACRO( gateway_timeout                 , 504 , "Gateway Timeout"                 ) \
X_MACRO( http_version_not_supported      , 505 , "HTTP Version Not Supported"      ) \
X_MACRO( variant_also_negotiates         , 506 , "Variant Also Negotiates"         ) \
X_MACRO( insufficient_storage            , 507 , "Insufficient Storage"            ) \
X_MACRO( loop_detected                   , 508 , "Loop Detected"                   ) \
X_MACRO( not_extended                    , 510 , "Not Extended"                    ) \
X_MACRO( network_authentication_required , 511 , "Network Authentication Required" )

enum class status
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
};
LIBGS_HTTP_VAPI void status_check(uint32_t s);
LIBGS_HTTP_VAPI void status_check(status s);

#define LIBGS_HTTP_METHOD_TABLE \
X_MACRO( GET     , 0x01 , "GET"     ) \
X_MACRO( PUT     , 0x02 , "PUT"     ) \
X_MACRO( POST    , 0x04 , "POST"    ) \
X_MACRO( HEAD    , 0x08 , "HEAD"    ) \
X_MACRO( DELETE  , 0x10 , "DELETE"  ) \
X_MACRO( OPTIONS , 0x20 , "OPTIONS" ) \
X_MACRO( CONNECT , 0x40 , "CONNECT" ) \
X_MACRO( TRACH   , 0x80 , "TRACH"   )

enum method
{
#define X_MACRO(e,v,d) e=(v),
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
};
LIBGS_HTTP_VAPI void method_check(uint32_t m);
LIBGS_HTTP_VAPI void method_check(method m);

#define LIBGS_HTTP_REDIRECT_TYPE_TABLE \
X_MACRO( moved_permanently  , static_cast<int>(status::moved_permanently ) ) \
X_MACRO( permanent_redirect , static_cast<int>(status::permanent_redirect) ) \
X_MACRO( found              , static_cast<int>(status::found             ) ) \
X_MACRO( see_other          , static_cast<int>(status::see_other         ) ) \
X_MACRO( temporary_redirect , static_cast<int>(status::temporary_redirect) ) \
X_MACRO( multiple_choices   , static_cast<int>(status::multiple_choices  ) ) \
X_MACRO( not_modified       , static_cast<int>(status::not_modified      ) )

enum class redirect_type
{
#define X_MACRO(e,v) e=(v),
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
};
LIBGS_HTTP_VAPI void redirect_type_check(uint32_t type);
LIBGS_HTTP_VAPI void redirect_type_check(redirect_type type);

template <status>
struct status_description
#ifndef _MSC_VER // _MSVC
{ static_assert(false, "Invalid http status."); }
#endif //_MSC_VER
;
#define X_MACRO(e,v,d) \
	template <> struct status_description<status::e> { \
		static constexpr const char *value = d; \
	};
LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO

template <status S>
constexpr const char *status_description_v = status_description<S>::value;

template <concept_char_type CharT = char>
std::basic_string<CharT> to_status_description(status s);

template <method>
struct method_string 
#ifndef _MSC_VER // _MSVC
{ static_assert(false, "Invalid http method."); }
#endif //_MSC_VER
;
#define X_MACRO(e,v,d) \
	template <> struct method_string<method::e> { \
		static constexpr const char *value = d; \
	};
LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO

template <method M>
constexpr const char *method_string_v = method_string<M>::value;

template <concept_char_type CharT = char>
LIBGS_HTTP_TAPI std::basic_string<CharT> to_method_string(method m);

LIBGS_HTTP_VAPI method from_method_string(std::string_view str);
LIBGS_HTTP_VAPI method from_method_string(std::wstring_view str);

template <concept_char_type CharT>
using basic_parameters = std::map<std::basic_string<CharT>, basic_value<CharT>, less_case_insensitive>;

using parameters = basic_parameters<char>;
using wparameters = basic_parameters<wchar_t>;

} //namespace libgs::http
#include <libgs/http/basic/detail/types.h>


#endif //LIBGS_HTTP_BASIC_TYPES_H
