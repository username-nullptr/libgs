
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

#ifndef LIBGS_DBI_MANAGEMENT_H
#define LIBGS_DBI_MANAGEMENT_H

#include <libgs/dbi/result_iterator.h>

namespace libgs::dbi
{

//template <concept_char_type CharT>
//class LIBGS_DBI_TAPI basic_manager
//{
//public:
//	static void register_driver(dbi::driver *driver, bool as_default = false) noexcept(false);
//	static void unregister_driver(const std::string &name);
//	static void unregister_driver(dbi::driver *driver);
//
//public:
//	static dbi::driver &driver(const std::string &name = "");
//	static void set_default_driver(dbi::driver *driver);
//	static void set_default_driver(const std::string &name);
//};

} //namespace libgs::dbi
#include <libgs/dbi/detail/management.h>


#endif //LIBGS_DBI_MANAGEMENT_H
