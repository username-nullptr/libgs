// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/utils/logger.h>
#include <libgs/utils/process.h>
#include <libgs/utils/sbus.h>
#include <libgs/utils/settings.h>

#include <fstream>
#include <future>

namespace
{

using namespace std::chrono_literals;

static_assert(libgs::test::canonical_executor_type<libgs::utils::process>);

bool wait_for(const std::atomic_bool &state)
{
	for(int retry = 0; retry < 200 and not state; ++retry)
		std::this_thread::sleep_for(1ms);
	return state;
}

template <typename T>
T wait_for(std::future<T> &future)
{
	auto &context = libgs::io_context();
	while( future.wait_for(1ms) != std::future_status::ready )
	{
		context.poll();
		context.restart();
	}
	return future.get();
}

void settings_persistence_and_signals()
{
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "settings.ini";
	auto &settings = libgs::utils::settings::instance("libgs-test-runtime");
	bool loaded = false;
	std::string changed_key;
	libgs::value changed_value;
	settings.loaded.connect([&](const libgs::error_code &error) {
		LIBGS_TEST_CHECK(not error);
		loaded = true;
	});
	settings.changed.connect([&](std::string_view key, libgs::value value)
	{
		changed_key = key;
		changed_value = std::move(value);
	});

	LIBGS_TEST_CHECK(settings.load_or(file));
	LIBGS_TEST_CHECK(loaded);
	settings.set("server/port", 8080);
	LIBGS_TEST_CHECK_EQ(changed_key, "server/port");
	LIBGS_TEST_CHECK_EQ(changed_value.to_int().value_or(0), 8080);
	LIBGS_TEST_CHECK_EQ(settings.get("server/port")->to_int().value_or(0), 8080);
	LIBGS_TEST_CHECK(settings.sync());
	LIBGS_TEST_CHECK(std::filesystem::is_regular_file(file));
	LIBGS_TEST_CHECK_EQ(settings.file_name(), file);

	const auto names = libgs::utils::settings::names();
	LIBGS_TEST_CHECK(std::ranges::find(names, "libgs-test-runtime") != names.end());
	settings.changed.disconnect();
	settings.loaded.disconnect();
}

void settings_io_tokens()
{
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "settings-tokens.ini";
	const auto missing = directory.path() / "missing.ini";
	auto &settings = libgs::utils::settings::instance("libgs-test-settings-tokens");
	static_assert(std::same_as<decltype(settings.load(missing)),libgs::sys_expected<>>);
	static_assert(std::same_as<decltype(settings.sync(file)),libgs::sys_expected<>>);
	static_assert(std::same_as<
		decltype(settings.load(missing, libgs::use_future)),
		std::future<libgs::sys_expected<>>
	>);

	std::atomic_int loaded_count {0};
	std::atomic_int synced_count {0};
	libgs::error_code loaded_error;
	libgs::error_code synced_error;
	settings.loaded.connect([&](libgs::error_code error) {
		loaded_error = error;
		loaded_count++;
	});
	settings.synced.connect([&](libgs::error_code error) {
		synced_error = error;
		synced_count++;
	});

	settings.set("tokens/value", 17);
	auto sync_future = settings.sync(file, libgs::use_future);
	const auto sync_result = wait_for(sync_future);
	LIBGS_TEST_CHECK(sync_result);
	LIBGS_TEST_CHECK(not synced_error);
	LIBGS_TEST_CHECK_EQ(synced_count.load(), 1);

	const auto direct_result = settings.load(missing);
	LIBGS_TEST_CHECK(not direct_result);
	LIBGS_TEST_CHECK_EQ(direct_result.error(),
		std::make_error_code(std::errc::no_such_file_or_directory));
	libgs::error_code token_error;
	settings.load(missing, token_error);
	LIBGS_TEST_CHECK_EQ(token_error, direct_result.error());

	auto load_future = settings.load(missing, libgs::use_future);
	const auto missing_result = wait_for(load_future);
	LIBGS_TEST_CHECK(not missing_result);
	LIBGS_TEST_CHECK_EQ(missing_result.error(),
		std::make_error_code(std::errc::no_such_file_or_directory));
	LIBGS_TEST_CHECK_EQ(loaded_error, missing_result.error());

	auto awaitable_future = asio::co_spawn(libgs::io_context(),
	[&]() -> libgs::awaitable<libgs::sys_expected<>>
	{
		co_return co_await settings.load(missing, libgs::use_awaitable);
	}, libgs::use_future);
	const auto awaitable_result = wait_for(awaitable_future);
	LIBGS_TEST_CHECK(not awaitable_result);
	LIBGS_TEST_CHECK_EQ(awaitable_result.error(), missing_result.error());

	auto deferred_load = settings.load(file, libgs::deferred);
	auto deferred_future = std::move(deferred_load)(libgs::use_future);
	LIBGS_TEST_CHECK(wait_for(deferred_future));

	std::promise<libgs::error_code> callback_result;
	settings.load(file, [&](libgs::error_code error) {
		callback_result.set_value(error);
	});
	auto callback_future = callback_result.get_future();
	LIBGS_TEST_CHECK(not wait_for(callback_future));
	LIBGS_TEST_CHECK_EQ(settings.get("tokens/value")->to_int().value_or(0), 17);

	libgs::io_context_t completion_context;
	std::promise<libgs::error_code> associated_result;
	auto associated_future = associated_result.get_future();
	std::thread::id completion_thread;
	std::thread::id callback_thread;
	settings.load(file, asio::bind_executor(completion_context.get_executor(),
	[&](libgs::error_code error)
	{
		callback_thread = std::this_thread::get_id();
		associated_result.set_value(error);
	}));
	std::thread completion_runner([&]
	{
		completion_thread = std::this_thread::get_id();
		completion_context.run();
	});
	LIBGS_TEST_CHECK(not wait_for(associated_future));
	completion_runner.join();
	LIBGS_TEST_CHECK(callback_thread == completion_thread);

	const auto before_detached = loaded_count.load();
	settings.load_or(file, libgs::detached);
	for(int retry = 0; retry < 200 and loaded_count == before_detached; ++retry)
	{
		libgs::io_context().poll();
		libgs::io_context().restart();
		std::this_thread::sleep_for(1ms);
	}
	LIBGS_TEST_CHECK(loaded_count > before_detached);
	LIBGS_TEST_CHECK(not loaded_error);

	settings.loaded.disconnect();
	settings.synced.disconnect();
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

void child_process_completed_single_byte_read()
{
#if defined(__unix__)
	libgs::utils::process process("/bin/sh", "-c", "printf x");
	LIBGS_TEST_CHECK_EQ(process.run(), 0);

	std::array<char,8> output {};
	std::error_code error;
	const auto size = process.read(asio::buffer(output), error);

	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(size, 1);
	LIBGS_TEST_CHECK_EQ(output[0], 'x');

	LIBGS_TEST_CHECK_EQ(process.read(asio::buffer(output), error), 0);
	LIBGS_TEST_CHECK_EQ(error,
		asio::error::make_error_code(asio::error::eof));
#endif
}

void child_process_environment_and_channels()
{
	libgs::test::temporary_directory directory;
	libgs::utils::process process;
	process.set_work_path(directory.path());
	process.setenv("LIBGS_PROCESS_VALUE", "from-child");
#if defined(_WIN32)
	auto started = process.start("cmd.exe", "/C",
		"echo %LIBGS_PROCESS_VALUE% & cd & echo stderr-value 1>&2");
#else
	auto started = process.start("/bin/sh", "-c",
		"printf '%s\\n' \"$LIBGS_PROCESS_VALUE\"; pwd; printf 'stderr-value\\n' >&2");
#endif
	LIBGS_TEST_CHECK(started);
	const auto output = process.read<std::string>();
	const auto errors = process.read_stderr<std::string>();
	LIBGS_TEST_CHECK_EQ(process.join(), 0);
	LIBGS_TEST_CHECK(output.find("from-child") != std::string::npos);
	LIBGS_TEST_CHECK(output.find(directory.path().string()) != std::string::npos);
	LIBGS_TEST_CHECK(errors.find("stderr-value") != std::string::npos);

	process.unsetenv("LIBGS_PROCESS_VALUE");
#if defined(_WIN32)
	const libgs::utils::process::args_t args {"/C", "exit", "7"};
	LIBGS_TEST_CHECK_EQ(libgs::utils::process::exec("cmd.exe", args).value_or(-1), 7);
	LIBGS_TEST_CHECK_EQ(
		libgs::utils::process::exec("cmd.exe /C \"exit 6\"").value_or(-1), 6);
#else
	const libgs::utils::process::args_t args {"-c", "exit 7"};
	LIBGS_TEST_CHECK_EQ(libgs::utils::process::exec("/bin/sh", args).value_or(-1), 7);
	LIBGS_TEST_CHECK_EQ(
		libgs::utils::process::exec("/bin/sh -c 'exit 6'").value_or(-1), 6);
#endif
}

void child_process_state_errors()
{
	using namespace std::chrono_literals;
	libgs::utils::process process;
	LIBGS_TEST_CHECK_EQ(process.state(), libgs::utils::process_state::idle);
	LIBGS_TEST_CHECK(not process.joinable());

#if defined(_WIN32)
	LIBGS_TEST_CHECK(process.start("cmd.exe", "/C", "ping -n 2 127.0.0.1 >nul"));
#else
	LIBGS_TEST_CHECK(process.start("/bin/sh", "-c", "sleep 0.05"));
#endif
	auto duplicate = process.start();
	LIBGS_TEST_CHECK(not duplicate);
	LIBGS_TEST_CHECK(duplicate.error() == std::errc::device_or_resource_busy);
	LIBGS_TEST_CHECK_THROWS(process.join(1ms), std::system_error);
	process.kill();
	std::error_code join_error;
	LIBGS_TEST_CHECK_EQ(process.join(join_error), 0);
	LIBGS_TEST_CHECK(join_error == std::errc::io_error);
	LIBGS_TEST_CHECK_EQ(process.state(), libgs::utils::process_state::crashed);
	LIBGS_TEST_CHECK(process.exit_code() != 0);
	LIBGS_TEST_CHECK(not process.joinable());
}

void child_process_uses_immediate_executor_on_early_error()
{
	libgs::io_context_t process_context;
	libgs::io_context_t completion_context;
	libgs::utils::process process(process_context);

	bool completed = false;
	const std::string input = "not-running";
	process.write(asio::buffer(input),
		asio::bind_immediate_executor(completion_context.get_executor(),
		[&](libgs::error_code error, size_t transferred)
		{
			LIBGS_TEST_CHECK(error);
			LIBGS_TEST_CHECK_EQ(transferred, 0U);
			completed = true;
	}));

	LIBGS_TEST_CHECK(not completed);
	LIBGS_TEST_CHECK_EQ(process_context.poll(), 0U);
	LIBGS_TEST_CHECK(not completed);
	completion_context.run();
	LIBGS_TEST_CHECK(completed);
}

void child_process_cancel_options()
{
	using process_t = libgs::utils::process;
	using cancel_option = process_t::cancel_option;

	auto check = [](cancel_option option, bool remains_joinable)
	{
		libgs::io_context_t context;
	#if defined(_WIN32)
		process_t process(context, "cmd.exe", "/C",
			"ping -n 6 127.0.0.1 >nul");
	#else
		process_t process(context, "/bin/sh", "-c", "sleep 5");
	#endif
		LIBGS_TEST_CHECK(process.start());
		const auto pid = process.pid();
		libgs::error_code run_error;
		auto completed = asio::co_spawn(context,
		[&]() -> libgs::awaitable<void>
		{
			co_await process.join(asio::redirect_error (
				libgs::use_awaitable, run_error
			));
		}, asio::use_future);

		asio::steady_timer timer(context, 5ms);
		timer.async_wait([&](const std::error_code &error)
		{
			if( not error )
				process.cancel(option);
		});
		context.run();
		completed.get();

		LIBGS_TEST_CHECK(run_error);
		LIBGS_TEST_CHECK_EQ(process.joinable(), remains_joinable);
		if( remains_joinable )
		{
			LIBGS_TEST_CHECK_EQ(process.state(), process_t::state_t::running);
			process.cancel(cancel_option::kill);
		}
		else if( option == cancel_option::detach )
		{
			// A detached child keeps running until explicitly stopped by PID.
			LIBGS_TEST_CHECK(process_t::kill(pid).has_value());
		}

		// Let the background monitor reap the released child while its executor
		// is still alive.
		context.restart();
		asio::steady_timer cleanup_delay(context, 50ms);
		cleanup_delay.async_wait([](const std::error_code&) {});
		context.run();
	};

	check(cancel_option::none, true);
	check(cancel_option::terminate, false);
	check(cancel_option::kill, false);
	check(cancel_option::detach, false);
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
	libgs::utils::sbus::publish<libgs::utils::sbus::local_interface>(topic, 42);
	LIBGS_TEST_CHECK(wait_for(received));
	LIBGS_TEST_CHECK_EQ(received_value.load(), 42);

	std::atomic_bool changed = false;
	std::atomic_size_t current_size = 0;
	std::atomic_size_t previous_size = 0;
	std::atomic_int decoded_current = 0;
	std::atomic_bool decoded = false;
	libgs::utils::sbus::local_cache cache(pool);
	cache.changed(topic).connect([&](std::vector<std::byte> current,
		std::vector<std::byte> previous) -> libgs::awaitable<void>
	{
		current_size = current.size();
		previous_size = previous.size();
		changed = true;
		co_return;
	});
	cache.changed(topic).connect([&](int current, int) {
		decoded_current = current;
		decoded = true;
	});
	cache.set(topic, 7);
	LIBGS_TEST_CHECK_EQ(cache.get<int>(topic).value_or(0), 7);
	LIBGS_TEST_CHECK(wait_for(changed));
	LIBGS_TEST_CHECK(wait_for(decoded));
	LIBGS_TEST_CHECK_EQ(decoded_current.load(), 7);
	LIBGS_TEST_CHECK_EQ(current_size.load(), sizeof(int));
	LIBGS_TEST_CHECK(previous_size == 0 or previous_size == sizeof(int));

	received = false;
	subscriber.cancel_sid(sid);
	libgs::utils::sbus::publish<libgs::utils::sbus::local_interface>(topic, 99);
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

void logger_file_flush()
{
	using logger = libgs::utils::logger;
	libgs::test::temporary_directory directory;
	auto &instance = logger::instance("libgs-test-runtime-file");
	logger::config_t config;
	config.path = directory.path();
	config.level.console = logger::level_t::off;
	config.level.daily = logger::level_t::info;
	instance.set_config(config);
	instance.info(
		logger::source_loc(__FILE__, __func__, __LINE__), "  queued message  "
	).flush();

	config.path.clear();
	instance.set_config(config);
	std::string output;
	for(const auto &entry : std::filesystem::recursive_directory_iterator(directory.path()))
	{
		if( not entry.is_regular_file() )
			continue;
		std::ifstream stream(entry.path());
		output.append(
			std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()
		);
	}
	LIBGS_TEST_CHECK(output.find(": queued message") != std::string::npos);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"settings persistence and signals", settings_persistence_and_signals},
		{"settings IO tokens", settings_io_tokens},
		{"child process IO", child_process_io},
		{"completed child single-byte read", child_process_completed_single_byte_read},
		{"child process environment and channels", child_process_environment_and_channels},
		{"child process state errors", child_process_state_errors},
		{"child process immediate executor on early error",
			child_process_uses_immediate_executor_on_early_error},
		{"child process cancel options", child_process_cancel_options},
		{"local message bus", local_message_bus},
		{"logger configuration", logger_configuration},
		{"logger file flush", logger_file_flush},
	});
}
