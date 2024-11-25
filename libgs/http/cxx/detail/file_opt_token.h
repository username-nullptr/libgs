
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

#ifndef LIBGS_HTTP_CXX_DETAIL_FILE_OPT_TOKEN_H
#define LIBGS_HTTP_CXX_DETAIL_FILE_OPT_TOKEN_H

#include <libgs/core/algorithm/mime_type.h>
#include <libgs/core/app_utls.h>
#include <filesystem>

namespace libgs::http
{

template <core_concepts::fstream_wkn FS>
void file_opt_token_base<FS>::set_size(auto &stream)
{
	stream->seekp(0, std::ios::end);
	size = stream->tellg();
	stream->seekp(0);
}

template <typename T>
file_opt_token_range<T,file_optype::single>::file_opt_token_range(const file_range &range) :
	range(range)
{

}

template <typename T>
file_opt_token_range<T,file_optype::multiple>::file_opt_token_range(const file_range &range) :
	ranges{range}
{

}

template <typename T>
file_opt_token_range<T,file_optype::multiple>::file_opt_token_range(file_ranges ranges) :
	ranges(std::move(ranges))
{

}

template <typename T>
file_opt_token<T,file_optype::multiple>&
file_opt_token_range<T,file_optype::multiple>::operator| (const file_range &range) &
{
	ranges.emplace_back(range);
	return static_cast<file_opt_token<T,file_optype::multiple>&>(*this);
}

template <typename T>
file_opt_token<T,file_optype::multiple>&
file_opt_token_range<T,file_optype::multiple>::operator| (file_ranges ranges) &
{
	ranges.splice(ranges.end(), std::move(ranges));
	return static_cast<file_opt_token<T,file_optype::multiple>&>(*this);
}

template <typename T>
file_opt_token<T,file_optype::multiple>&&
file_opt_token_range<T,file_optype::multiple>::operator| (const file_range &range) &&
{
	ranges.emplace_back(range);
	return static_cast<file_opt_token<T,file_optype::multiple>&&>(*this);
}

template <typename T>
file_opt_token<T,file_optype::multiple>&&
file_opt_token_range<T,file_optype::multiple>::operator| (file_ranges ranges) &&
{
	ranges.splice(ranges.end(), std::move(ranges));
	return static_cast<file_opt_token<T,file_optype::multiple>&&>(*this);
}

inline file_opt_token<void,file_optype::single>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

inline file_opt_token<void,file_optype::single>::file_opt_token(std::string_view file_name) :
	file_opt_token(file_name, {0,0})
{

}

inline file_opt_token<void,file_optype::single>::file_opt_token(std::string_view file_name, const file_range &range) :
	file_opt_token_range(range), file_name(file_name.data(), file_name.size())
{

}

inline file_opt_token<void,file_optype::multiple>
file_opt_token<void,file_optype::single>::operator| (const file_range &range)
{
	return {*this, range};
}

inline file_opt_token<void,file_optype::multiple>
file_opt_token<void,file_optype::single>::operator| (file_ranges ranges)
{
	return {*this, std::move(ranges)};
}

inline error_code file_opt_token<void,file_optype::single>::init(int flags) noexcept
{
	error_code error;
	if( file_name.empty() )
		error = std::make_error_code(std::errc::invalid_argument);
	else
	{
		auto abs_name = app::absolute_path(error, file_name);
		if( not error )
		{
			file_name = std::move(abs_name);
			namespace fs = std::filesystem;

			if( (flags & std::ios_base::out) == 0 and not fs::exists(file_name) )
				error = std::make_error_code(std::errc::no_such_file_or_directory);
			else
			{
				stream->open(file_name.c_str(), flags);
				if( not stream->is_open() )
					error = std::make_error_code(static_cast<std::errc>(errno));
				else
					this->set_size(stream);
			}
		}
	}
	return error;
}

inline std::string file_opt_token<void,file_optype::single>::mime_type()
{
	return get_mime_type(file_name);
}

// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::single>::~file_opt_token()
// {
// 	if( stream.use_count() == 1 and stream->is_open() )
// 		stream->close();
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::single>::file_opt_token(fstream_t &&stream) :
// 	file_opt_token(std::move(stream), {0,0})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::single>::file_opt_token(fstream_t &&stream, const file_range &range) :
// 	file_opt_token_range<FS&&,file_optype::single>(range), stream(new fstream_t(std::move(stream)))
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>
// file_opt_token<FS&&,file_optype::single>::operator| (const file_range &range)
// {
// 	return {*this, range};
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>
// file_opt_token<FS&&,file_optype::single>::operator| (file_ranges ranges)
// {
// 	return {*this, std::move(ranges)};
// }
//
// template <core_concepts::fstream_wkn FS>
// error_code file_opt_token<FS&&,file_optype::single>::init(int) noexcept
// {
// 	if( stream->is_open() )
// 	{
// 		this->set_size(stream);
// 		return error_code();
// 	}
// 	return std::make_error_code(std::errc::bad_file_descriptor);
// }
//
// template <core_concepts::fstream_wkn FS>
// std::string file_opt_token<FS&&,file_optype::single>::mime_type()
// {
// 	return get_mime_type(*stream);
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::single>::file_opt_token(fstream_t &stream) :
// 	file_opt_token(stream, {0,0})
// {
// 	if( not stream.is_open() )
// 		this->error = std::make_error_code(static_cast<std::errc>(errno));
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::single>::file_opt_token(fstream_t &stream, const file_range &range) :
// 	file_opt_token_range<FS&,file_optype::single>(range), stream(&stream)
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple> file_opt_token<FS&,file_optype::single>::operator| (const file_range &range)
// {
// 	return {*this, range};
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple> file_opt_token<FS&,file_optype::single>::operator| (file_ranges ranges)
// {
// 	return {*this, std::move(ranges)};
// }
//
// template <core_concepts::fstream_wkn FS>
// error_code file_opt_token<FS&,file_optype::single>::init(int) noexcept
// {
// 	if( stream->is_open() )
// 	{
// 		this->set_size(stream);
// 		return error_code();
// 	}
// 	return std::make_error_code(std::errc::bad_file_descriptor);
// }
//
// template <core_concepts::fstream_wkn FS>
// std::string file_opt_token<FS&,file_optype::single>::mime_type()
// {
// 	return get_mime_type(*stream);
// }

inline file_opt_token<void,file_optype::multiple>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name) :
	file_opt_token(file_name, file_range{0,0})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, const file_range &range) :
	file_opt_token(file_name, file_ranges{range})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, file_ranges ranges) :
	file_opt_token_range(std::move(ranges)), stream(new fstream_t()), file_name(file_name.data(), file_name.size())
{

}

template <typename...Args>
file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	file_opt_token(file_name, file_ranges{std::forward<Args>(ranges)...})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token
(file_opt_token<void,file_optype::single> &opt, const file_range &range) :
	file_opt_token(opt, file_ranges{range})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token
(file_opt_token<void,file_optype::single> &opt, file_ranges ranges) :
	file_opt_token_range(std::move(ranges)), stream(opt.stream), file_name(opt.file_name)
{

}

inline error_code file_opt_token<void,file_optype::multiple>::init(int flags) noexcept
{
	error_code error;
	if( file_name.empty() )
		error = std::make_error_code(std::errc::invalid_argument);
	else
	{
		auto abs_name = app::absolute_path(error, file_name);
		if( not error )
		{
			file_name = std::move(abs_name);
			namespace fs = std::filesystem;

			if( (flags & std::ios_base::out) == 0 and not fs::exists(file_name) )
				error = std::make_error_code(std::errc::no_such_file_or_directory);
			else
			{
				stream->open(file_name.c_str(), flags);
				if( not stream->is_open() )
					error = std::make_error_code(static_cast<std::errc>(errno));
				else
					this->set_size(stream);
			}
		}
	}
	return error;
}

inline std::string file_opt_token<void,file_optype::multiple>::mime_type()
{
	return get_mime_type(file_name);
}

// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::~file_opt_token()
// {
// 	if( stream.use_count() == 1 and stream->is_open() )
// 		stream->close();
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream) :
// 	file_opt_token(std::move(stream), file_range{0,0})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, const file_range &range) :
// 	file_opt_token(std::move(stream), file_ranges{range})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, file_ranges ranges) :
// 	file_opt_token_range<FS&&,file_optype::multiple>(std::move(ranges)), stream(new fstream_t(std::move(stream)))
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// template <typename...Args>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, Args&&...ranges) requires
// 	requires { file_ranges{std::forward<Args>(ranges)...}; } :
// 	file_opt_token(std::move(stream), file_ranges{std::forward<Args>(ranges)...})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token
// (file_opt_token<FS&&,file_optype::single> &opt, const file_range &range) :
// 	file_opt_token(opt, file_ranges{range})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&&,file_optype::multiple>::file_opt_token
// (file_opt_token<FS&&,file_optype::single> &opt, file_ranges ranges) :
// 	file_opt_token_range<FS&&,file_optype::multiple>(std::move(ranges)), stream(opt.stream)
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// error_code file_opt_token<FS&&,file_optype::multiple>::init(int) noexcept
// {
// 	if( stream->is_open() )
// 	{
// 		this->set_size(stream);
// 		return error_code();
// 	}
// 	return std::make_error_code(std::errc::bad_file_descriptor);
// }
//
// template <core_concepts::fstream_wkn FS>
// std::string file_opt_token<FS&&,file_optype::multiple>::mime_type()
// {
// 	return get_mime_type(*stream);
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream) :
// 	file_opt_token(stream, file_range{0,0})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, const file_range &range) :
// 	file_opt_token(stream, file_ranges{range})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, file_ranges ranges) :
// 	file_opt_token_range<FS&,file_optype::multiple>(std::move(ranges)), stream(stream)
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// template <typename...Args>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, Args&&...ranges) requires
// 	requires { file_ranges{std::forward<Args>(ranges)...}; } :
// 	file_opt_token(stream, file_ranges{std::forward<Args>(ranges)...})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token
// (file_opt_token<FS&,file_optype::single> &opt, const file_range &range) :
// 	file_opt_token(opt, file_ranges{range})
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// file_opt_token<FS&,file_optype::multiple>::file_opt_token
// (file_opt_token<FS&,file_optype::single> &opt, file_ranges ranges) :
// 	file_opt_token_range<FS&,file_optype::multiple>(std::move(ranges)), stream(opt.stream)
// {
//
// }
//
// template <core_concepts::fstream_wkn FS>
// error_code file_opt_token<FS&,file_optype::multiple>::init(int) noexcept
// {
// 	if( stream->is_open() )
// 	{
// 		this->set_size(stream);
// 		return error_code();
// 	}
// 	return std::make_error_code(std::errc::bad_file_descriptor);
// }
//
// template <core_concepts::fstream_wkn FS>
// std::string file_opt_token<FS&,file_optype::multiple>::mime_type()
// {
// 	return get_mime_type(*stream);
// }
//
// namespace detail
// {
//
// template <typename T, typename...Args>
// auto make_file_opt_token(auto &&arg, Args&&...args) noexcept
// {
// 	using arg_t = decltype(arg);
// 	if constexpr( sizeof...(args) == 0 )
// 		return file_opt_token<T,file_optype::single>(std::forward<arg_t>(arg));
// 	else if constexpr( sizeof...(args) == 1 )
// 	{
// 		using first_arg_t = std::remove_cvref_t<std::tuple_element_t<0,std::tuple<Args&&...>>>;
// 		if constexpr( std::is_same_v<first_arg_t, file_range> )
// 			return file_opt_token<T,file_optype::single>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
// 		else
// 			return file_opt_token<T,file_optype::multiple>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
// 	}
// 	else
// 		return file_opt_token<T,file_optype::multiple>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
// }
//
// } //namespace detail
//
// template <typename...Args>
// auto make_file_opt_token(std::string_view file_name, Args&&...args) noexcept
// {
// 	return detail::make_file_opt_token<void>(file_name, std::forward<Args>(args)...);
// }
//
// template <typename...Args>
// auto make_file_opt_token(core_concepts::fstream_wkn auto &&stream, Args&&...args) noexcept
// {
// 	using fstream_t = decltype(stream);
// 	return detail::make_file_opt_token<fstream_t>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
// }
//
// auto make_file_opt_token(concepts::file_opt_token auto &&opt) noexcept
// {
// 	return std::forward<decltype(opt)>(opt);
// }

namespace operators
{

// inline auto operator| (std::string_view file_name, const file_range &range)
// {
// 	return make_file_opt_token(file_name, range);
// }
//
// inline auto operator| (std::string_view file_name, file_ranges ranges)
// {
// 	return make_file_opt_token(file_name, std::move(ranges));
// }
//
// auto operator| (core_concepts::fstream_wkn auto &&stream, const file_range &range)
// {
// 	using fstream_t = decltype(stream);
// 	return make_file_opt_token(std::forward<fstream_t>(stream), range);
// }
//
// auto operator| (core_concepts::fstream_wkn auto &&stream, file_ranges ranges)
// {
// 	using fstream_t = decltype(stream);
// 	return make_file_opt_token(std::forward<fstream_t>(stream), std::move(ranges));
// }

}} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_FILE_OPT_TOKEN_H