// SPDX-FileCopyrightText: 2024-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_HTTP_PROTOCOL_DETAIL_TYPES_H
#define LIBGS_HTTP_PROTOCOL_DETAIL_TYPES_H

namespace libgs::http
{

inline bool status::check(enumeration status, bool _throw)
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
				runtime_error::loc_throw(std::format (
					"libgs::http::status::check: Invalid http status: '{}'.",
					status
				));
			}
			break;
	}
	return false;
}

template <status_enum Status>
consteval bool status::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Status == e ) return true;
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else return false;
}

inline const char *status::description(enumeration status, bool _throw)
{
	switch(status)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			runtime_error::loc_throw(std::format (
				"libgs::http::status::description: Invalid http status: '{}'.",
				status
			));
		}
		break;
	}
	return "";
}

template <status_enum Status>
consteval const char *status::description() requires is_valid_v<Status>
{
#define X_MACRO(e,v,d) if constexpr( Status == e ) return d;
	LIBGS_HTTP_STATUS_TABLE
#undef X_MACRO
	else return "";
}

inline bool method::check(enumeration method, bool _throw)
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
			runtime_error::loc_throw(std::format (
				"libgs::http::method::check: Invalid http method: '{}'.",
				method
			));
		}
		break;
	}
	return false;
}

template <method_enum Method>
consteval bool method::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Method == e ) return true;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else return false;
}

inline const char *method::string(enumeration method, bool _throw)
{
	switch(method)
	{
#define X_MACRO(e,v,d) case e: return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	default:
		if( _throw )
		{
			runtime_error::loc_throw(std::format (
				"libgs::http::method::string: Invalid http method: '{}'.",
				method
			));
		}
		break;
	}
	return "";
}

template <method_enum Method>
consteval const char *method::string() requires is_valid_v<Method>
{
#define X_MACRO(e,v,d) if constexpr( Method == e ) return d;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	else return "";
}

constexpr method_enum method::from_string(std::string_view str)
{
#define X_MACRO(e,v,d) if( str == d ) return method::e;
	LIBGS_HTTP_METHOD_TABLE
#undef X_MACRO
	runtime_error::loc_throw(std::format (
		"libgs::http::method::from_string: Invalid http method: '{}'.", str
	));
}

constexpr method::method(std::string_view str) :
	value(from_string(str))
{

}

inline bool redirect::check(enumeration redirect, bool _throw)
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
			runtime_error::loc_throw(std::format (
				"libgs::http::redirect::check: Invalid http redirect type: '{}'.",
				redirect
			));
		}
		break;
	}
	return "";
}

template <redirect_enum Redirect>
consteval bool redirect::is_valid()
{
#define X_MACRO(e,v,d) if constexpr( Redirect == e ) return true;
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
	else return false;
}

inline const char *redirect::description(enumeration redirect, bool _throw)
{
	switch(redirect)
	{
#define X_MACRO(e,v,d) case e: return d;
		LIBGS_HTTP_REDIRECT_TYPE_TABLE
	#undef X_MACRO
		default:
		if( _throw )
		{
			runtime_error::loc_throw(std::format (
				"libgs::http::redirect::string: Invalid http redirect type: '{}'.",
				redirect
			));
		}
		break;
	}
	return "";
}

template <redirect_enum Redirect>
consteval const char *redirect::description() requires is_valid_v<Redirect>
{
#define X_MACRO(e,v,d) if constexpr( Redirect == e ) return d;
	LIBGS_HTTP_REDIRECT_TYPE_TABLE
#undef X_MACRO
	else return "";
}

} //namespace libgs::http


#endif //LIBGS_HTTP_PROTOCOL_DETAIL_TYPES_H
