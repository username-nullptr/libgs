
/************************************************************************************
*                                                                                   *
*   Copyright (c) 2025-2026 Xiaoqiang <username_nullptr@163.com>                    *
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
#include <libgs/core/system/app_utls.h>
#include <iostream>
#include <time.h>

namespace libgs::utils
{

using self_level_t = logger::level_t;
using time_mode_t = logger::time_mode_t;
using spd_level_t = spdlog::level::level_enum;

static constexpr auto
	g_daily_log    = ".daily_log"   ,
	g_warning_log  = ".warning_log" ,
	g_error_log    = ".error_log"   ,
	g_critical_log = ".critical_log";

class LIBGS_DECL_HIDDEN logger::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)
	using logger_ptr = std::shared_ptr<spdlog::logger>;
	using sink_ptr = spdlog::sink_ptr;

public:
	explicit impl(std::string name) :
		m_name(std::move(name))
	{
		m_loggers[0] = spdlog::default_logger()->clone(m_name);
		set_config({});
	}

	~impl()
	{
		for(auto &logger : m_loggers)
		{
			if( logger )
				spdlog::drop(logger->name());
		}
	}

public:
	void set_config(config_t conf) noexcept
	{
		// std::cerr << ">>>>>>>>>>++++++++++++------------------------- " << m_name << " : " << m_loggers[0]->name() << std::endl;

		set_logger(m_loggers[0],
			spd_level_t::info, spd_level_t::warn, conf.time_mode
		);
		if( m_config.path != conf.path )
		{
			for(size_t i=1; i<5; i++)
			{
				if( not m_loggers[i] )
					continue;

				spdlog::drop(m_loggers[i]->name());
				m_loggers[i] = {};
			}
			if( not conf.path.empty() )
			{
				auto path = app::absolute_path(conf.path).or_else()->ptostr() + PCHAR("/");

				m_loggers[1] = spdlog::daily_logger_mt<spdlog::async_factory>(
					m_name + g_daily_log, path + PCHAR("daily/daily.log")
				);
				m_loggers[2] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_warning_log, path + PCHAR("warning.log"),
					conf.max_file_size.warning, conf.max_file_count.warning
				);
				m_loggers[3] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_error_log, path + PCHAR("error.log"),
					conf.max_file_size.error, conf.max_file_count.error
				);
				m_loggers[4] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_critical_log, path + PCHAR("critical.log"),
					conf.max_file_size.critical, conf.max_file_count.critical
				);
			}
		}
		if( m_loggers[1] )
		{
			set_logger(m_loggers[1],
				conf_level(conf.level.daily), spd_level_t::warn, conf.time_mode
			);
		}
		if( m_loggers[2] )
		{
			set_logger(m_loggers[2],
				spd_level_t::warn, spd_level_t::warn, conf.time_mode
			);
		}
		if( m_loggers[3] )
		{
			set_logger(m_loggers[3],
				spd_level_t::err, spd_level_t::err, conf.time_mode
			);
		}
		if( m_loggers[4] )
		{
			set_logger(m_loggers[4],
				spd_level_t::critical, spd_level_t::critical, conf.time_mode
			);
		}
		m_config = std::move(conf);
	}

	[[nodiscard]] static spd_level_t conf_level(self_level_t lv) noexcept
	{
		switch(lv)
		{
			case self_level_t::off     : return spd_level_t::off     ;
			case self_level_t::critical: return spd_level_t::critical;
			case self_level_t::error   : return spd_level_t::err     ;
			case self_level_t::warning : return spd_level_t::warn    ;
			case self_level_t::info    : return spd_level_t::info    ;
			case self_level_t::debug   : return spd_level_t::debug   ;
			case self_level_t::trace   : return spd_level_t::trace   ;
			default: break;
		}
		return spd_level_t::off;
	}

private:
	class LIBGS_DECL_HIDDEN dy_tz_flag_formatter : public spdlog::custom_flag_formatter
	{
	public:
		void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t &dest) override
		{
			auto local_time = time(nullptr);
			tm tm_gmt {};
#ifdef _WIN32
			gmtime_s(&tm_gmt, &local_time);
#else
			gmtime_r(&local_time, &tm_gmt);
#endif
			auto gmt_time = mktime(&tm_gmt);
			auto offset_seconds = local_time - gmt_time;
			auto minutes = std::abs(offset_seconds) / 60;

			auto text = std::format("{}{:02d}:{:02d}",
				offset_seconds >= 0 ? "+" : "-",
				minutes / 60, minutes - minutes / 60 * 60
			);
			dest.append(text.begin(), text.end());
		}

		[[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override {
			return spdlog::details::make_unique<dy_tz_flag_formatter>();
		}
	};

	class LIBGS_DECL_HIDDEN logger_name_flag_formatter : public spdlog::custom_flag_formatter
	{
	public:
		void format(const spdlog::details::log_msg &ctx, const std::tm&, spdlog::memory_buf_t &dest) override {
			dest.append(ctx.logger_name.begin(), ctx.logger_name.end());
		}
		[[nodiscard]] std::unique_ptr<custom_flag_formatter> clone() const override {
			return spdlog::details::make_unique<logger_name_flag_formatter>();
		}
	};

	static void set_logger(const std::shared_ptr<spdlog::logger> &logger,
		spd_level_t level, spd_level_t flush_level, time_mode_t time_mode) noexcept
	{
		logger->set_level(level);
		logger->flush_on(flush_level);

		std::unique_ptr<spdlog::pattern_formatter> formatter {};
		if( time_mode == time_mode_t::utc )
		{
			formatter = std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::utc);
			formatter->add_flag<logger_name_flag_formatter>('+');
			formatter->set_pattern("[%^%l%$]-[UTC %Y-%m-%d %H:%M:%S.%e]-[%+][%s:%#] %v");
		}
		else if( time_mode == time_mode_t::local )
		{
			formatter = std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::local);
			formatter->add_flag<logger_name_flag_formatter>('+');
			formatter->set_pattern("[%^%l%$]-[Local %Y-%m-%d %H:%M:%S.%e]-[%+][%s:%#] %v");
		}
		else if( time_mode == time_mode_t::utc_tz )
		{
			formatter = std::make_unique<spdlog::pattern_formatter>(
				spdlog::pattern_time_type::utc
			);
			formatter->add_flag<dy_tz_flag_formatter>('*');
			formatter->add_flag<logger_name_flag_formatter>('+');
			formatter->set_pattern("[%^%l%$]-[UTC %Y-%m-%d %H:%M:%S.%e %*]-[%+][%s:%#] %v");
		}
		else /* if( time_mode == time_mode_t::local_tz ) */
		{
			formatter = std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::local);
			formatter->add_flag<dy_tz_flag_formatter>('*');
			formatter->add_flag<logger_name_flag_formatter>('+');
			formatter->set_pattern("[%^%l%$]-[Local %Y-%m-%d %H:%M:%S.%e UTC%*]-[%+][%s:%#] %v");
		}
		logger->set_formatter(std::move(formatter));
	}

public:
	logger_ptr m_loggers[5] {};
	std::string m_name {};
	config_t m_config {};
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
	if( create )
	{
		spin_shared_unique_lock locker(g_instances_lock);
		auto [it, inserted] = g_instances.emplace(_name, nullptr);
		if( inserted )
		{
			locker.unlock();
			auto obj = logger_ptr(new logger(std::move(_name)), no_deleter());

			locker.lock();
			it->second = std::move(obj);
		}
		return *it->second;
	}
	spin_shared_unique_lock locker(g_instances_lock);
	auto it = g_instances.find(_name);

	if( it != g_instances.end() )
		return *it->second;
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

logger &logger::set_config(config_t conf)
{
	m_impl->set_config(std::move(conf));
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
	spdlog::source_loc src_loc {loc.file, loc.line, loc.func};
	for(auto &logger : m_impl->m_loggers)
	{
		if( not logger )
			continue;

		logger->log(src_loc, impl::conf_level(lv), m_impl->m_config.line_break ?
			std::format(": \n{}\n", strtls::trimmed(msg)) :
			std::format(": {}", strtls::trimmed(msg))
		);
		logger->flush();
	}
}

} //namespace libgs::utils