
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

#ifndef LIBGS_CORE_ARGS_PARSER_H
#define LIBGS_CORE_ARGS_PARSER_H

#include <libgs/core/value.h>
#include <libgs/core/string_list.h>

namespace libgs::cmdline
{

class args_parser_impl;

class LIBGS_CORE_API args_parser
{
	LIBGS_DISABLE_COPY_MOVE(args_parser)

public:
	using arguments = std::unordered_map<std::string, libgs::value>;

public:
	explicit args_parser(std::string help_title = {});
	~args_parser();

public:
	args_parser &set_help_title(std::string text);

public:
	// ./a.out -f filename
	// ./a.out --file=filename
	args_parser &add_group(const std::string &rule, const std::string &description, const std::string &identification = {});

	// ./a.out -abc
	// ./a.out -a -b -c
	args_parser &add_flag(const std::string &rule, const std::string &description, const std::string &identification = {});

public:
	args_parser &set_version(std::string d);
	args_parser &set_v(std::string d);

public:
	args_parser &enable_h();
	args_parser &disable_h();
	args_parser &set_help_extension(std::string d);

public:
	arguments parsing(int argc, const char *argv[], string_list &other);
	arguments parsing(int argc, const char *argv[]); // exit if other.

private:
	args_parser_impl *m_impl;
};

} //namespace libgs::cmdline

LIBGS_CORE_API bool operator&(const libgs::cmdline::args_parser::arguments &args_hash, const std::string &key);
LIBGS_CORE_API bool operator&(const std::string &key, const libgs::cmdline::args_parser::arguments &args_hash);


#endif //LIBGS_CORE_ARGS_PARSER_H
