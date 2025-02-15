
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

inline file_opt_token<void,file_optype::single>::file_opt_token(std::string_view file_name) :
	file_name(file_name.data(), file_name.size())
{

}

inline file_opt_token<void,file_optype::single>::file_opt_token(std::string_view file_name, const file_range &range) :
	file_name(file_name.data(), file_name.size()),
	range(range)
{

}

inline file_opt_token<void,file_optype::single>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

inline error_code file_opt_token<void,file_optype::single>::init(std::ios_base::openmode mode) noexcept
{
	if( file_name.empty() )
		return std::make_error_code(std::errc::invalid_argument);

	error_code error;
	auto abs_name = app::absolute_path(error, file_name);
	if( error )
		return error;

	file_name = std::move(abs_name);
	namespace fs = std::filesystem;

	if( (mode & std::ios_base::out) == 0 and not fs::exists(mbstowcs(file_name)) )
		return std::make_error_code(std::errc::no_such_file_or_directory);

	stream->open(file_name.c_str(), mode);
	if( not stream->is_open() )
		return std::make_error_code(static_cast<std::errc>(errno));
	return error;
}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::single>::file_opt_token(fstream_t &&stream) :
	stream(new fstream_t(std::move(stream)))
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::single>::file_opt_token(fstream_t &&stream, const file_range &range) :
	stream(new fstream_t(std::move(stream))),
	range(range)
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::single>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

template <core_concepts::fstream_wkn FS>
error_code file_opt_token<FS&&,file_optype::single>::init(std::ios_base::openmode) noexcept
{
	return stream->is_open() ? error_code() : std::make_error_code(std::errc::bad_file_descriptor);
}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::single>::file_opt_token(fstream_t &stream) :
	stream(&stream)
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::single>::file_opt_token(fstream_t &stream, const file_range &range) :
	stream(&stream),
	range(range)
{

}

template <core_concepts::fstream_wkn FS>
error_code file_opt_token<FS&,file_optype::single>::init(std::ios_base::openmode) noexcept
{
	return stream->is_open() ? error_code() : std::make_error_code(std::errc::bad_file_descriptor);
}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name) :
	stream(new fstream_t()),
	file_name(file_name.data(), file_name.size())
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, const file_range &range) :
	file_opt_token(file_name, file_ranges{range})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, file_ranges ranges) :
	stream(new fstream_t()),
	file_name(file_name.data(), file_name.size()),
	ranges(std::move(ranges))
{

}

template <concepts::file_ranges_init_list...Args>
file_opt_token<void,file_optype::multiple>::file_opt_token(std::string_view file_name, Args&&...ranges) :
	file_opt_token(file_name, file_ranges{std::forward<Args>(ranges)...})
{

}

inline file_opt_token<void,file_optype::multiple>::file_opt_token(file_opt_token<type,file_optype::single> opt) :
	stream(std::move(opt.stream)),
	file_name(std::move(opt.file_name))
{
	if( opt.range )
		ranges.emplace_back(*opt.range);
}

inline file_opt_token<void,file_optype::multiple>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

inline error_code file_opt_token<void,file_optype::multiple>::init(std::ios_base::openmode mode) noexcept
{
	if( file_name.empty() )
		return std::make_error_code(std::errc::invalid_argument);

	error_code error;
	auto abs_name = app::absolute_path(error, file_name);
	if( error )
		return error;

	file_name = std::move(abs_name);
	namespace fs = std::filesystem;

	if( (mode & std::ios_base::out) == 0 and not fs::exists(mbstowcs(file_name)) )
		return std::make_error_code(std::errc::no_such_file_or_directory);

	stream->open(file_name.c_str(), mode);
	if( not stream->is_open() )
		return std::make_error_code(static_cast<std::errc>(errno));
	return error;
}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream) :
	stream(new fstream_t(std::move(stream)))
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, const file_range &range) :
	file_opt_token(std::move(stream), file_ranges{range})
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, file_ranges ranges) :
	stream(new fstream_t(std::move(stream))),
	ranges(std::move(ranges))
{

}

template <core_concepts::fstream_wkn FS>
template <concepts::file_ranges_init_list...Args>
file_opt_token<FS&&,file_optype::multiple>::file_opt_token(fstream_t &&stream, Args&&...ranges) :
	file_opt_token(std::move(stream), file_ranges{std::forward<Args>(ranges)...})
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::multiple>::file_opt_token(file_opt_token<type,file_optype::single> opt) :
	stream(std::move(opt.stream))
{
	if( opt.range )
		ranges.emplace_back(*opt.range);
}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&&,file_optype::multiple>::~file_opt_token()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

template <core_concepts::fstream_wkn FS>
error_code file_opt_token<FS&&,file_optype::multiple>::init(std::ios_base::openmode) noexcept
{
	return stream->is_open() ? error_code() : std::make_error_code(std::errc::bad_file_descriptor);
}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream) :
	stream(new fstream_t(std::move(stream)))
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, const file_range &range) :
	file_opt_token(stream, file_ranges{range})
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, file_ranges ranges) :
	stream(stream),
	ranges(std::move(ranges))
{

}

template <core_concepts::fstream_wkn FS>
template <concepts::file_ranges_init_list...Args>
file_opt_token<FS&,file_optype::multiple>::file_opt_token(fstream_t &stream, Args&&...ranges) :
	file_opt_token(stream, file_ranges{std::forward<Args>(ranges)...})
{

}

template <core_concepts::fstream_wkn FS>
file_opt_token<FS&,file_optype::multiple>::file_opt_token(file_opt_token<type,file_optype::single> opt) :
	stream(opt.stream)
{
	if( opt.range )
		ranges.emplace_back(*opt.range);
}

template <core_concepts::fstream_wkn FS>
error_code file_opt_token<FS&,file_optype::multiple>::init(std::ios_base::openmode) noexcept
{
	return stream->is_open() ? error_code() : std::make_error_code(std::errc::bad_file_descriptor);
}

namespace detail
{

template <typename T, typename...Args>
auto make_file_opt_token(auto &&arg, Args&&...args) noexcept
{
	using arg_t = decltype(arg);
	if constexpr( sizeof...(args) == 0 )
		return file_opt_token<T,file_optype::single>(std::forward<arg_t>(arg));
	else if constexpr( sizeof...(args) == 1 )
	{
		using first_arg_t = std::remove_cvref_t<std::tuple_element_t<0,std::tuple<Args&&...>>>;
		if constexpr( std::is_same_v<first_arg_t, file_range> )
			return file_opt_token<T,file_optype::single>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
		else
			return file_opt_token<T,file_optype::multiple>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
	}
	else
		return file_opt_token<T,file_optype::multiple>(std::forward<arg_t>(arg), std::forward<Args>(args)...);
}

} //namespace detail

template <typename...Args>
auto make_file_opt_token(std::string_view file_name, Args&&...args) noexcept
{
	return detail::make_file_opt_token<void>(file_name, std::forward<Args>(args)...);
}

template <typename...Args>
auto make_file_opt_token(core_concepts::fstream_wkn auto &&stream, Args&&...args) noexcept
{
	using fstream_t = decltype(stream);
	return detail::make_file_opt_token<fstream_t>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
}

std::optional<size_t> file_size(concepts::file_opt_token auto &opt, io_permission::type mode)
{
	using opt_t = std::remove_cvref_t<decltype(opt)>;
	using fstream_t = typename opt_t::fstream_t;

	std::optional<size_t> size;
	if constexpr( is_fstream_v<fstream_t> )
	{
		if( mode & io_permission::read )
		{
			auto cur = opt.stream->tellg();
			if( opt.stream->good() )
			{
				opt.stream->seekg(0, std::ios::end);
				size = opt.stream->tellg();
				opt.stream->seekg(cur, std::ios::beg);
			}
		}
		else if( mode & io_permission::write )
		{
			auto cur = opt.stream->tellp();
			if( opt.stream->good() )
			{
				opt.stream->seekp(0, std::ios::end);
				size = opt.stream->tellp();
				opt.stream->seekp(cur, std::ios::beg);
			}
		}
	}
	if constexpr( is_ifstream_v<fstream_t> )
	{
		if( mode & io_permission::read )
		{
			auto cur = opt.stream->tellg();
			opt.stream->seekg(0, std::ios::end);
			size = opt.stream->tellg();
			opt.stream->seekg(cur, std::ios::beg);
		}
	}
	else
	{
		if( mode & io_permission::write )
		{
			auto cur = opt.stream->tellp();
			opt.stream->seekp(0, std::ios::end);
			size = opt.stream->tellp();
			opt.stream->seekp(cur, std::ios::beg);
		}
	}
	return size;
}

std::string mime_type(concepts::file_opt_token auto &opt)
{
	using opt_t = std::remove_cvref_t<decltype(opt)>;
	using type = typename opt_t::type;

	if constexpr( std::is_same_v<type,void> )
		return libgs::mime_type(opt.file_name);
	else if constexpr( opt_t::permissions & io_permission::read )
		return libgs::mime_type(opt.stream);
	else
		return "Unknown";
}

namespace operators
{

inline auto operator| (std::string_view file_name, const file_range &range)
{
	return make_file_opt_token(file_name, range);
}

inline auto operator| (std::string_view file_name, file_ranges ranges)
{
	return make_file_opt_token(file_name, std::move(ranges));
}

auto operator| (core_concepts::fstream_wkn auto &&stream, const file_range &range)
{
	using fstream_t = decltype(stream);
	return make_file_opt_token(std::forward<fstream_t>(stream), range);
}

auto operator| (core_concepts::fstream_wkn auto &&stream, file_ranges ranges)
{
	using fstream_t = decltype(stream);
	return make_file_opt_token(std::forward<fstream_t>(stream), std::move(ranges));
}

template <typename T>
file_opt_token<T,file_optype::multiple> operator|
(file_opt_token<T,file_optype::single> opt, const file_range &range)
{
	file_opt_token<T,file_optype::multiple> token(std::move(opt));
	token.ranges.emplace_back(range);
	return token;
}

template <typename T>
file_opt_token<T,file_optype::multiple> operator|
(file_opt_token<T,file_optype::single> opt, file_ranges ranges)
{
	file_opt_token<T,file_optype::multiple> token(std::move(opt));
	token.ranges.splice(token.ranges.end(), std::move(ranges));
	return token;
}

template <typename T>
file_opt_token<T,file_optype::multiple> &operator|
(file_opt_token<T,file_optype::multiple> &opt, const file_range &range)
{
	opt.ranges.emplace_back(range);
	return opt;
}

template <typename T>
file_opt_token<T,file_optype::multiple> &operator|
(file_opt_token<T,file_optype::multiple> &opt, file_ranges ranges)
{
	opt.ranges.splice(opt.ranges.end(), std::move(ranges));
	return opt;
}

template <typename T>
file_opt_token<T,file_optype::multiple> &&operator|
(file_opt_token<T,file_optype::multiple> &&opt, const file_range &range)
{
	opt.ranges.emplace_back(range);
	return std::move(opt);
}

template <typename T>
file_opt_token<T,file_optype::multiple> &&operator|
(file_opt_token<T,file_optype::multiple> &&opt, file_ranges ranges)
{
	opt.ranges.splice(opt.ranges.end(), std::move(ranges));
	return std::move(opt);
}

}} //namespace libgs::http


#endif //LIBGS_HTTP_CXX_DETAIL_FILE_OPT_TOKEN_H