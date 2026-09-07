#include "test.h"

#include <libgs/utils/logger.h>
#include <libgs/utils/process.h>
#include <libgs/utils/sbus.h>
#include <libgs/utils/settings.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace
{

using namespace std::chrono_literals;

bool wait_for(const std::atomic_bool &state)
{
	for(int retry = 0; retry < 200 and not state; ++retry)
		std::this_thread::sleep_for(1ms);
	return state;
}

void settings_persistence_and_signals()
{
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "settings.ini";
	auto &settings = libgs::utils::settings::instance("libgs-test-runtime");
	bool loaded = false;
	std::string changed_key;
	libgs::value changed_value;
	settings.loaded.connect([&] { loaded = true; });
	settings.changed.connect([&](std::string_view key, libgs::value value)
	{
		changed_key = key;
		changed_value = std::move(value);
	});

	LIBGS_TEST_CHECK(settings.load(file).has_value());
	LIBGS_TEST_CHECK(loaded);
	settings.set("server/port", 8080);
	LIBGS_TEST_CHECK_EQ(changed_key, "server/port");
	LIBGS_TEST_CHECK_EQ(changed_value.to_int().value_or(0), 8080);
	LIBGS_TEST_CHECK_EQ(settings.get("server/port")->to_int().value_or(0), 8080);
	LIBGS_TEST_CHECK(settings.sync().has_value());
	LIBGS_TEST_CHECK(std::filesystem::is_regular_file(file));
	LIBGS_TEST_CHECK_EQ(settings.file_name(), file);

	const auto names = libgs::utils::settings::names();
	LIBGS_TEST_CHECK(std::ranges::find(names, "libgs-test-runtime") != names.end());
	settings.changed.disconnect();
	settings.loaded.disconnect();
}

void child_process_io()
{
	libgs::utils::process process;
#if defined(_WIN32)
	auto started = process.start("cmd.exe", "/C", "echo", "libgs-process-test");
#else
	auto started = process.start("/bin/echo", "libgs-process-test");
#endif
	LIBGS_TEST_CHECK(started.has_value());
	LIBGS_TEST_CHECK(process.joinable());
	LIBGS_TEST_CHECK(process.pid() != 0);
	const auto output = process.read<std::string>();
	const auto exit_code = process.join();
	LIBGS_TEST_CHECK_EQ(exit_code, 0);
	LIBGS_TEST_CHECK(output.find("libgs-process-test") != std::string::npos);
	LIBGS_TEST_CHECK_EQ(process.state(), libgs::utils::process_state::exited);
	LIBGS_TEST_CHECK_EQ(process.exit_code(), 0);
	LIBGS_TEST_CHECK(libgs::utils::process::self_pid().value_or(0) != 0);
}

void local_message_bus()
{
	constexpr std::string_view topic = "libgs.test.counter";
	asio::thread_pool pool(1);
	std::atomic_bool received = false;
	std::atomic_int received_value = 0;
	libgs::utils::sbus::local_subscriber subscriber(pool);
	const auto sid = subscriber.subscribe(topic, [&](int value)
	{
		received_value = value;
		received = true;
	});
	libgs::utils::sbus::publish(topic, 42);
	LIBGS_TEST_CHECK(wait_for(received));
	LIBGS_TEST_CHECK_EQ(received_value.load(), 42);

	std::atomic_bool changed = false;
	std::atomic_size_t current_size = 0;
	std::atomic_size_t previous_size = 0;
	libgs::utils::sbus::local_cache cache(pool);
	cache.changed(topic).connect([&](std::vector<std::byte> current,
		std::vector<std::byte> previous) -> libgs::awaitable<void>
	{
		current_size = current.size();
		previous_size = previous.size();
		changed = true;
		co_return;
	});
	cache.set(topic, 7);
	LIBGS_TEST_CHECK_EQ(cache.get<int>(topic).value_or(0), 7);
	LIBGS_TEST_CHECK(wait_for(changed));
	LIBGS_TEST_CHECK_EQ(current_size.load(), sizeof(int));
	LIBGS_TEST_CHECK(previous_size == 0 or previous_size == sizeof(int));

	received = false;
	subscriber.cancel_sid(sid);
	libgs::utils::sbus::publish(topic, 99);
	std::this_thread::sleep_for(5ms);
	LIBGS_TEST_CHECK(not received);
	cache.changed(topic).disconnect();
	pool.stop();
	pool.join();
}

void logger_configuration()
{
	using logger = libgs::utils::logger;
	auto &instance = logger::instance("libgs-test-runtime");
	logger::config_t config;
	config.level.console = logger::level_t::off;
	config.level.daily = logger::level_t::off;
	config.time_mode = logger::time_mode_t::utc;
	config.line_break = true;
	instance.set_config(config);

	const auto restored = instance.config();
	LIBGS_TEST_CHECK_EQ(restored.level.console, logger::level_t::off);
	LIBGS_TEST_CHECK_EQ(restored.level.daily, logger::level_t::off);
	LIBGS_TEST_CHECK_EQ(restored.time_mode, logger::time_mode_t::utc);
	LIBGS_TEST_CHECK(restored.line_break);
	LIBGS_TEST_CHECK_EQ(instance.name(), "libgs-test-runtime");
	instance.info(logger::source_loc(__FILE__, __func__, __LINE__), "suppressed message");
	const auto names = logger::names();
	LIBGS_TEST_CHECK(std::ranges::find(names, "libgs-test-runtime") != names.end());
}

} //namespace

int main()
{
	return libgs::test::run({
		{"settings persistence and signals", settings_persistence_and_signals},
		{"child process IO", child_process_io},
		{"local message bus", local_message_bus},
		{"logger configuration", logger_configuration},
	});
}
