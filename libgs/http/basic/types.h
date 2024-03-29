
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

enum class status
{
	continue_request                = 100, // Continue
	switching_protocols             = 101, // Switching Protocols
	processing                      = 102, // Processing
	ok                              = 200, // OK
	created                         = 201, // Created
	accepted                        = 202, // Accepted
	non_authoritative_information   = 203, // Non-Authoritative Information
	no_content                      = 204, // No Content
	reset_content                   = 205, // Reset Content
	partial_content                 = 206, // Partial Content
	multi_status                    = 207, // Multi-Status
	already_reported                = 208, // Already Reported
	im_used                         = 226, // IM Used
	multiple_choices                = 300, // Multiple Choices
	moved_permanently               = 301, // Moved Permanently
	found                           = 302, // Found
	see_other                       = 303, // See Other
	not_modified                    = 304, // Not Modified
	use_proxy                       = 305, // Use Proxy
	temporary_redirect              = 307, // Temporary Redirect
	permanent_redirect              = 308, // Permanent Redirect
	bad_request                     = 400, // Bad Request
	unauthorized                    = 401, // Unauthorized
	payment_required                = 402, // Payment Required
	forbidden                       = 403, // Forbidden
	not_found                       = 404, // Not Found
	method_not_allowed              = 405, // Method Not Allowed
	not_acceptable                  = 406, // Not Acceptable
	proxy_authentication_required   = 407, // Proxy Authentication Required
	request_timeout                 = 408, // Request Timeout
	conflict                        = 409, // Conflict
	gone                            = 410, // Gone
	length_required                 = 411, // Length Required
	precondition_failed             = 412, // Precondition Failed
	payload_too_large               = 413, // Payload Too Large
	uri_too_long                    = 414, // URI Too Long
	unsupported_media_type          = 415, // Unsupported Media Type
	range_not_satisfiable           = 416, // Range Not Satisfiable
	expectation_failed              = 417, // Expectation Failed
	misdirected_request             = 421, // Misdirected Request
	unprocessable_entity            = 422, // Unprocessable Entity
	locked                          = 423, // Locked
	failed_dependency               = 424, // Failed Dependency
	upgrade_required                = 426, // Upgrade Required
	precondition_required           = 428, // Precondition Required
	tooMany_requests                = 429, // Too Many Requests
	request_header_fields_too_large = 431, // Request Header Fields Too Large
	unavailable_for_legal_reasons   = 451, // Unavailable For Legal Reasons
	internal_server_error           = 500, // Internal Server Error
	not_implemented                 = 501, // Not Implemented
	bad_gateway                     = 502, // Bad Gateway
	service_unavailable             = 503, // Service Unavailable
	gateway_timeout                 = 504, // Gateway Timeout
	http_version_not_supported      = 505, // HTTP Version Not Supported
	variant_also_negotiates         = 506, // Variant Also Negotiates
	insufficient_storage            = 507, // Insufficient Storage
	loop_detected                   = 508, // Loop Detected
	not_extended                    = 510, // Not Extended
	network_authentication_required = 511, // Network Authentication Required
};

enum class method
{
	GET     = 0x01,
	PUT     = 0x02,
	POST    = 0x04,
	HEAD    = 0x08,
	DELETE  = 0x10,
	OPTIONS = 0x20,
	CONNECT = 0x40,
	TRACH   = 0x80
};

enum class redirect_type
{
	moved_permanently  = static_cast<int>(status::moved_permanently ),
	permanent_redirect = static_cast<int>(status::permanent_redirect),
	found              = static_cast<int>(status::found             ),
	see_other          = static_cast<int>(status::see_other         ),
	temporary_redirect = static_cast<int>(status::temporary_redirect),
	multiple_choices   = static_cast<int>(status::multiple_choices  ),
	not_modified       = static_cast<int>(status::not_modified      )
};

template <status>
struct status_description
#ifndef _MSC_VER // _MSVC
{ static_assert(false, "Invalid http status."); }
#endif //_MSC_VER
;
#define LIBGS_HTTP_STATUS_DESCRIPTION(_s, _d) \
	template <> struct status_description<_s> { \
		static constexpr const char *value = _d; \
	}
LIBGS_HTTP_STATUS_DESCRIPTION(status::continue_request               , "Continue"                       );
LIBGS_HTTP_STATUS_DESCRIPTION(status::switching_protocols            , "Switching Protocols"            );
LIBGS_HTTP_STATUS_DESCRIPTION(status::processing                     , "Processing"                     );
LIBGS_HTTP_STATUS_DESCRIPTION(status::ok                             , "OK"                             );
LIBGS_HTTP_STATUS_DESCRIPTION(status::created                        , "Created"                        );
LIBGS_HTTP_STATUS_DESCRIPTION(status::accepted                       , "Accepted"                       );
LIBGS_HTTP_STATUS_DESCRIPTION(status::non_authoritative_information  , "Non-Authoritative Information"  );
LIBGS_HTTP_STATUS_DESCRIPTION(status::no_content                     , "No Content"                     );
LIBGS_HTTP_STATUS_DESCRIPTION(status::reset_content                  , "Reset Content"                  );
LIBGS_HTTP_STATUS_DESCRIPTION(status::partial_content                , "Partial Content"                );
LIBGS_HTTP_STATUS_DESCRIPTION(status::multi_status                   , "Multi-Status"                   );
LIBGS_HTTP_STATUS_DESCRIPTION(status::already_reported               , "Already Reported"               );
LIBGS_HTTP_STATUS_DESCRIPTION(status::im_used                        , "IM Used"                        );
LIBGS_HTTP_STATUS_DESCRIPTION(status::multiple_choices               , "Multiple Choices"               );
LIBGS_HTTP_STATUS_DESCRIPTION(status::moved_permanently              , "Moved Permanently"              );
LIBGS_HTTP_STATUS_DESCRIPTION(status::found                          , "Found"                          );
LIBGS_HTTP_STATUS_DESCRIPTION(status::see_other                      , "See Other"                      );
LIBGS_HTTP_STATUS_DESCRIPTION(status::not_modified                   , "Not Modified"                   );
LIBGS_HTTP_STATUS_DESCRIPTION(status::use_proxy                      , "Use Proxy"                      );
LIBGS_HTTP_STATUS_DESCRIPTION(status::temporary_redirect             , "Temporary Redirect"             );
LIBGS_HTTP_STATUS_DESCRIPTION(status::permanent_redirect             , "Permanent Redirect"             );
LIBGS_HTTP_STATUS_DESCRIPTION(status::bad_request                    , "Bad Request"                    );
LIBGS_HTTP_STATUS_DESCRIPTION(status::unauthorized                   , "Unauthorized"                   );
LIBGS_HTTP_STATUS_DESCRIPTION(status::payment_required               , "Payment Required"               );
LIBGS_HTTP_STATUS_DESCRIPTION(status::forbidden                      , "Forbidden"                      );
LIBGS_HTTP_STATUS_DESCRIPTION(status::not_found                      , "Not Found"                      );
LIBGS_HTTP_STATUS_DESCRIPTION(status::method_not_allowed             , "Method Not Allowed"             );
LIBGS_HTTP_STATUS_DESCRIPTION(status::not_acceptable                 , "Not Acceptable"                 );
LIBGS_HTTP_STATUS_DESCRIPTION(status::proxy_authentication_required  , "Proxy Authentication Required"  );
LIBGS_HTTP_STATUS_DESCRIPTION(status::request_timeout                , "Request Timeout"                );
LIBGS_HTTP_STATUS_DESCRIPTION(status::conflict                       , "Conflict"                       );
LIBGS_HTTP_STATUS_DESCRIPTION(status::gone                           , "Gone"                           );
LIBGS_HTTP_STATUS_DESCRIPTION(status::length_required                , "Length Required"                );
LIBGS_HTTP_STATUS_DESCRIPTION(status::precondition_failed            , "Precondition Failed"            );
LIBGS_HTTP_STATUS_DESCRIPTION(status::payload_too_large              , "Payload Too Large"              );
LIBGS_HTTP_STATUS_DESCRIPTION(status::uri_too_long                   , "URI Too Long"                   );
LIBGS_HTTP_STATUS_DESCRIPTION(status::unsupported_media_type         , "Unsupported Media Type"         );
LIBGS_HTTP_STATUS_DESCRIPTION(status::range_not_satisfiable          , "Range Not Satisfiable"          );
LIBGS_HTTP_STATUS_DESCRIPTION(status::expectation_failed             , "Expectation Failed"             );
LIBGS_HTTP_STATUS_DESCRIPTION(status::misdirected_request            , "Misdirected Request"            );
LIBGS_HTTP_STATUS_DESCRIPTION(status::unprocessable_entity           , "Unprocessable Entity"           );
LIBGS_HTTP_STATUS_DESCRIPTION(status::locked                         , "Locked"                         );
LIBGS_HTTP_STATUS_DESCRIPTION(status::failed_dependency              , "Failed Dependency"              );
LIBGS_HTTP_STATUS_DESCRIPTION(status::upgrade_required               , "Upgrade Required"               );
LIBGS_HTTP_STATUS_DESCRIPTION(status::precondition_required          , "Precondition Required"          );
LIBGS_HTTP_STATUS_DESCRIPTION(status::tooMany_requests               , "Too Many Requests"              );
LIBGS_HTTP_STATUS_DESCRIPTION(status::request_header_fields_too_large, "Request Header Fields Too Large");
LIBGS_HTTP_STATUS_DESCRIPTION(status::unavailable_for_legal_reasons  , "Unavailable For Legal Reasons"  );
LIBGS_HTTP_STATUS_DESCRIPTION(status::internal_server_error          , "Internal Server Error"          );
LIBGS_HTTP_STATUS_DESCRIPTION(status::not_implemented                , "Not Implemented"                );
LIBGS_HTTP_STATUS_DESCRIPTION(status::bad_gateway                    , "Bad Gateway"                    );
LIBGS_HTTP_STATUS_DESCRIPTION(status::service_unavailable            , "Service Unavailable"            );
LIBGS_HTTP_STATUS_DESCRIPTION(status::gateway_timeout                , "Gateway Timeout"                );
LIBGS_HTTP_STATUS_DESCRIPTION(status::http_version_not_supported     , "HTTP Version Not Supported"     );
LIBGS_HTTP_STATUS_DESCRIPTION(status::variant_also_negotiates        , "Variant Also Negotiates"        );
LIBGS_HTTP_STATUS_DESCRIPTION(status::insufficient_storage           , "Insufficient Storage"           );
LIBGS_HTTP_STATUS_DESCRIPTION(status::loop_detected                  , "Loop Detected"                  );
LIBGS_HTTP_STATUS_DESCRIPTION(status::not_extended                   , "Not Extended"                   );
LIBGS_HTTP_STATUS_DESCRIPTION(status::network_authentication_required, "Network Authentication Required");

template <int S>
constexpr const char *status_description_v = status_description<static_cast<status>(S)>::value;

template <concept_char_type CharT = char>
std::basic_string<CharT> to_status_description(status s);

template <method>
struct method_string 
#ifndef _MSC_VER // _MSVC
{ static_assert(false, "Invalid http method."); }
#endif //_MSC_VER
;
#define LIBGS_HTTP_METHOD_MAPPING(_m, _s) \
	template <> struct method_string<_m> { \
		static constexpr const char *value = _s; \
	}
LIBGS_HTTP_METHOD_MAPPING(method::GET    , "GET"    );
LIBGS_HTTP_METHOD_MAPPING(method::PUT    , "PUT"    );
LIBGS_HTTP_METHOD_MAPPING(method::POST   , "POST"   );
LIBGS_HTTP_METHOD_MAPPING(method::HEAD   , "HEAD"   );
LIBGS_HTTP_METHOD_MAPPING(method::DELETE , "DELETE" );
LIBGS_HTTP_METHOD_MAPPING(method::OPTIONS, "OPTIONS");
LIBGS_HTTP_METHOD_MAPPING(method::CONNECT, "CONNECT");
LIBGS_HTTP_METHOD_MAPPING(method::TRACH  , "TRACH"  );

template <method M>
constexpr const char *method_string_v = method_string<M>::value;

template <concept_char_type CharT = char>
LIBGS_HTTP_TAPI std::basic_string<CharT> to_method_string(method m);

LIBGS_HTTP_API method from_method_string(std::string_view str);
LIBGS_HTTP_API method from_method_string(std::wstring_view str);

template <concept_char_type CharT>
using basic_parameters = std::map<std::basic_string<CharT>, basic_value<CharT>, less_case_insensitive>;

using parameters = basic_parameters<char>;
using wparameters = basic_parameters<wchar_t>;

template <concept_char_type CharT>
struct LIBGS_HTTP_TAPI basic_datagram
{
	using str_type = std::basic_string<CharT>;
	using headers_type = basic_headers<CharT>;

	str_type version;
	headers_type headers;
	std::string partial_body;
};

} //namespace libgs::http
#include <libgs/http/basic/detail/types.h>


#endif //LIBGS_HTTP_BASIC_TYPES_H
