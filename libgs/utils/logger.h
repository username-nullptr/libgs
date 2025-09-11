
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

#ifndef LIBGS_UTILS_LOG_H
#define LIBGS_UTILS_LOG_H

#include <libgs/utils/global.h>

namespace libgs::utils
{

#ifdef _WIN32
# define LIBSEPP_LOG_FILE(_file) ( \
	strrchr(_file, '\\') ? \
		strrchr(_file, '\\') + 1 : \
		strrchr(_file, '/') ? \
			strrchr(_file, '/') + 1 : \
			_file \
)
#else
# define LIBSEPP_LOG_FILE(_file) ( \
	strrchr(_file, '/') ? \
		strrchr(_file, '/') + 1 : _file \
)
#endif

#define libgs_utils_clog(Level, name, ...)    libgs::utils::logger::instance(name).write<Level>(libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_trace(name, ...)     libgs::utils::logger::instance(name).trace       (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_debug(name, ...)     libgs::utils::logger::instance(name).debug       (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_info(name, ...)      libgs::utils::logger::instance(name).info        (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_warning(name, ...)   libgs::utils::logger::instance(name).warning     (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_error(name, ...)     libgs::utils::logger::instance(name).error       (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)
#define libgs_utils_clog_critical(name, ...)  libgs::utils::logger::instance(name).critical    (libgs::utils::logger::source_loc(LIBSEPP_LOG_FILE(__FILE__), __FUNCTION__, __LINE__), __VA_ARGS__)

#define libgs_utils_log(Level, ...)    libgs_utils_clog         (Level, "default", __VA_ARGS__)
#define libgs_utils_log_trace(...)     libgs_utils_clog_trace   (       "default", __VA_ARGS__)
#define libgs_utils_log_debug(...)     libgs_utils_clog_debug   (       "default", __VA_ARGS__)
#define libgs_utils_log_info(...)      libgs_utils_clog_info    (       "default", __VA_ARGS__)
#define libgs_utils_log_warning(...)   libgs_utils_clog_warning (       "default", __VA_ARGS__)
#define libgs_utils_log_error(...)     libgs_utils_clog_error   (       "default", __VA_ARGS__)
#define libgs_utils_log_critical(...)  libgs_utils_clog_critical(       "default", __VA_ARGS__)

class LIBGS_UTILS_API logger
{
	LIBGS_DISABLE_COPY_MOVE(logger)
	explicit logger(std::string name);
	~logger();

public:
	[[nodiscard]] static std::vector<std::string> names() noexcept;
	static logger &instance(std::string_view name, bool create = true);
	static logger &instance(); // "default"

	enum class level_t {
		off, critical, error, warning, info, debug, trace
	};
 	struct config_t
	{
		std::filesystem::path path {};

 		struct {
			level_t console = level_t::info;
			level_t daily = level_t::info;
 		} level;

 		struct {
			size_t warning  = 32 * 1024 * 1024;
			size_t error    = 16 * 1024 * 1024;
			size_t critical =  8 * 1024 * 1024;
 		} max_file_size;

 		struct {
			size_t warning  = 16;
			size_t error    =  8;
			size_t critical =  4;
 		} max_file_count;
	};
	logger &set_config(config_t conf);
	[[nodiscard]] config_t config() const noexcept;

	[[nodiscard]] std::string_view name() const noexcept;

public:
	struct LIBGS_UTILS_API source_loc
	{
        source_loc(const char *file, const char *func, int line);
		const char *file = nullptr;
        const char *func = nullptr;
        int line = 0;
	};

	template <typename...Args>
    using fmt_str_t = std::format_string<Args...>;

public:
	template <level_t Lv, typename Arg0, typename...Args>
	logger &write(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <level_t Lv, typename T>
	logger &write(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &trace(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger &trace(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &debug(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger & debug(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &info(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger &info(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &warning(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger &warning(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &error(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger &error(const source_loc &loc, T &&msg);

public:
	template <typename Arg0, typename...Args>
	logger &critical(const source_loc &loc, fmt_str_t<Arg0,Args...> msg, Arg0 &&arg0, Args&&...args);

	template <typename T>
	logger &critical(const source_loc &loc, T &&msg);

private:
	void _log(level_t lv, const source_loc &loc, std::string_view msg) const;

	template <level_t Lv>
	static consteval void check_level();

private:
	class impl;
	impl *m_impl = nullptr;
};

} //namespace libgs::utils
#include <libgs/utils/detail/logger.h>


#endif //LIBGS_UTILS_LOG_H
