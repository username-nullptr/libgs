// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/core/args_parser.h>
#include <libgs/core/ini.h>
#include <libgs/core/lock_free_queue.h>
#include <libgs/core/mime_type.h>
#include <libgs/core/string_vector.h>
#include <libgs/core/system/app_utls.h>
#include <libgs/core/url.h>
#include <libgs/core/value.h>

#include <atomic>
#include <fstream>
#include <numeric>
#include <thread>

namespace
{

void values()
{
	libgs::value integer = "42";
	LIBGS_TEST_CHECK_EQ(integer.to_int().value_or(0), 42);
	LIBGS_TEST_CHECK_EQ(integer.get<unsigned>().value_or(0), 42U);

	libgs::value boolean = "true";
	LIBGS_TEST_CHECK(boolean.to_bool().value_or(false));
	LIBGS_TEST_CHECK(not libgs::value("not-a-number").to_double());

	libgs::value formatted("{} + {} = {}", 20, 22, 42);
	LIBGS_TEST_CHECK_EQ(formatted.to_string(), "20 + 22 = 42");
	formatted.set(3.5);
	LIBGS_TEST_CHECK_EQ(formatted.to_double().value_or(0.0), 3.5);
}

void string_containers()
{
	const auto words = libgs::string_vector::from_string("alpha,,beta,gamma", ',', true);
	LIBGS_TEST_CHECK_EQ(words.size(), 3U);
	LIBGS_TEST_CHECK_EQ(words.join('|'), "alpha|beta|gamma");
	LIBGS_TEST_CHECK_EQ(words.join(1, 2, '/'), "beta/gamma");

	const auto with_empty = libgs::string_vector::from_string("a,,b", ',', false);
	LIBGS_TEST_CHECK_EQ(with_empty.size(), 3U);
	LIBGS_TEST_CHECK(with_empty[1].empty());

	const libgs::string_vector empty;
	LIBGS_TEST_CHECK(empty.join(',').empty());
}

void url_parsing()
{
	const libgs::url local;
	LIBGS_TEST_CHECK(local.is_valid());
	LIBGS_TEST_CHECK_EQ(local.protocol(), "local");
	LIBGS_TEST_CHECK(local.host().empty());
	LIBGS_TEST_CHECK_EQ(local.port(), 0);
	LIBGS_TEST_CHECK_EQ(local.to_string(), "local:///");

	const libgs::url file("file:///tmp/libgs.txt");
	LIBGS_TEST_CHECK(file.is_valid());
	LIBGS_TEST_CHECK(file.host().empty());
	LIBGS_TEST_CHECK_EQ(file.port(), 0);
	LIBGS_TEST_CHECK_EQ(file.to_string(), "file:///tmp/libgs.txt");

	const libgs::url ftp("ftp://example.test/pub");
	LIBGS_TEST_CHECK(ftp.is_valid());
	// URL parsing is protocol-neutral; default ports belong to protocol modules.
	LIBGS_TEST_CHECK_EQ(ftp.port(), 0);

	libgs::url target("HTTPS://example.test:8443/a%20b/items?q=hello%20world&flag");
	LIBGS_TEST_CHECK(target.is_valid());
	LIBGS_TEST_CHECK_EQ(target.protocol(), "https");
	LIBGS_TEST_CHECK_EQ(target.host(), "example.test");
	LIBGS_TEST_CHECK_EQ(target.port(), 8443);
	LIBGS_TEST_CHECK_EQ(target.path(), "/a b/items");
	LIBGS_TEST_CHECK(target.contains_parameter("q", "hello world"));
	LIBGS_TEST_CHECK(target.contains_parameter("flag", "flag"));
	target.set_parameter("page", 2).unset_parameter("flag");
	LIBGS_TEST_CHECK_EQ(target.parameter("page")->to_int().value_or(0), 2);
	LIBGS_TEST_CHECK(not target.contains_parameter("flag"));
	LIBGS_TEST_CHECK_EQ(
		target.to_string(),
		"https://example.test:8443/a%20b/items?q=hello%20world&page=2"
	);

	const libgs::url ipv6("http://[::1]/health");
	LIBGS_TEST_CHECK(ipv6.is_valid());
	LIBGS_TEST_CHECK_EQ(ipv6.host(), "::1");
	LIBGS_TEST_CHECK_EQ(ipv6.port(), 0);
	LIBGS_TEST_CHECK_EQ(ipv6.to_string(), "http://[::1]/health");

	libgs::url preserved("wss://example.test/a%2Fb//c?flag&empty=#part%2Fone");
	LIBGS_TEST_CHECK_EQ(preserved.encoded_path(), "/a%2Fb//c");
	LIBGS_TEST_CHECK(preserved.has_query());
	LIBGS_TEST_CHECK_EQ(preserved.encoded_query(), "flag&empty=");
	LIBGS_TEST_CHECK(preserved.has_fragment());
	LIBGS_TEST_CHECK_EQ(preserved.fragment(), "part/one");
	LIBGS_TEST_CHECK_EQ(
		preserved.to_string(),
		"wss://example.test/a%2Fb//c?flag&empty=#part%2Fone"
	);
	preserved.clear_fragment().set_fragment("next/value");
	LIBGS_TEST_CHECK_EQ(
		preserved.to_string(),
		"wss://example.test/a%2Fb//c?flag&empty=#next/value"
	);

	const libgs::url invalid("example.test/no-scheme");
	LIBGS_TEST_CHECK(not invalid.is_valid());
	LIBGS_TEST_CHECK(invalid.to_string().empty());

	const libgs::url invalid_scheme("ht*tp://example.test/");
	LIBGS_TEST_CHECK(not invalid_scheme.is_valid());

	const libgs::url invalid_escape("https://example.test/a%2");
	LIBGS_TEST_CHECK(not invalid_escape.is_valid());
	LIBGS_TEST_CHECK_EQ(
		libgs::url("https://example.test/a b?q=hello world").to_string(),
		"https://example.test/a%20b?q=hello%20world"
	);
}

void url_resolution()
{
	const libgs::url base("https://example.test:443/a/b/index.html?old=1");

	LIBGS_TEST_CHECK_EQ(
		libgs::url::resolve(base, "../image.png").to_string(),
		"https://example.test:443/a/image.png"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::url::resolve(base, "/status?q=ok").to_string(),
		"https://example.test:443/status?q=ok"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::url::resolve(base, "?fresh=1").to_string(),
		"https://example.test:443/a/b/index.html?fresh=1"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::url::resolve(base, "http://other.test/x").to_string(),
		"http://other.test/x"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::url::resolve(base, "#section").to_string(),
		"https://example.test:443/a/b/index.html?old=1#section"
	);
}

void command_line_parsing()
{
	libgs::string_vector positional;
	auto parsed = libgs::cmdline::args_parser("test")
		.add_group("-o,--output", "output path", "output")
		.add_flag("-v,--verbose", "verbose", "verbose")
		.add_flag("-q,--quiet", "quiet", "quiet")
		.parsing({"program", "--output=result.txt", "-vq", "input.dat"}, positional);

	LIBGS_TEST_CHECK(parsed & "output");
	LIBGS_TEST_CHECK(parsed & "verbose");
	LIBGS_TEST_CHECK(parsed & "quiet");
	LIBGS_TEST_CHECK_EQ(parsed.at("output").to_string(), "result.txt");
	LIBGS_TEST_CHECK_EQ(positional.size(), 1U);
	LIBGS_TEST_CHECK_EQ(positional.front(), "input.dat");
}

void ini_memory_and_file()
{
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "settings.ini";

	libgs::ini config(file);
	config.write("server/host", "127.0.0.1");
	config.write({"server", "port"}, 8080);
	config["feature"]["enabled"] = true;
	LIBGS_TEST_CHECK_EQ(config.size(), 2U);
	LIBGS_TEST_CHECK_EQ(config.group("server").size(), 2U);
	LIBGS_TEST_CHECK_EQ(config.read("server/port")->to_int().value_or(0), 8080);

	std::error_code error;
	config.sync(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK(std::filesystem::is_regular_file(file));

	libgs::ini restored(file);
	restored.load(error);
	LIBGS_TEST_CHECK(not error);
	LIBGS_TEST_CHECK_EQ(restored.read("server/host")->to_string(), "127.0.0.1");
	LIBGS_TEST_CHECK(restored.read("feature/enabled")->to_bool().value_or(false));
	LIBGS_TEST_CHECK_EQ(restored.file_name(), file);
}

template <libgs::queue_type Type>
void check_queue_type()
{
	libgs::lock_free_queue<int,Type,3> queue;
	LIBGS_TEST_CHECK(queue.empty());
	LIBGS_TEST_CHECK(queue.emplace(1));
	LIBGS_TEST_CHECK(queue.enqueue(2));
	LIBGS_TEST_CHECK(queue.enqueue(3));
	LIBGS_TEST_CHECK(queue.full());
	LIBGS_TEST_CHECK(not queue.enqueue(4));
	LIBGS_TEST_CHECK_EQ(*queue.dequeue(), 1);
	LIBGS_TEST_CHECK_EQ(queue.force_emplace(4), 0U);
	LIBGS_TEST_CHECK_EQ(queue.force_emplace(5), 1U);
	LIBGS_TEST_CHECK_EQ(*queue.dequeue(), 3);
	LIBGS_TEST_CHECK_EQ(*queue.dequeue(), 4);
	LIBGS_TEST_CHECK_EQ(*queue.dequeue(), 5);
	LIBGS_TEST_CHECK(queue.empty());
}

void lock_free_queues()
{
	check_queue_type<libgs::queue_type::linked>();
	check_queue_type<libgs::queue_type::circular>();

	libgs::circular_lock_free_queue<int> queue(32);
	std::atomic_bool done = false;
	std::atomic_int sum = 0;
	std::thread producer([&]
	{
		for(int value = 1; value <= 1000; ++value)
		{
			while(not queue.enqueue(value))
				std::this_thread::yield();
		}
		done = true;
	});
	std::thread consumer([&]
	{
		while(not done or not queue.empty())
		{
			if(auto value = queue.dequeue())
				sum += *value;
			else
				std::this_thread::yield();
		}
	});
	producer.join();
	consumer.join();
	LIBGS_TEST_CHECK_EQ(sum.load(), 500500);
}

void mime_detection()
{
	libgs::test::temporary_directory directory;
	const auto text = directory.path() / "sample.txt";
	const auto png = directory.path() / "sample.bin";
	{
		std::ofstream stream(text, std::ios::binary);
		stream << "plain UTF-8 text\n";
	}
	{
		std::ofstream stream(png, std::ios::binary);
		const unsigned char signature[] {0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a};
		stream.write(reinterpret_cast<const char*>(signature), sizeof(signature));
	}

	LIBGS_TEST_CHECK_EQ(libgs::mime_type::get(text), "text/plain");
	LIBGS_TEST_CHECK(libgs::mime_type::is_text(text));
	LIBGS_TEST_CHECK_EQ(libgs::mime_type::get(png, true), "image/png");
	LIBGS_TEST_CHECK(libgs::mime_type::is_binary(png));
}

void application_environment()
{
	const auto executable = libgs::app::file_path();
	const auto directory = libgs::app::dir_path();
	const auto current = libgs::app::current_directory();
	LIBGS_TEST_CHECK(executable and not executable->empty());
	LIBGS_TEST_CHECK(directory and std::filesystem::is_directory(*directory));
	LIBGS_TEST_CHECK(current and std::filesystem::is_directory(*current));
	LIBGS_TEST_CHECK(libgs::app::absolute_path(".").has_value());

	constexpr auto key = "LIBGS_TEST_ENVIRONMENT_VALUE";
	LIBGS_TEST_CHECK(libgs::app::setenv(key, "available").has_value());
	LIBGS_TEST_CHECK_EQ(libgs::app::getenv(key).value_or(""), "available");
	LIBGS_TEST_CHECK(libgs::app::unsetenv(key).has_value());
	LIBGS_TEST_CHECK(not libgs::app::getenv(key));
}

} //namespace

int main()
{
	return libgs::test::run({
		{"values", values},
		{"string containers", string_containers},
		{"URL parsing", url_parsing},
		{"URL resolution", url_resolution},
		{"command line parsing", command_line_parsing},
		{"INI memory and file", ini_memory_and_file},
		{"lock-free queues", lock_free_queues},
		{"MIME detection", mime_detection},
		{"application environment", application_environment},
	});
}
