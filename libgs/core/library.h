
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

#ifndef LIBGS_CORE_LIBRARY_H
#define LIBGS_CORE_LIBRARY_H

#include <libgs/core/global.h>

namespace libgs
{

class LIBGS_CORE_API library
{
	LIBGS_DISABLE_COPY(library)

public:
	explicit library(std::string_view file_name, std::string_view version = {});
	~library();

	library(library &&other) noexcept;
	library &operator=(library &&other) noexcept;

public:
	void load(error_code &error) noexcept;
	void load();

	void unload(error_code &error) noexcept;
	void unload();

public:
	template <concepts::function Func>
	[[nodiscard]] auto interface(std::string_view ifname) const;

	template <concepts::function Func, typename Arg0, typename...Args>
	[[nodiscard]] auto interface(std::format_string<Arg0,Args...> fmt_value, Arg0 &&arg0, Args&&...args) const;

	[[nodiscard]] void *interface(std::string_view ifname) const;
	[[nodiscard]] bool exists(std::string_view ifname) const;

public:
	[[nodiscard]] bool is_loaded() const noexcept;
	[[nodiscard]] std::string_view file_name() const noexcept;

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs
#include <libgs/core/detail/library.h>


#endif //LIBGS_CORE_LIBRARY_H
