
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

#ifndef LIBGS_HTTP_DETAIL_OPT_TOKEN_H
#define LIBGS_HTTP_DETAIL_OPT_TOKEN_H

#include <libgs/core/app_utls.h>

namespace libgs::http
{

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::single>::~file_opt()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::single>::file_opt(fstream_t &&stream) :
	stream(new fstream_t(std::move(stream)))
{
	if( not this->stream->is_open() )
		this->error = std::make_error_code(std::errc::bad_file_descriptor);
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::single>::file_opt(fstream_t &&stream, const file_range &range) :
	file_opt(std::move(stream))
{
	this->range = range;
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::single>::file_opt(std::string_view file_name)
{
	if( file_name.empty() )
		this->error = std::make_error_code(std::errc::invalid_argument);
	else
	{
		auto abs_name = app::absolute_path(file_name);
		stream->open(abs_name.c_str(), std::ios::binary);
		if( not stream->is_open() )
			this->error = std::make_error_code(static_cast<std::errc>(errno));
	}
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::single>::file_opt(std::string_view file_name, const file_range &range) :
	file_opt(file_name)
{
	this->range = range;
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> file_opt<FS,file_optype::single>::operator| (const file_range &range)
{
	return {*this, range};
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> file_opt<FS,file_optype::single>::operator| (file_ranges ranges)
{
	return {*this, std::move(ranges)};
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::single>::file_opt(fstream_t &stream) :
	stream(&stream)
{
	if( not stream.is_open() )
		this->error = std::make_error_code(static_cast<std::errc>(errno));
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::single>::file_opt(fstream_t &stream, const file_range &range) :
	file_opt(stream)
{
	this->range = range;
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple> file_opt<FS&,file_optype::single>::operator| (const file_range &range)
{
	return {*this, range};
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple> file_opt<FS&,file_optype::single>::operator| (file_ranges ranges)
{
	return {*this, std::move(ranges)};
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::~file_opt()
{
	if( stream.use_count() == 1 and stream->is_open() )
		stream->close();
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(fstream_t &&stream) :
	stream(new fstream_t(std::move(stream)))
{
	if( not this->stream->is_open() )
		this->error = std::make_error_code(static_cast<std::errc>(errno));
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(fstream_t &&stream, const file_range &range) :
	file_opt(std::move(stream))
{
	ranges.emplace_back(range);
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(fstream_t &&stream, file_ranges ranges) :
	file_opt(std::move(stream))
{
	this->ranges = std::move(ranges);
}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_opt<FS,file_optype::multiple>::file_opt(fstream_t &&stream, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	file_opt(std::move(stream))
{
	this->ranges = {std::forward<Args>(ranges)...};
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(std::string_view file_name)
{
	if( file_name.empty() )
		this->error = std::make_error_code(std::errc::invalid_argument);
	else
	{
		auto abs_name = app::absolute_path(file_name);
		stream->open(abs_name.c_str(), std::ios::binary);
		if( not stream->is_open() )
			this->error = std::make_error_code(static_cast<std::errc>(errno));
	}
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(std::string_view file_name, const file_range &range) :
	file_opt(file_name)
{
	ranges.emplace_back(range);
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(std::string_view file_name, file_ranges ranges) :
	file_opt(file_name)
{
	this->ranges = std::move(ranges);
}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_opt<FS,file_optype::multiple>::file_opt(std::string_view file_name, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	file_opt(file_name)
{
	this->ranges = {std::forward<Args>(ranges)...};
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(file_opt<FS,file_optype::single> &opt, const file_range &range) :
	stream(opt.stream), ranges{range}
{

}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple>::file_opt(file_opt<FS,file_optype::single> &opt, file_ranges ranges) :
	stream(opt.stream), ranges(std::move(ranges))
{

}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> &file_opt<FS,file_optype::multiple>::operator| (const file_range &range) &
{
	ranges.emplace_back(range);
	return *this;
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> &file_opt<FS,file_optype::multiple>::operator| (file_ranges ranges) &
{
	this->ranges.splice(this->ranges.end(), ranges);
	return *this;
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> &&file_opt<FS,file_optype::multiple>::operator| (const file_range &range) &&
{
	ranges.emplace_back(range);
	return std::move(*this);
}

template <concepts::fstream_wkn FS>
file_opt<FS,file_optype::multiple> &&file_opt<FS,file_optype::multiple>::operator| (file_ranges ranges) &&
{
	this->ranges.splice(this->ranges.end(), ranges);
	return std::move(*this);
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple>::file_opt(fstream_t &stream) :
	stream(&stream)
{

}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple>::file_opt(fstream_t &stream, const file_range &range) :
	file_opt(stream)
{
	ranges.emplace_back(range);
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple>::file_opt(fstream_t &stream, file_ranges ranges) :
	file_opt(stream)
{
	this->ranges = std::move(ranges);
}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_opt<FS&,file_optype::multiple>::file_opt(fstream_t &stream, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	file_opt(stream)
{
	this->rangesi = {std::forward<Args>(ranges)...};
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple>::file_opt(file_opt<FS&,file_optype::single> &opt, const file_range &range) :
	stream(opt.stream), ranges{range}
{

}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple>::file_opt(file_opt<FS&,file_optype::single> &opt, file_ranges ranges) :
	stream(opt.stream), ranges(std::move(ranges))
{

}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple> &file_opt<FS&,file_optype::multiple>::operator| (const file_range &range)
{
	ranges.emplace_back(range);
	return *this;
}

template <concepts::fstream_wkn FS>
file_opt<FS&,file_optype::multiple> &file_opt<FS&,file_optype::multiple>::operator| (file_ranges ranges)
{
	this->ranges.splice(this->ranges.end(), ranges);
	return *this;
}

template <typename...Args>
auto make_file_opt(std::string_view file_name, Args&&...args) noexcept
{
	auto abs_name = app::absolute_path(file_name);
	return make_file_opt(std::fstream(abs_name.c_str(), std::ios::binary), std::forward<Args>(args)...);
}

template <typename...Args>
auto make_file_opt(concepts::fstream_wkn auto &&stream, Args&&...args) noexcept
{
	using fstream_t = decltype(stream);
	if constexpr( sizeof...(args) == 0 )
		return file_opt<fstream_t,file_optype::single>(std::forward<fstream_t>(stream));
	else if constexpr( sizeof...(args) == 1 )
	{
		using first_arg_t = std::remove_cvref_t<std::tuple_element_t<0,std::tuple<Args&&...>>>;
		if constexpr( std::is_same_v<first_arg_t, file_range> )
			return file_opt<fstream_t,file_optype::single>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
		else
			return file_opt<fstream_t,file_optype::multiple>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
	}
	else
		return file_opt<fstream_t,file_optype::multiple>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
}

auto make_file_opt(concepts::file_opt auto &&opt) noexcept
{
	using opt_t = decltype(opt);
    if constexpr( std::is_rvalue_reference_v<opt_t> )
		return std::move(opt);
	else
		return opt;
}

namespace operators
{

inline auto operator| (std::string_view file_name, const file_range &range)
{
	return make_file_opt(file_name, range);
}

inline auto operator| (std::string_view file_name, file_ranges ranges)
{
	return make_file_opt(file_name, std::move(ranges));
}

auto operator| (concepts::fstream_wkn auto &&stream, const file_range &range)
{
	using fstream_t = decltype(stream);
	return make_file_opt(std::forward<fstream_t>(stream), range);
}

auto operator| (concepts::fstream_wkn auto &&stream, file_ranges ranges)
{
	using fstream_t = decltype(stream);
	return make_file_opt(std::forward<fstream_t>(stream), std::move(ranges));
}

}} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_OPT_TOKEN_H