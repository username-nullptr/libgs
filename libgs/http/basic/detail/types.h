
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

#ifndef LIBGS_HTTP_BASIC_DETAIL_TYPES_H
#define LIBGS_HTTP_BASIC_DETAIL_TYPES_H

namespace libgs::http
{

template <concept_char_type CharT>
std::basic_string<CharT> to_status_description(status s)
{
	switch(s)
	{
	case status::continue_request               : return "Continue"                       ;
	case status::switching_protocols            : return "Switching Protocols"            ;
	case status::processing                     : return "Processing"                     ;
	case status::ok                             : return "OK"                             ;
	case status::created                        : return "Created"                        ;
	case status::accepted                       : return "Accepted"                       ;
	case status::non_authoritative_information  : return "Non-Authoritative Information"  ;
	case status::no_content                     : return "No Content"                     ;
	case status::reset_content                  : return "Reset Content"                  ;
	case status::partial_content                : return "Partial Content"                ;
	case status::multi_status                   : return "Multi-Status"                   ;
	case status::already_reported               : return "Already Reported"               ;
	case status::im_used                        : return "IM Used"                        ;
	case status::multiple_choices               : return "Multiple Choices"               ;
	case status::moved_permanently              : return "Moved Permanently"              ;
	case status::found                          : return "Found"                          ;
	case status::see_other                      : return "See Other"                      ;
	case status::not_modified                   : return "Not Modified"                   ;
	case status::use_proxy                      : return "Use Proxy"                      ;
	case status::temporary_redirect             : return "Temporary Redirect"             ;
	case status::permanent_redirect             : return "Permanent Redirect"             ;
	case status::bad_request                    : return "Bad Request"                    ;
	case status::unauthorized                   : return "Unauthorized"                   ;
	case status::payment_required               : return "Payment Required"               ;
	case status::forbidden                      : return "Forbidden"                      ;
	case status::not_found                      : return "Not Found"                      ;
	case status::method_not_allowed             : return "Method Not Allowed"             ;
	case status::not_acceptable                 : return "Not Acceptable"                 ;
	case status::proxy_authentication_required  : return "Proxy Authentication Required"  ;
	case status::request_timeout                : return "Request Timeout"                ;
	case status::conflict                       : return "Conflict"                       ;
	case status::gone                           : return "Gone"                           ;
	case status::length_required                : return "Length Required"                ;
	case status::precondition_failed            : return "Precondition Failed"            ;
	case status::payload_too_large              : return "Payload Too Large"              ;
	case status::uri_too_long                   : return "URI Too Long"                   ;
	case status::unsupported_media_type         : return "Unsupported Media Type"         ;
	case status::range_not_satisfiable          : return "Range Not Satisfiable"          ;
	case status::expectation_failed             : return "Expectation Failed"             ;
	case status::misdirected_request            : return "Misdirected Request"            ;
	case status::unprocessable_entity           : return "Unprocessable Entity"           ;
	case status::locked                         : return "Locked"                         ;
	case status::failed_dependency              : return "Failed Dependency"              ;
	case status::upgrade_required               : return "Upgrade Required"               ;
	case status::precondition_required          : return "Precondition Required"          ;
	case status::tooMany_requests               : return "Too Many Requests"              ;
	case status::request_header_fields_too_large: return "Request Header Fields Too Large";
	case status::unavailable_for_legal_reasons  : return "Unavailable For Legal Reasons"  ;
	case status::internal_server_error          : return "Internal Server Error"          ;
	case status::not_implemented                : return "Not Implemented"                ;
	case status::bad_gateway                    : return "Bad Gateway"                    ;
	case status::service_unavailable            : return "Service Unavailable"            ;
	case status::gateway_timeout                : return "Gateway Timeout"                ;
	case status::http_version_not_supported     : return "HTTP Version Not Supported"     ;
	case status::variant_also_negotiates        : return "Variant Also Negotiates"        ;
	case status::insufficient_storage           : return "Insufficient Storage"           ;
	case status::loop_detected                  : return "Loop Detected"                  ;
	case status::not_extended                   : return "Not Extended"                   ;
	case status::network_authentication_required: return "Network Authentication Required";
	default: break;
	}
	throw runtime_error("libgs::http: Invalid http status (enum): '{}'.", s);
//	return {};
}

template <concept_char_type CharT>
std::basic_string<CharT> to_method_string(method m)
{
	switch(m)
	{
	case method::GET    : return "GET"    ;
	case method::PUT    : return "PUT"    ;
	case method::POST   : return "POST"   ;
	case method::HEAD   : return "HEAD"   ;
	case method::DELETE : return "DELETE" ;
	case method::OPTIONS: return "OPTIONS";
	case method::CONNECT: return "CONNECT";
	case method::TRACH  : return "TRACH"  ;
	default: break;
	}
	throw runtime_error("libgs::http: Invalid http method (enum): '{}'.", m);
//	return {};
}

inline method from_method_string(std::string_view str)
{
	if( str == "GET" )
		return method::GET;
	else if( str == "PUT" )
		return method::PUT;
	else if( str == "POST" )
		return method::POST;
	else if( str == "HEAD" )
		return method::HEAD;
	else if( str == "DELETE" )
		return method::DELETE;
	else if( str == "OPTIONS" )
		return method::OPTIONS;
	else if( str == "CONNECT" )
		return method::CONNECT;
	else if( str == "TRACH" )
		return method::TRACH;
	else
		throw runtime_error("libgs::http: Invalid http method: '{}'.", str);
}

inline method from_method_string(std::wstring_view str)
{
	if( str == L"GET" )
		return method::GET;
	else if( str == L"PUT" )
		return method::PUT;
	else if( str == L"POST" )
		return method::POST;
	else if( str == L"HEAD" )
		return method::HEAD;
	else if( str == L"DELETE" )
		return method::DELETE;
	else if( str == L"OPTIONS" )
		return method::OPTIONS;
	else if( str == L"CONNECT" )
		return method::CONNECT;
	else if( str == L"TRACH" )
		return method::TRACH;
	else
		throw runtime_error("libgs::http: Invalid http method: '{}'.", wcstombs(str));
}

} //namespace libgs::http


#endif //LIBGS_HTTP_BASIC_DETAIL_TYPES_H
