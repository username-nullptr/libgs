
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2024-2025 Xiaoqiang <username_nullptr@163.com>                    *
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

#include <libgs/core/ini.h>

namespace libgs::detail
{

static ini_error_category

g_invalid_group {
	"invalid_group", "Invalid group"
},
g_no_group_specified {
	"no_group_specified", "No group specified"
},
g_invalid_key_value_line {
	"invalid_key_value_line", "Invalid key-value line"
},
g_key_is_empty {
	"invalid_key_value_line", "Invalid key-value line: Key is empty"
},
g_invalid_value {
	"invalid_key_value_line", "Invalid key-value line: Invalid value"
};

ini_error_category &ini_invalid_group() noexcept
{
	return g_invalid_group;
}

ini_error_category &ini_no_group_specified() noexcept
{
	return g_no_group_specified;
}

ini_error_category &ini_invalid_key_value_line() noexcept
{
	return g_invalid_key_value_line;
}

ini_error_category &ini_key_is_empty() noexcept
{
	return g_key_is_empty;
}

ini_error_category &ini_invalid_value() noexcept
{
	return g_invalid_value;
}

static struct io_work
{
	// Don't destruct it.
	// An exception occurs when the main function exits in Win10.
	io_context_t *ioc = new io_context_t();

	using io_worker_t = asio::executor_work_guard<io_context_t::executor_type>;
	io_worker_t io_worker = make_work_guard(*ioc);

	std::thread thread {[this] {
		ioc->run();
	}};
	~io_work()
	{
		io_worker.reset();
		if( not thread.joinable() )
			return ;
		try {
			thread.join();
		} catch(...) {}
	}
}
g_io_work;

using path_t = std::filesystem::path;

path_t ini_tmp_file(const path_t &file_name)
{
#ifdef _WIN32
	return strtls::file_path(file_name.wstring()) + L"libgs.tmp.ini";
#else
	return strtls::file_path(file_name.string()) + ".libgs.tmp.ini";
#endif
}

void ini_commit_io_work(std::function<void()> work)
{
	asio::post(*g_io_work.ioc, std::move(work));
}

} //namespace libgs::detail