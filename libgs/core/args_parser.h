// SPDX-FileCopyrightText: 2024-2025 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#ifndef LIBGS_CORE_ARGS_PARSER_H
#define LIBGS_CORE_ARGS_PARSER_H

#include <libgs/core/string_vector.h>
#include <libgs/core/value.h>
#include <map>

namespace libgs::cmdline
{

class LIBGS_CORE_API args_parser
{
	LIBGS_DISABLE_COPY_MOVE(args_parser)

public:
	using arguments = std::map<std::string, value>;

public:
	explicit args_parser(std::string help_title = {});
	~args_parser();

public:
	args_parser &set_help_title(std::string text);

public:
	// ./a.out -f filename
	// ./a.out --file=filename
	args_parser &add_group (
		std::string_view rule, std::string_view description, std::string_view identification = {}
	);

	// ./a.out -abc
	// ./a.out -a -b -c
	args_parser &add_flag (
		std::string_view rule, std::string_view description, std::string_view identification = {}
	);

public:
	args_parser &set_version(std::string d);
	args_parser &set_v(std::string d);

public:
	args_parser &enable_h();
	args_parser &disable_h();
	args_parser &set_help_extension(std::string d);

public:
	arguments parsing(int argc, const char *argv[], string_vector &other);
	arguments parsing(const string_vector &args, string_vector &other);

	// exit if other.
	arguments parsing(int argc, const char *argv[]);
	arguments parsing(const string_vector &args);

private:
	class impl;
	impl *m_impl;
};

} //namespace libgs::cmdline

LIBGS_CORE_API bool operator&(const libgs::cmdline::args_parser::arguments &args_hash, const std::string &key);
LIBGS_CORE_API bool operator&(const std::string &key, const libgs::cmdline::args_parser::arguments &args_hash);


#endif //LIBGS_CORE_ARGS_PARSER_H
