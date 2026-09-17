// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "types.h"

namespace libgs::http
{

bool status::check(enumeration status, bool _throw)
{
	switch(status)
	{
#define X_MACRO(e,v,d) case e:
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
			return true;
	default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::status::check: Invalid http status: '{}'.",
				status
			));
		}
		break;
	}
	return false;
}

const char *status::description(enumeration status, bool _throw)
{
	switch(status)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::status::description: Invalid http status: '{}'.",
				status
			));
		}
		break;
	}
	return "";
}

bool method::check(enumeration method, bool _throw)
{
	switch(method)
	{
#define X_MACRO(e,v,d) case e:
		LIBGS_HTTP_METHOD_TABLE
	#undef X_MACRO
				return true;
	default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::method::check: Invalid http method: '{}'.",
				method
			));
		}
		break;
	}
	return false;
}

const char *method::string(enumeration method, bool _throw)
{
	switch(method)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_METHOD_TABLE
	#undef X_MACRO
		default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::method::string: Invalid http method: '{}'.",
				method
			));
		}
		break;
	}
	return "";
}

bool redirect::check(enumeration redirect, bool _throw)
{
	switch(redirect)
	{
#define X_MACRO(e,v,d) case e:
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
	#undef X_MACRO
			return true;
	default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::redirect::check: Invalid http redirect type: '{}'.",
				redirect
			));
		}
		break;
	}
	return "";
}

const char *redirect::description(enumeration redirect, bool _throw)
{
	switch(redirect)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
	#undef X_MACRO
		default:
		if( _throw )
		{
			invalid_argument::loc_throw(std::format (
				"libgs::http::redirect::string: Invalid http redirect type: '{}'.",
				redirect
			));
		}
		break;
	}
	return "";
}

} //namespace libgs::http
