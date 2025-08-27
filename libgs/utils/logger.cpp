
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

#ifdef _WIN32
# define SPDLOG_WCHAR_FILENAMES
# define PCHAR(s)  LIBGS_WCHAR(s)
# define ptostr    wstring
#else
# define PCHAR(s)  s
# define ptostr    string
#endif //_WIN32

#include "logger.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>

#include <libgs/core/shared_mutex.h>
#include <libgs/core/app_utls.h>
#include <iostream>

namespace libgs::utils
{

class LIBGS_DECL_HIDDEN logger::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	explicit impl(std::string name) :
		m_name(std::move(name)) {}

	std::string m_name;
	config_t m_config;
};

logger::source_loc::source_loc(const char *file, const char *func, int line) :
	file(file), func(func), line(line)
{

}

logger::logger(std::string name) :
	m_impl(new impl(std::move(name)))
{

}

logger::~logger()
{
	delete m_impl;
}

struct LIBGS_DECL_HIDDEN no_deleter {
	void operator()(logger*) const {}
};
using logger_ptr = std::unique_ptr<logger, no_deleter>;

static std::map<std::string, logger_ptr> g_instances;
static spin_shared_mutex g_instances_lock;

std::vector<std::string> logger::names() noexcept
{
	std::vector<std::string> names;
	spin_shared_unique_lock locker(g_instances_lock);
	for(auto &pair : g_instances)
		names.emplace_back(pair.first);
	return names;
}

logger &logger::instance(std::string_view name, bool create)
{
	std::string _name(name.data(), name.size());
	spin_shared_unique_lock locker(g_instances_lock);

	if( auto [it, inserted] = g_instances.emplace(_name, nullptr); not inserted )
		return *it->second;

	else if( create )
	{
		auto obj = new logger(std::move(_name));
		it->second = logger_ptr(obj, no_deleter());
		return *it->second;
	}
	locker.unlock();

	throw runtime_error (
		"libgs::utils::logger::instance: Instance '{}' is not exist.", name
	);
	// return {};
}

logger &logger::instance()
{
	return instance("default");
}

using self_level = logger::level_t;
using spd_level = spdlog::level::level_enum;

spd_level conflevel(self_level lv)
{
	switch(lv)
	{
	case self_level::off     : return spd_level::off     ;
	case self_level::critical: return spd_level::critical;
	case self_level::error   : return spd_level::err     ;
	case self_level::warning : return spd_level::warn    ;
	case self_level::info    : return spd_level::info    ;
	case self_level::debug   : return spd_level::debug   ;
	case self_level::trace   : return spd_level::trace   ;
	default: break;
	}
	return spd_level::off;
}

static void set_logger
(const std::shared_ptr<spdlog::logger> &logger, spd_level level, spd_level flush_level, const char *errmsg)
{
	if( not logger )
        std::cerr << "libsepp: Log: " << errmsg << "." << std::endl;

	logger->set_level(level);
	logger->flush_on(flush_level);
}

static constexpr auto
	g_daily_log    = ".daily_log"   ,
	g_warning_log  = ".warning_log" ,
	g_error_log    = ".error_log"   ,
	g_critical_log = ".critical_log";

logger &logger::set_config(config_t conf)
{
    set_logger(spdlog::default_logger(),
    	spd_level::info, spd_level::warn, "Default logger is null"
    );
	if( conf.path.empty() )
		return *this;

	auto path = app::absolute_path(conf.path).or_else()->ptostr() + PCHAR("/");
	std::string suffix(name());

    auto file_name = path + PCHAR("daily/daily.log");
    auto logger = spdlog::daily_logger_mt<spdlog::async_factory>(suffix + g_daily_log, file_name);
    set_logger(logger,
    	conflevel(conf.level.daily), spd_level::warn, "Daily logger create failed"
    );
    file_name = path + PCHAR("warning.log");
    logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
    	suffix + g_warning_log, file_name, conf.max_file_size.warning, conf.max_file_count.warning
    );
    set_logger(logger,
    	spd_level::warn, spd_level::warn, "Warning logger create failed"
    );
    file_name = path + PCHAR("error.log");
    logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
    	suffix + g_error_log, file_name, conf.max_file_size.error, conf.max_file_count.error
    );
    set_logger(logger,
    	spd_level::err, spd_level::err, "Error logger create failed"
    );
    file_name = path + PCHAR("critical.log");
    logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
    	suffix + g_critical_log, file_name, conf.max_file_size.critical, conf.max_file_count.critical
    );
    set_logger(logger,
    	spd_level::critical, spd_level::critical, "Critical logger create failed"
    );
	m_impl->m_config = std::move(conf);
	return *this;
}

logger::config_t logger::config() const noexcept
{
	return m_impl->m_config;
}

std::string_view logger::name() const noexcept
{
	return m_impl->m_name;
}

void logger::_log(level_t lv, const source_loc &loc, std::string_view msg) const
{
	std::string suffix(name());
	std::vector loggers {
		spdlog::default_logger(),
		spdlog::get(suffix + g_daily_log)
	};
	if( lv == level_t::warning )
		loggers.emplace_back(spdlog::get(suffix + g_warning_log));
	else if( lv == level_t::error )
		loggers.emplace_back(spdlog::get(suffix + g_error_log));
	else if( lv == level_t::critical )
		loggers.emplace_back(spdlog::get(suffix + g_critical_log));

	spdlog::source_loc src_loc {loc.file, loc.line, loc.func};
	for(auto &logger : loggers)
	{
		if( not logger )
			continue;
		logger->log(src_loc, conflevel(lv), std::format("[{}]: {}", name(), msg));
		logger->flush();
	}
}

} //namespace libgs::utils
