// SPDX-FileCopyrightText: 2025-2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

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
#include <ctime>

namespace libgs::utils
{

using self_level_t = logger::level_t;
using spd_level_t = spdlog::level::level_enum;

static constexpr auto
	g_daily_log    = ".daily_log"   ,
	g_warning_log  = ".warning_log" ,
	g_error_log    = ".error_log"   ,
	g_critical_log = ".critical_log";

class LIBGS_DECL_HIDDEN logger::impl
{
	LIBGS_DISABLE_COPY_MOVE(impl)

public:
	using logger_ptr = std::shared_ptr<spdlog::logger>;

	explicit impl(std::string name) :
		m_name(std::move(name))
	{
		m_terminal_logger = spdlog::default_logger()->clone(m_name);
		set_config({});
	}

	~impl()
	{
		spdlog::drop(m_terminal_logger->name());
		for(auto &logger : m_file_loggers)
		{
			if( logger )
				spdlog::drop(logger->name());
		}
	}

public:
	[[nodiscard]] bool enabled(spd_level_t level) const noexcept
	{
		if( m_terminal_logger->should_log(level) or
			(m_file_loggers[0] and m_file_loggers[0]->should_log(level)) )
			return true;

		for(size_t index = 1; index < 4; ++index)
		{
			if( m_file_loggers[index] and m_file_loggers[index]->level() == level )
				return true;
		}
		return false;
	}

	void flush()
	{
		m_terminal_logger->flush();
		for(auto &logger : m_file_loggers)
		{
			if( logger )
				logger->flush();
		}
	}

	void set_config(config_t conf) noexcept
	{
		set_logger(m_terminal_logger,
			conf_level(conf.level.console), spd_level_t::warn, conf.time_mode
		);
		if( m_config.path != conf.path )
		{
			for(auto &logger : m_file_loggers)
			{
				if( not logger )
					continue;

				spdlog::drop(logger->name());
				logger = {};
			}
			if( not conf.path.empty() )
			{
				auto path = app::absolute_path(conf.path).or_else()->ptostr() + PCHAR("/");

				m_file_loggers[0] = spdlog::daily_logger_mt<spdlog::async_factory>(
					m_name + g_daily_log, path + PCHAR("daily/daily.log")
				);
				m_file_loggers[1] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_warning_log, path + PCHAR("warning.log"),
					conf.max_file_size.warning, conf.max_file_count.warning
				);
				m_file_loggers[2] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_error_log, path + PCHAR("error.log"),
					conf.max_file_size.error, conf.max_file_count.error
				);
				m_file_loggers[3] = spdlog::rotating_logger_mt<spdlog::async_factory>(
					m_name + g_critical_log, path + PCHAR("critical.log"),
					conf.max_file_size.critical, conf.max_file_count.critical
				);
			}
		}
		if( m_file_loggers[0] )
		{
			set_logger(m_file_loggers[0],
				conf_level(conf.level.daily), spd_level_t::warn, conf.time_mode
			);
		}
		if( m_file_loggers[1] )
		{
			set_logger(m_file_loggers[1],
				spd_level_t::warn, spd_level_t::warn, conf.time_mode
			);
		}
		if( m_file_loggers[2] )
		{
			set_logger(m_file_loggers[2],
				spd_level_t::err, spd_level_t::err, conf.time_mode
			);
		}
		if( m_file_loggers[3] )
		{
			set_logger(m_file_loggers[3],
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
	logger_ptr m_terminal_logger {};
	logger_ptr m_file_loggers[4] {};
	std::string m_name {};
	config_t m_config {};
};

logger::source_loc::source_loc(const char *source_file, const char *source_func, int source_line) :
	file(source_file), func(source_func), line(source_line)
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

namespace
{

struct LIBGS_DECL_HIDDEN no_deleter {
	void operator()(logger*) const {}
};

} //namespace

using logger_ptr = std::unique_ptr<logger, no_deleter>;

static std::map<std::string, logger_ptr, std::less<>> g_instances;
static spin_shared_mutex g_instances_lock;

std::vector<std::string> logger::names() noexcept
{
	std::vector<std::string> names {};
	spin_shared_shared_lock locker(g_instances_lock);

	names.reserve(g_instances.size());
	for(auto &pair : g_instances)
		names.emplace_back(pair.first);
	return names;
}

logger &logger::instance(std::string_view name, bool create)
{
	// Instance objects and map keys remain valid for the process lifetime.
	struct cache_t {
		std::string_view name {};
		logger *object = nullptr;
	};
	thread_local cache_t cache;
	if( cache.object and cache.name == name )
		return *cache.object;
	{
		spin_shared_shared_lock locker(g_instances_lock);
		if( auto it = g_instances.find(name); it != g_instances.end() )
		{
			cache = {it->first, it->second.get()};
			return *it->second;
		}
	}
	if( create )
	{
		spin_shared_unique_lock locker(g_instances_lock);
		if( auto it = g_instances.find(name); it != g_instances.end() )
		{
			cache = {it->first, it->second.get()};
			return *it->second;
		}
		std::string logger_name(name);
		logger_ptr object(new logger(logger_name), no_deleter());

		auto it = g_instances.emplace(std::move(logger_name), std::move(object)).first;
		cache = {it->first, it->second.get()};
		return *it->second;
	}
	runtime_error::loc_throw(std::format (
		"libgs::utils::logger::instance: Instance '{}' is not exist.", name
	));
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

logger &logger::flush()
{
	m_impl->flush();
	return *this;
}

std::string_view logger::name() const noexcept
{
	return m_impl->m_name;
}

bool logger::_enabled(level_t lv) const noexcept
{
	return m_impl->enabled(impl::conf_level(lv));
}

void logger::_log(level_t lv, const source_loc &loc, std::string_view msg) const
{
	spdlog::source_loc src_loc {loc.file, loc.line, loc.func};
	auto conf_lv = impl::conf_level(lv);

	while( not msg.empty() and msg.front() >= 1 and msg.front() <= 32 )
		msg.remove_prefix(1);

	while( not msg.empty() and msg.back() >= 1 and msg.back() <= 32 )
		msg.remove_suffix(1);

	std::string text;
	text.reserve(msg.size() + 4);

	if( m_impl->m_config.line_break )
		text = ": \n";
	else
		text = ": ";

	text.append(msg);
	if( m_impl->m_config.line_break )
		text.push_back('\n');

	if( m_impl->m_terminal_logger->should_log(conf_lv) )
		m_impl->m_terminal_logger->log(src_loc, conf_lv, text);

	if( auto &daily_logger = m_impl->m_file_loggers[0] )
	{
		if( daily_logger->should_log(conf_lv) )
			daily_logger->log(src_loc, conf_lv, text);
	}
	for(size_t i=1; i<4; i++)
	{
		auto &file_logger = m_impl->m_file_loggers[i];
		if( not file_logger or file_logger->level() != conf_lv )
			continue;

		file_logger->log(src_loc, conf_lv, text);
	}
}

void logger::check_level(level_t lv)
{
	if( lv != level_t::trace and
		lv != level_t::debug and
		lv != level_t::info and
		lv != level_t::warning and
		lv != level_t::error and
		lv != level_t::critical )
	{
		runtime_error::loc_throw(std::format (
			"logger: Code bug: Invalid level: {}.", lv
		));
	}
}

} //namespace libgs::utils
