
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2026 Xiaoqiang <username_nullptr@163.com>                         *
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

#ifndef LIBGS_CORE_CXX_DETAIL_STREAMER_CUSTOM_H
#define LIBGS_CORE_CXX_DETAIL_STREAMER_CUSTOM_H

namespace libgs { namespace concepts
{

template <class T>
concept streamer_custom_type = requires(T &v) {
	v.meta_fields();
};

template <class T>
concept streamer_type = requires(T &v)
{
	streamer<T>::encode(v);
	v = *streamer<T>::decode(std::declval<std::vector<std::byte>>());
};

} //namespace concepts

template <concepts::streamer_custom_type T>
struct streamer<T>
{
	[[nodiscard]] static auto encode(const T &v) noexcept
	{
		std::vector<std::byte> buf;
		std::apply([&]<typename...F>(const F&...xs) {
			(helper(buf, streamer<std::remove_cvref_t<F>>::encode(xs)), ...);
		}, v.meta_fields());
		return buf;
	}

	[[nodiscard]] static decoder_data<T> decode(const std::vector<std::byte> &buf, size_t offset = 0)
	{
		size_t sum = 0;
		T v;
		std::apply([&](auto&...xs) {
			(helper(offset, xs, sum, buf), ...);
		}, v.meta_fields());
		return { v, sum };
	}

private:
	static void helper(std::vector<std::byte> &total, std::vector<std::byte> &&sub)
	{
		total.insert(total.end(),
			std::make_move_iterator(sub.begin()),
			std::make_move_iterator(sub.end())
		);
	}

	template <typename F>
	static void helper(size_t &offset, F &fields, size_t &sum, const std::vector<std::byte> &buf)
	{
		auto data = streamer<F>::decode(buf, offset);
		fields = std::move(*data);
		offset += data.size;
		sum += data.size;
	}
};

} //namespace libgs


#endif //LIBGS_CORE_CXX_DETAIL_STREAMER_CUSTOM_H