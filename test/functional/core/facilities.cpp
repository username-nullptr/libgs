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
#include <fstream>
#include <future>

namespace
{

static_assert(libgs::test::canonical_executor_type<libgs::ini>);

struct ini_char_traits : std::char_traits<char> {};

using ini_traits_string = std::basic_string<char,ini_char_traits>;
using ini_traits_view = std::basic_string_view<char,ini_char_traits>;
using ini_keys_t = libgs::ini::ini_keys_t;

static_assert(std::same_as<
	decltype(std::declval<libgs::ini&>()["group"]), ini_keys_t&
>);
static_assert(std::same_as<
	decltype(std::declval<const libgs::ini&>()["group"]), const ini_keys_t&
>);
static_assert(std::same_as<
	decltype(std::declval<ini_keys_t&>()["key"]), libgs::value&
>);
static_assert(std::same_as<
	decltype(std::declval<const ini_keys_t&>()["key"]), libgs::optional<libgs::value>
>);

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

	const auto trailing = libgs::string_vector::from_string("a,", ',', false);
	LIBGS_TEST_CHECK_EQ(trailing.size(), 2U);
	LIBGS_TEST_CHECK(trailing.back().empty());

	const auto multi = libgs::string_vector::from_string(
		"left::middle::::right::", "::", false);
	LIBGS_TEST_CHECK_EQ(multi.size(), 5U);
	LIBGS_TEST_CHECK_EQ(multi[0], "left");
	LIBGS_TEST_CHECK_EQ(multi[1], "middle");
	LIBGS_TEST_CHECK(multi[2].empty());
	LIBGS_TEST_CHECK_EQ(multi[3], "right");
	LIBGS_TEST_CHECK(multi[4].empty());

	const auto whitespace = libgs::string_vector::from_string(
		"left, \t , right", ',', true);
	LIBGS_TEST_CHECK_EQ(whitespace.size(), 2U);
	LIBGS_TEST_CHECK_EQ(whitespace[1], " right");

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
	const libgs::url invalid_authority(
		"htt://4002[:db8::1]:8080-a/b?empty=&x=1");
	LIBGS_TEST_CHECK(not invalid_authority.is_valid());
	libgs::url invalid_setter("https://example.test/");
	invalid_setter.set_address("http://[2001:db8::1");
	LIBGS_TEST_CHECK(not invalid_setter.is_valid());
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

	libgs::ini moved(std::move(restored));
	LIBGS_TEST_CHECK_EQ(moved.read("server/port")->to_int().value_or(0), 8080);
	libgs::ini assigned;
	assigned = std::move(moved);
	LIBGS_TEST_CHECK(assigned.read("feature/enabled")->to_bool().value_or(false));
}

void ini_text_parameters()
{
	libgs::ini config;

	char group_buffer[] = "pointer group";
	char *group_pointer = group_buffer;
	const char key_array[] = "array key";
	config[group_pointer][key_array] = 11;

	std::string group_string = "string group";
	std::string_view key_view = "view key";
	config[group_string][key_view] = 12;

	ini_traits_string traits_group = "traits group";
	ini_traits_string traits_key = "traits key";
	config[traits_group][traits_key] = 13;

	config['g']['k'] = 14;

	ini_traits_view traits_group_view(traits_group.data(), traits_group.size());
	ini_traits_view traits_key_view(traits_key.data(), traits_key.size());
	LIBGS_TEST_CHECK(config.find(traits_group_view) != config.end());
	LIBGS_TEST_CHECK(config.group(traits_group_view).find(traits_key_view) !=
		config.group(traits_group_view).end());

	const auto &reader = config;
	LIBGS_TEST_CHECK_EQ(reader[std::string_view("pointer group")][key_array]->to_int().value_or(0), 11);
	LIBGS_TEST_CHECK_EQ(reader[group_string][key_view]->to_int().value_or(0), 12);
	LIBGS_TEST_CHECK_EQ(reader[traits_group_view][traits_key_view]->to_int().value_or(0), 13);
	LIBGS_TEST_CHECK_EQ(reader['g']['k']->to_int().value_or(0), 14);
}

void ini_asynchronous_file_io()
{
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "async-settings.ini";
	libgs::io_context_t context;

	libgs::ini config(context, file);
	config.write("server/host", "127.0.0.1");
	config.write("server/port", 8080);
	auto sync_result = config.sync(libgs::use_future);

	context.run();
	sync_result.get();
	LIBGS_TEST_CHECK(std::filesystem::is_regular_file(file));

	libgs::io_context_t completion_context;
	bool associated_completion = false;
	config.sync(asio::bind_executor(completion_context.get_executor(),
		[&](const libgs::error_code &error)
		{
			LIBGS_TEST_CHECK(not error);
			associated_completion = true;
		}
	));
	completion_context.run();
	LIBGS_TEST_CHECK(associated_completion);

	context.restart();
	libgs::ini restored(context, file);
	auto load_result = restored.load(libgs::use_future);
	context.run();
	load_result.get();
	LIBGS_TEST_CHECK_EQ(restored.read("server/host")->to_string(), "127.0.0.1");
	LIBGS_TEST_CHECK_EQ(restored.read("server/port")->to_int().value_or(0), 8080);

	context.restart();
	libgs::ini optional(context, directory.path() / "missing.ini");
	bool completed = false;
	const auto runner = std::this_thread::get_id();
	optional.load_or([&](const libgs::error_code &error)
	{
		LIBGS_TEST_CHECK(not error);
		LIBGS_TEST_CHECK(std::this_thread::get_id() == runner);
		completed = true;
	});
	context.run();
	LIBGS_TEST_CHECK(completed);

	libgs::error_code stale_error = make_error_code(std::errc::io_error);
	optional.load_or(stale_error);
	LIBGS_TEST_CHECK(not stale_error);

	std::promise<void> worker_entered;
	std::promise<void> release_worker;
	auto worker_ready = worker_entered.get_future();
	auto worker_release = release_worker.get_future();
	libgs::detail::ini_commit_io_work([&]
	{
		worker_entered.set_value();
		worker_release.wait();
	});
	worker_ready.wait();

	context.restart();
	libgs::error_code cancellation_error;
	restored.load([&](const libgs::error_code &error)
	{
		cancellation_error = error;
	});
	restored.cancel();
	release_worker.set_value();
	context.run();
	LIBGS_TEST_CHECK_EQ(cancellation_error,
		asio::error::make_error_code(asio::error::operation_aborted));

	const auto malformed_file = directory.path() / "malformed.ini";
	{
		std::ofstream stream(malformed_file);
		stream << "key-without-group=value\n";
	}
	context.restart();
	libgs::ini malformed(context, malformed_file);
	malformed.write("preserved/value", 42);
	auto malformed_result = malformed.load(libgs::use_future);
	context.run();
	LIBGS_TEST_CHECK_THROWS(malformed_result.get(), std::system_error);
	LIBGS_TEST_CHECK_EQ(malformed.read("preserved/value")->to_int().value_or(0), 42);
}

template <typename Queue>
auto checked_dequeue(Queue &queue)
{
	auto value = queue.dequeue();
	LIBGS_TEST_CHECK(value);
	return std::move(*value);
}

struct throwing_queue_value
{
	explicit throwing_queue_value(int value) : value(value)
	{
		if( value < 0 )
			throw std::runtime_error("queue value construction failed");
	}
	throwing_queue_value(throwing_queue_value &&other) : value(other.value)
	{
		if( std::exchange(throw_on_move, false) )
			throw std::runtime_error("queue value move failed");
	}

	static bool throw_on_move;
	int value;
};

bool throwing_queue_value::throw_on_move = false;

void check_linked_queue_exception_reuse()
{
	libgs::linked_lock_free_queue<throwing_queue_value> queue(128);
	for(int value = 0; value < 128; ++value)
	{
		LIBGS_TEST_CHECK(queue.emplace(value));
		LIBGS_TEST_CHECK_EQ(checked_dequeue(queue).value, value);
	}

	bool caught = false;
	try {
		queue.emplace(-1);
	}
	catch(const std::runtime_error&)
	{
		caught = true;
	}
	LIBGS_TEST_CHECK(caught);
	LIBGS_TEST_CHECK(queue.empty());
	LIBGS_TEST_CHECK_EQ(queue.size(), 0U);

	LIBGS_TEST_CHECK(queue.emplace(41));
	throwing_queue_value::throw_on_move = true;
	caught = false;
	try {
		queue.dequeue();
	}
	catch(const std::runtime_error&)
	{
		caught = true;
	}
	LIBGS_TEST_CHECK(caught);
	LIBGS_TEST_CHECK(queue.empty());
	LIBGS_TEST_CHECK_EQ(queue.size(), 0U);

	LIBGS_TEST_CHECK(queue.emplace(42));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue).value, 42);
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
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 1);
	LIBGS_TEST_CHECK_EQ(queue.force_emplace(4), 0U);
	LIBGS_TEST_CHECK_EQ(queue.force_emplace(5), 1U);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 3);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 4);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 5);
	LIBGS_TEST_CHECK(queue.empty());
}

template <libgs::queue_type Type>
void check_concurrent_queue_type()
{
	libgs::lock_free_queue<int,Type> queue(32);
	for(size_t round = 0; round < 20; ++round)
	{
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
		LIBGS_TEST_CHECK(queue.empty());
	}
}

template <libgs::queue_type Type>
void check_queue_boundaries_and_reuse()
{
	libgs::lock_free_queue<int,Type> queue(1);
	int output = -1;
	LIBGS_TEST_CHECK(not queue.dequeue(output));
	LIBGS_TEST_CHECK_EQ(output, -1);
	for(int value = 0; value < 10'000; ++value)
	{
		LIBGS_TEST_CHECK(queue.empty());
		LIBGS_TEST_CHECK_EQ(queue.size(), 0U);
		LIBGS_TEST_CHECK(not queue.dequeue());
		LIBGS_TEST_CHECK(queue.enqueue(value));
		LIBGS_TEST_CHECK(queue.full());
		LIBGS_TEST_CHECK_EQ(queue.size(), 1U);
		LIBGS_TEST_CHECK(not queue.enqueue(value + 1));
		LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), value);
	}

	LIBGS_TEST_CHECK_EQ(queue.force_emplace(1), 0U);
	for(int value = 2; value <= 100; ++value)
		LIBGS_TEST_CHECK_EQ(queue.force_emplace(value), 1U);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 100);

	queue.set_capacity(2);
	LIBGS_TEST_CHECK(queue.enqueue(1));
	queue.set_capacity(3);
	LIBGS_TEST_CHECK(queue.enqueue(2));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 1);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 2);

	LIBGS_TEST_CHECK(queue.enqueue(3));
	LIBGS_TEST_CHECK(queue.enqueue(4));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 3);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 4);
	queue.set_capacity(1);
	LIBGS_TEST_CHECK(queue.enqueue(5));
	LIBGS_TEST_CHECK(not queue.enqueue(6));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 5);

	queue.set_capacity(0);
	LIBGS_TEST_CHECK(queue.capacity() > 0);
	LIBGS_TEST_CHECK(queue.enqueue(7));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(queue), 7);

	libgs::lock_free_queue<int,Type> source(4);
	LIBGS_TEST_CHECK(source.enqueue(11));
	LIBGS_TEST_CHECK(source.enqueue(12));
	libgs::lock_free_queue<int,Type> moved(std::move(source));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(moved), 11);
	LIBGS_TEST_CHECK_EQ(checked_dequeue(moved), 12);
	LIBGS_TEST_CHECK(source.enqueue(13));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(source), 13);

	auto *same_queue = &moved;
	moved = std::move(*same_queue);
	LIBGS_TEST_CHECK(moved.enqueue(14));
	LIBGS_TEST_CHECK_EQ(checked_dequeue(moved), 14);

	libgs::lock_free_queue<std::unique_ptr<int>,Type> move_only(2);
	LIBGS_TEST_CHECK(move_only.enqueue(std::make_unique<int>(42)));
	auto move_only_value = checked_dequeue(move_only);
	LIBGS_TEST_CHECK(move_only_value);
	LIBGS_TEST_CHECK_EQ(*move_only_value, 42);

	std::weak_ptr<int> lifetime;
	{
		auto owner = std::make_shared<int>(7);
		lifetime = owner;
		libgs::lock_free_queue<std::shared_ptr<int>,Type> retained(2);
		LIBGS_TEST_CHECK(retained.enqueue(std::move(owner)));
	}
	LIBGS_TEST_CHECK(lifetime.expired());
}

template <libgs::queue_type Type>
void check_mpmc_queue_type()
{
	constexpr size_t producer_count = 2;
	constexpr size_t items_per_producer = 2'000;
	constexpr size_t item_count = producer_count * items_per_producer;
	libgs::lock_free_queue<size_t,Type> queue(64);
	std::array<std::atomic_uint,item_count> seen {};
	std::atomic_size_t consumed = 0;
	std::atomic_size_t producers_left = producer_count;
	std::atomic_bool invalid_value = false;

	std::array<std::thread,producer_count> producers;
	for(size_t producer = 0; producer < producer_count; ++producer)
	{
		producers[producer] = std::thread([&, producer]
		{
			const auto begin = producer * items_per_producer;
			for(size_t index = begin; index < begin + items_per_producer; ++index)
			{
				while(not queue.enqueue(index))
					std::this_thread::yield();
			}
			producers_left.fetch_sub(1, std::memory_order_release);
		});
	}

	std::array<std::thread,2> consumers;
	for(auto &consumer : consumers)
	{
		consumer = std::thread([&]
		{
			while(producers_left.load(std::memory_order_acquire) != 0 or
				not queue.empty())
			{
				if(auto value = queue.dequeue())
				{
					if(*value < item_count)
						seen[*value].fetch_add(1, std::memory_order_relaxed);
					else
						invalid_value.store(true, std::memory_order_relaxed);
					consumed.fetch_add(1, std::memory_order_relaxed);
				}
				else
					std::this_thread::yield();
			}
		});
	}

	for(auto &producer : producers)
		producer.join();
	for(auto &consumer : consumers)
		consumer.join();
	LIBGS_TEST_CHECK(not invalid_value.load());
	LIBGS_TEST_CHECK_EQ(consumed.load(), item_count);
	for(const auto &count : seen)
		LIBGS_TEST_CHECK_EQ(count.load(), 1U);
	LIBGS_TEST_CHECK(queue.empty());
}

void lock_free_queues()
{
	check_queue_type<libgs::queue_type::linked>();
	check_queue_type<libgs::queue_type::circular>();
	check_queue_boundaries_and_reuse<libgs::queue_type::linked>();
	check_queue_boundaries_and_reuse<libgs::queue_type::circular>();
	check_concurrent_queue_type<libgs::queue_type::linked>();
	check_concurrent_queue_type<libgs::queue_type::circular>();
	check_mpmc_queue_type<libgs::queue_type::linked>();
	check_mpmc_queue_type<libgs::queue_type::circular>();
	check_linked_queue_exception_reuse();
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
		{"INI text parameters", ini_text_parameters},
		{"INI asynchronous file I/O", ini_asynchronous_file_io},
		{"lock-free queues", lock_free_queues},
		{"MIME detection", mime_detection},
		{"application environment", application_environment},
	});
}
