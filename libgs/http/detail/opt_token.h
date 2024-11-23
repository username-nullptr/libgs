
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
file_single_opt<FS>::file_single_opt(fstream_t &&stream) :
	stream(std::move(stream))
{

}

template <concepts::fstream_wkn FS>
file_single_opt<FS>::file_single_opt(fstream_t &&stream, const file_range &range) :
	stream(std::move(stream)), range(range)
{

}

template <concepts::fstream_wkn FS>
file_single_opt<FS>::file_single_opt(std::string_view file_name)
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
file_single_opt<FS>::file_single_opt(std::string_view file_name, const file_range &range) :
	range(range)
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> file_single_opt<FS>::operator| (const file_range &range)
{
	return {std::move(stream), range};
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> file_single_opt<FS>::operator| (file_ranges ranges)
{
	return {std::move(stream), std::move(ranges)};
}

template <concepts::fstream_wkn FS>
file_single_opt<FS&>::file_single_opt(fstream_t &stream) :
	stream(stream)
{

}

template <concepts::fstream_wkn FS>
file_single_opt<FS&>::file_single_opt(fstream_t &stream, const file_range &range) :
	stream(stream), range(range)
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&> file_single_opt<FS&>::operator| (const file_range &range)
{
	return {stream, range};
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&> file_single_opt<FS&>::operator| (file_ranges ranges)
{
	return {stream, std::move(ranges)};
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(fstream_t &&stream) :
	stream(std::move(stream))
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(fstream_t &&stream, const file_range &range) :
	stream(std::move(stream)), ranges{range}
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(fstream_t &&stream, file_ranges ranges) :
	stream(std::move(stream)), ranges(std::move(ranges))
{

}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_multiple_opt<FS>::file_multiple_opt(fstream_t &&stream, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	stream(std::move(stream)), ranges{std::forward<Args>(ranges)...}
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(std::string_view file_name)
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(std::string_view file_name, const file_range &range) :
	ranges{range}
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS>::file_multiple_opt(std::string_view file_name, file_ranges ranges) :
	ranges(std::move(ranges))
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_multiple_opt<FS>::file_multiple_opt(std::string_view file_name, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	ranges{std::forward<Args>(ranges)...}
{
	auto abs_name = app::absolute_path(file_name);
	stream.open(abs_name.c_str(), std::ios::binary);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> &file_multiple_opt<FS>::operator| (const file_range &range) &
{
	ranges.emplace_back(range);
	return *this;
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> &file_multiple_opt<FS>::operator| (file_ranges ranges) &
{
	this->ranges.splice(this->ranges.end(), ranges);
	return *this;
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> &&file_multiple_opt<FS>::operator| (const file_range &range) &&
{
	ranges.emplace_back(range);
	return std::move(*this);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS> &&file_multiple_opt<FS>::operator| (file_ranges ranges) &&
{
	this->ranges.splice(this->ranges.end(), ranges);
	return std::move(*this);
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&>::file_multiple_opt(fstream_t &stream) :
	stream(stream)
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&>::file_multiple_opt(fstream_t &stream, const file_range &range) :
	stream(stream), ranges{range}
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&>::file_multiple_opt(fstream_t &stream, file_ranges ranges) :
	stream(stream), ranges(std::move(ranges))
{

}

template <concepts::fstream_wkn FS>
template <typename...Args>
file_multiple_opt<FS&>::file_multiple_opt(fstream_t &stream, Args&&...ranges) requires
	requires { file_ranges{std::forward<Args>(ranges)...}; } :
	stream(stream), ranges{std::forward<Args>(ranges)...}
{

}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&> &file_multiple_opt<FS&>::operator| (const file_range &range)
{
	ranges.emplace_back(range);
	return *this;
}

template <concepts::fstream_wkn FS>
file_multiple_opt<FS&> &file_multiple_opt<FS&>::operator| (file_ranges ranges)
{
	this->ranges.splice(this->ranges.end(), ranges);
	return *this;
}

template <typename...Args>
auto make_file_opt(std::string_view file_name, Args&&...args)
{
	auto abs_name = app::absolute_path(file_name);
	return make_file_opt(std::fstream(abs_name.c_str(), std::ios::binary), std::forward<Args>(args)...);
}

template <typename...Args>
auto make_file_opt(concepts::fstream_wkn auto &&stream, Args&&...args)
{
	using fstream_t = decltype(stream);
	if constexpr( sizeof...(args) == 0 )
		return file_single_opt<fstream_t>(std::forward<fstream_t>(stream));
	else if constexpr( sizeof...(args) == 1 )
	{
		using first_arg_t = std::remove_cvref_t<std::tuple_element_t<0,std::tuple<Args&&...>>>;
		if constexpr( std::is_same_v<first_arg_t, file_range> )
			return file_single_opt<fstream_t>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
		else
			return file_multiple_opt<fstream_t>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
	}
	else
		return file_multiple_opt<fstream_t>(std::forward<fstream_t>(stream), std::forward<Args>(args)...);
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