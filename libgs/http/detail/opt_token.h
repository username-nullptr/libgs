
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

namespace libgs::http
{

template <typename CT>
file_opt_base<CT>::file_opt_base(const file_range &range) :
	ranges{range}
{

}

template <typename CT>
file_opt_base<CT>::file_opt_base(file_ranges ranges) :
	ranges(std::move(ranges))
{

}

template <typename T>
file_opt<T>::file_opt(std::string_view file_name) :
	file_name(file_name.data(), file_name.size()),
	stream(file_name.data(), std::ios::binary)
{

}

template <typename T>
file_opt<T>::file_opt(std::string_view filename, const file_range &range) :
	file_opt_base<file_opt>(range),
	file_name(filename.data(), filename.size()),
	stream(file_name.data(), std::ios::binary)
{

}

template <typename T>
file_opt<T>::file_opt(std::string_view filename, file_ranges ranges) :
	file_opt_base<file_opt>(std::move(ranges)),
	file_name(filename.data(), filename.size()),
	stream(file_name.data(), std::ios::binary)
{

}

template <concepts::fiostream FS> requires std::is_lvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream) :
	stream(&stream)
{

}

template <concepts::fiostream FS> requires std::is_lvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream, const file_range &range) :
	file_opt_base<file_opt>(range),
	stream(&stream)
{

}

template <concepts::fiostream FS> requires std::is_lvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream, file_ranges ranges) :
	file_opt_base<file_opt>(std::move(ranges)),
	stream(&stream)
{

}

template <concepts::fiostream FS> requires std::is_rvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream) :
	stream(std::move(stream))
{

}

template <concepts::fiostream FS> requires std::is_rvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream, const file_range &range) :
	file_opt_base<file_opt>(range),
	stream(std::move(stream))
{

}

template <concepts::fiostream FS> requires std::is_rvalue_reference_v<FS>
file_opt<FS>::file_opt(fstream_t stream, file_ranges ranges) :
	file_opt_base<file_opt>(std::move(ranges)),
	stream(std::move(stream))
{

}

template <typename...Args>
auto make_file_opt(Args&&...args)
{
	using fstream_t = std::tuple_element_t<0,std::tuple<Args...>>;
	return file_opt<fstream_t>(std::forward<Args>(args)...);
}

namespace operators
{

auto operator| (std::string_view file_name, const file_range &range)
{
	return make_file_opt(file_name, range);
}

auto operator| (std::string_view file_name, file_ranges ranges)
{
	return make_file_opt(file_name, std::move(ranges));
}

auto operator| (concepts::fiostream auto &&stream, const file_range &range)
{
	using fstream_t = std::remove_cvref_t<decltype(stream)>;
	return make_file_opt(std::forward<fstream_t>(stream), range);
}

auto operator| (concepts::fiostream auto &&stream, file_ranges ranges)
{
	using fstream_t = std::remove_cvref_t<decltype(stream)>;
	return make_file_opt(std::forward<fstream_t>(stream), std::move(ranges));
}

}} //namespace libgs::http


#endif //LIBGS_HTTP_DETAIL_OPT_TOKEN_H