
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

#ifndef LIBGS_DBI_CONNECTION_H
#define LIBGS_DBI_CONNECTION_H

#include <libgs/dbi/result_iterator.h>

namespace libgs::dbi
{

template <concept_char_type CharT>
class LIBGS_DBI_TAPI connection
{
	LIBGS_DISABLE_COPY_MOVE(connection)

public:
	connection();
	virtual ~connection();

public:
	virtual bool connect(const connect_info &info, error_code &error) noexcept = 0;
	virtual bool connect(const std::string &info, error_code &error) noexcept = 0;
	void connect(const connect_info &info) noexcept(false);
	void connect(const std::string &info) noexcept(false);

public:
	virtual void disconnect(error_code &error) noexcept = 0;
	void disconnect() noexcept(false);

public:
	template <typename...Args>
	connection &prepare(fmt::format_string<Args...> fmt, Args&&...args) noexcept;
	connection &operator<<(const std::string &statement) noexcept;

public:
	virtual void set_binary(std::size_t column, const binary &data) noexcept(false) = 0;
	// ... ...

public:
	virtual bool execute(error_code &error) noexcept = 0;
	void execute() noexcept(false);

public:
	virtual bool roll_back(error_code &error) noexcept = 0;
	void roll_back() noexcept(false);

public:
	virtual bool commit(error_code &error) noexcept = 0;
	void commit() noexcept(false);

public:
	template <typename...Args> GTS_CXX_NODISCARD("Returns a query iterator")
	result_iterator_ptr create_query(error_code &error, fmt::format_string<Args...> fmt, Args&&...args) noexcept;

	template <typename...Args> GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	table_data query(error_code &error, fmt::format_string<Args...> fmt, Args&&...args) noexcept;

	GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	table_data query(error_code &error, const std::string &sql) noexcept;

	template <typename...Args> GTS_CXX_NODISCARD("Returns a query iterator")
	result_iterator_ptr create_query(fmt::format_string<Args...> fmt, Args&&...args) noexcept(false);

	template <typename...Args> GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	table_data query(fmt::format_string<Args...> fmt, Args&&...args) noexcept(false);

	GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	table_data query(const std::string &sql) noexcept(false);

	GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	virtual table_data prepare_query(const std::string &statement, error_code &error) noexcept(false);

public:
	virtual bool set_auto_commit(error_code &error, bool enable) noexcept = 0;
	void set_auto_commit(bool enable = true) noexcept(false);
	GTS_CXX_NODISCARD("") virtual bool auto_commit() const = 0;

public:
	virtual connection &set_query_string_buffer_size(std::size_t size);
	GTS_CXX_NODISCARD("") virtual std::size_t query_string_buffer_size() const;
	GTS_CXX_NODISCARD("") virtual dbi::driver &driver() const = 0;

protected:
	virtual connection &prepare_statement(const std::string &statement) noexcept = 0;

	GTS_CXX_NODISCARD("Returns a 2 dimensional vector")
	virtual result_iterator_ptr create_query_work(const std::string &statement, error_code &error) noexcept = 0;

private:
	connection_impl *m_impl;
};

} //namespace libgs::dbi
#include <libgs/dbi/detail/connection.h>


#endif //LIBGS_DBI_CONNECTION_H
