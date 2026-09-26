// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/core/algorithm/math.h>
#include <libgs/core/algorithm/misc.h>
#include <libgs/core/algorithm/sha1.h>
#include <libgs/core/algorithm/uuid.h>
#include <libgs/core/async_expected.h>
#include <libgs/core/cxx/expected.h>
#include <libgs/core/cxx/optional.h>
#include <libgs/core/cxx/tools.h>
#include <libgs/core/shared_mutex.h>
#include <libgs/core/atomic_mutex.h>
#include <libgs/core/utils/byte_order.h>
#include <libgs/core/utils/streamer.h>
#include <libgs/core/utils/string_tools.h>
#include <memory>

namespace
{

template <typename Buffer, typename Source>
concept buffer_data_copyable = requires(Source &&source) {
	libgs::copy_buffer_data<Buffer>(std::forward<Source>(source));
};

static_assert(libgs::is_array_buffer_v<std::array<std::uint32_t,4>>);
static_assert(libgs::is_vector_buffer_v<std::vector<std::uint32_t>>);
static_assert(libgs::is_string_buffer_v<std::string>);

static_assert(not libgs::is_array_buffer_v<std::array<std::string,4>>);
static_assert(not libgs::is_array_buffer_v<std::array<const std::uint32_t,4>>);
static_assert(not libgs::is_vector_buffer_v<std::vector<std::string>>);
static_assert(not libgs::is_vector_buffer_v<std::vector<bool>>);
static_assert(not libgs::is_buffer_v<std::vector<std::string>>);

static_assert(std::derived_from<libgs::optional<int>,std::optional<int>>);
static_assert(std::same_as<libgs::nullopt_t,std::nullopt_t>);
static_assert(libgs::is_optional_v<libgs::optional<int>>);
static_assert(libgs::is_optional_v<std::optional<int>>);
static_assert(std::same_as <
	decltype(libgs::make_optional("value")),libgs::optional<const char*>
>);
static_assert(std::same_as <
	decltype(std::declval<libgs::optional<std::string>&>().emplace(3, 'x')),
	std::string&
>);
static_assert(std::same_as <
	decltype(std::declval<libgs::optional<int>&>().reset()),void
>);
static_assert(std::same_as <
	decltype(std::declval<libgs::expected<int,std::string>&>().emplace(1)),int&
>);
static_assert(std::same_as <
	decltype(std::declval<libgs::expected<void,std::string>&>().emplace()),void
>);
#if LIBGS_HAS_STD_EXPECTED
static_assert(std::derived_from <
	libgs::expected<int,std::string>,std::expected<int,std::string>
>);
static_assert(std::derived_from <
	libgs::unexpected<std::string>,std::unexpected<std::string>
>);
static_assert(libgs::is_expected_v<std::expected<int,std::string>>);
#endif

static_assert(buffer_data_copyable<
	std::vector<std::uint32_t>, std::vector<std::byte>
>);
static_assert(not buffer_data_copyable<
	std::vector<std::string>, std::vector<std::byte>
>);
static_assert(not buffer_data_copyable<
	std::vector<std::byte>, std::vector<std::string>
>);

struct polymorphic_base
{
	virtual ~polymorphic_base() = default;
};

struct polymorphic_derived final : polymorphic_base {};

struct meta_fields_sample
{
	LIBGS_META_FIELDS (
		( int, plain ),
		( std::string, initialized, "value" ),
		( (std::pair<int,int>), pair )
	);
};

struct large_meta_fields_sample
{
	LIBGS_META_FIELDS (
		( int, field_00 ),
		( int, field_01 ),
		( int, field_02 ),
		( int, field_03 ),
		( int, field_04 ),
		( int, field_05 ),
		( int, field_06 ),
		( int, field_07 ),
		( int, field_08 ),
		( int, field_09 ),
		( int, field_10 ),
		( int, field_11 ),
		( int, field_12 ),
		( int, field_13 ),
		( int, field_14 ),
		( int, field_15 ),
		( int, field_16 ),
		( int, field_17 ),
		( int, field_18 ),
		( int, field_19 ),
		( int, field_20 ),
		( int, field_21 ),
		( int, field_22 ),
		( int, field_23 ),
		( int, field_24 ),
		( int, field_25 ),
		( int, field_26 ),
		( int, field_27 ),
		( int, field_28 ),
		( int, field_29 ),
		( int, field_30 ),
		( int, field_31 ),
		( int, field_32 ),
		( int, field_33 ),
		( int, field_34 ),
		( int, field_35 ),
		( int, field_36 ),
		( int, field_37 ),
		( int, field_38 ),
		( int, field_39 ),
		( int, field_40 ),
		( int, field_41 ),
		( int, field_42 ),
		( int, field_43 ),
		( int, field_44 ),
		( int, field_45 ),
		( int, field_46 ),
		( int, field_47 ),
		( int, field_48 ),
		( int, field_49 ),
		( int, field_50 ),
		( int, field_51 ),
		( int, field_52 ),
		( int, field_53 ),
		( int, field_54 ),
		( int, field_55 ),
		( int, field_56 ),
		( int, field_57 ),
		( int, field_58 ),
		( int, field_59 ),
		( int, field_60 ),
		( int, field_61 ),
		( int, field_62 ),
		( int, field_63 ),
		( int, field_64 ),
		( int, field_65 ),
		( int, field_66 ),
		( int, field_67 ),
		( int, field_68 ),
		( int, field_69 ),
		( int, field_70 ),
		( int, field_71 ),
		( int, field_72 ),
		( int, field_73 ),
		( int, field_74 ),
		( int, field_75 ),
		( int, field_76 ),
		( int, field_77 ),
		( int, field_78 ),
		( int, field_79 )
	);
};

void meta_fields_macros()
{
	meta_fields_sample sample;
	LIBGS_TEST_CHECK_EQ(sample.plain, 0);
	LIBGS_TEST_CHECK_EQ(sample.initialized, "value");
	LIBGS_TEST_CHECK_EQ(sample.pair, (std::pair<int,int> {}));

	auto fields = sample.meta_fields();
	static_assert(std::tuple_size_v<decltype(fields)> == 3);
	std::get<0>(fields) = 42;
	LIBGS_TEST_CHECK_EQ(sample.plain, 42);

	large_meta_fields_sample large;
	auto large_fields = large.meta_fields();
	static_assert(std::tuple_size_v<decltype(large_fields)> == 80);
	std::get<79>(large_fields) = 79;
	LIBGS_TEST_CHECK_EQ(large.field_79, 79);
}

void type_names()
{
	const char *first = libgs::type_name<int>();
	LIBGS_TEST_CHECK(first != nullptr);
	LIBGS_TEST_CHECK(first == libgs::type_name<int>());
	LIBGS_TEST_CHECK_EQ(std::string_view(first), "int");
	LIBGS_TEST_CHECK_EQ(
		std::string_view(libgs::type_name(typeid(int))),
		std::string_view(first)
	);

	polymorphic_derived derived;
	polymorphic_base &base = derived;
	const char *dynamic = libgs::type_name(base);
	LIBGS_TEST_CHECK(dynamic != nullptr);
	LIBGS_TEST_CHECK(dynamic == libgs::type_name(base));
	LIBGS_TEST_CHECK_EQ(
		std::string_view(dynamic),
		std::string_view(libgs::type_name<polymorphic_derived>())
	);
}

void percent_encoding()
{
	LIBGS_TEST_CHECK_EQ(
		libgs::to_percent_encoding("a b/c?value=1"),
		"a%20b%2Fc%3Fvalue%3D1"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::to_percent_encoding("a b/c", "/"),
		"a%20b/c"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::to_percent_encoding(
			"a-b", std::string_view(), std::string_view("-")
		),
		"a%2Db"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::from_percent_encoding(std::string("a%20b%2fc")),
		"a b/c"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::from_percent_encoding(std::wstring(L"%E4%B8%AD")),
		std::wstring(L"\x00E4\x00B8\x00AD")
	);
}

void wildcard_matching()
{
	LIBGS_TEST_CHECK_EQ(libgs::wildcard_match("README.md", "README.md"), 0);
	LIBGS_TEST_CHECK(libgs::wildcard_match("*.md", "README.md") > 0);
	LIBGS_TEST_CHECK(libgs::wildcard_match("libgs/??re.h", "libgs/core.h") > 0);
	LIBGS_TEST_CHECK_EQ(libgs::wildcard_match("*.cpp", "README.md"), -1);
	LIBGS_TEST_CHECK_EQ(libgs::wildcard_match("", ""), 0);
}

void sha1_vectors()
{
	libgs::sha1 empty;
	LIBGS_TEST_CHECK_EQ(
		empty.finalize().hex(false),
		"da39a3ee5e6b4b0d3255bfef95601890afd80709"
	);

	libgs::sha1 abc;
	abc.append('a').append("bc").finalize();
	LIBGS_TEST_CHECK_EQ(
		abc.hex(),
		"A9993E364706816ABA3E25717850C26C9CD0D89D"
	);
	LIBGS_TEST_CHECK_EQ(abc.base64(), "qZk+NkcGgWq6PiVxeFDCbJzQ2J0=");

	libgs::sha1 original(std::string(80, 'x'));
	libgs::sha1 copied(original);
	libgs::sha1 assigned;
	assigned = original;
	original.append("original suffix").finalize();
	copied.append("original suffix").finalize();
	assigned.append("original suffix").finalize();
	LIBGS_TEST_CHECK_EQ(copied.hex(), original.hex());
	LIBGS_TEST_CHECK_EQ(assigned.hex(), original.hex());
}

void uuid_values()
{
	const libgs::uuid dns_namespace("6ba7b810-9dad-11d1-80b4-00c04fd430c8");
	LIBGS_TEST_CHECK(dns_namespace.is_valid());
	LIBGS_TEST_CHECK_EQ(
		dns_namespace.to_string(),
		"6BA7B810-9DAD-11D1-80B4-00C04FD430C8"
	);
	LIBGS_TEST_CHECK_EQ(
		dns_namespace.to_string(true),
		"{6BA7B810-9DAD-11D1-80B4-00C04FD430C8}"
	);

	const auto named = libgs::uuid::generate_v5(dns_namespace, "www.widgets.com");
	LIBGS_TEST_CHECK(named.is_valid());
	LIBGS_TEST_CHECK_EQ(named.version(), libgs::uuid_version::v5);
	LIBGS_TEST_CHECK_EQ(
		named.to_string(),
		"21F7F8DE-8051-5B89-8680-0195EF798B6A"
	);

	const libgs::uuid invalid("not-a-uuid");
	LIBGS_TEST_CHECK(not invalid.is_valid());
	LIBGS_TEST_CHECK_EQ(invalid.version(), libgs::uuid_version::none);
	LIBGS_TEST_CHECK(invalid.to_string().empty());

	const libgs::uuid nil("00000000-0000-0000-0000-000000000000");
	LIBGS_TEST_CHECK(nil.is_valid());
	LIBGS_TEST_CHECK(nil.is_nil());
}

void arithmetic_and_byte_order()
{
	const std::array values {2, 4, 6, 8};
	LIBGS_TEST_CHECK_EQ(libgs::mean(values.begin(), values.end()), 5);

	constexpr std::uint32_t value = 0x01020304U;
	LIBGS_TEST_CHECK_EQ(libgs::reverse(value), 0x04030201U);
	LIBGS_TEST_CHECK_EQ(libgs::reverse(libgs::reverse(value)), value);
	LIBGS_TEST_CHECK(libgs::is_little_endian() != libgs::is_big_endian());
	LIBGS_TEST_CHECK_EQ(libgs::ntoh(libgs::hton(value)), value);
}

void string_tools()
{
	LIBGS_TEST_CHECK_EQ(libgs::strtls::trimmed(" \t LibGS \r\n"), "LibGS");
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_lower("LiBgS"), "libgs");
	LIBGS_TEST_CHECK_EQ(libgs::strtls::to_upper(std::string("LiBgS")), "LIBGS");
	LIBGS_TEST_CHECK(libgs::strtls::is_alpha("LibGS"));
	LIBGS_TEST_CHECK(not libgs::strtls::is_alpha("LibGS1"));
	LIBGS_TEST_CHECK(libgs::strtls::is_digit("012345"));
	LIBGS_TEST_CHECK_EQ(*libgs::strtls::to_int32("-42"), -42);
	LIBGS_TEST_CHECK_EQ(*libgs::strtls::to_uint32("2a", 16), 42U);
	LIBGS_TEST_CHECK(not libgs::strtls::to_int32("12x"));
	LIBGS_TEST_CHECK_EQ(libgs::strtls::file_name("/tmp/libgs/test.cpp"), "test.cpp");
	LIBGS_TEST_CHECK_EQ(libgs::strtls::file_path("/tmp/libgs/test.cpp"), "/tmp/libgs/");
}

void buffer_copying()
{
	const std::vector<std::byte> source {
		std::byte {0x01}, std::byte {0x02}, std::byte {0x03},
		std::byte {0x04}, std::byte {0x05}
	};
	const auto words = libgs::copy_buffer_data<std::vector<std::uint16_t>>(source);
	LIBGS_TEST_CHECK_EQ(words.size(), 3U);
	LIBGS_TEST_CHECK(std::memcmp(words.data(), source.data(), source.size()) == 0);

	const auto text = libgs::copy_buffer_data<std::string>(source);
	LIBGS_TEST_CHECK_EQ(text.size(), source.size());
	LIBGS_TEST_CHECK(std::memcmp(text.data(), source.data(), source.size()) == 0);
}

void optional_and_expected()
{
	libgs::optional<std::string> optional;
	LIBGS_TEST_CHECK(not optional);
	LIBGS_TEST_CHECK_EQ(optional.value_or("fallback"), "fallback");
	LIBGS_TEST_CHECK(optional == libgs::nullopt);
	LIBGS_TEST_CHECK(optional == libgs::optional<std::string> {});
	LIBGS_TEST_CHECK_THROWS(optional.value(), libgs::bad_optional_access);

	optional.emplace(3, 'x');
	LIBGS_TEST_CHECK(optional.has_value());
	LIBGS_TEST_CHECK_EQ(*optional, "xxx");
	const auto length = optional.transform([](const auto &value) {
		return value.size();
	});
	LIBGS_TEST_CHECK_EQ(*length, size_t {3});

	std::optional<std::string> standard_optional("standard");
	optional = standard_optional;
	LIBGS_TEST_CHECK_EQ(optional.value(), "standard");
	std::optional<std::string> &optional_base = optional;
	LIBGS_TEST_CHECK_EQ(optional_base.value(), "standard");
	LIBGS_TEST_CHECK_EQ(libgs::optional<int> {}.and_then([](int value) {
		return std::optional<long>(value);
	}), std::nullopt);
	LIBGS_TEST_CHECK_EQ(libgs::optional<int> {}.or_else([] {
		return std::optional<int>(9);
	}).value(), 9);
	bool optional_recovered = false;
	static_cast<void>(libgs::optional<int> {}.or_else([&optional_recovered] {
		optional_recovered = true;
	}));
	LIBGS_TEST_CHECK(optional_recovered);

	libgs::optional<std::unique_ptr<int>> move_source(
		std::in_place, std::make_unique<int>(1)
	);
	auto move_target = std::move(move_source);
	LIBGS_TEST_CHECK(move_source.has_value());
	LIBGS_TEST_CHECK(move_target.has_value());

	libgs::expected<int,std::string> success(42);
	LIBGS_TEST_CHECK(success.has_value());
	LIBGS_TEST_CHECK(not success.is_error());
	LIBGS_TEST_CHECK_EQ(*success, 42);

	libgs::expected<int,std::string> error(
		libgs::unexpected<std::string>("failure")
	);
	LIBGS_TEST_CHECK(not error.has_value());
	LIBGS_TEST_CHECK(error.is_error());
	LIBGS_TEST_CHECK_EQ(error.error(), "failure");
	LIBGS_TEST_CHECK_EQ(error.error_or("fallback"), "failure");
	LIBGS_TEST_CHECK_EQ(error.transform_error([](const std::string &message) {
		return message.size();
	}).error(), size_t {7});
	LIBGS_TEST_CHECK_EQ(error.and_then([](int value) {
		return libgs::expected<long,std::string>(value);
	}).error(), "failure");
	LIBGS_TEST_CHECK_THROWS(error.value(), libgs::bad_expected_access<std::string>);
	error.emplace(7);
	LIBGS_TEST_CHECK_EQ(*error, 7);

	libgs::expected<void,std::string> no_value(libgs::unexpect, "void-error");
	LIBGS_TEST_CHECK_EQ(no_value.transform([] { return 11; }).error(), "void-error");
	no_value.emplace();
	LIBGS_TEST_CHECK(no_value.has_value());
	libgs::sys_expected<bool> direct_error {
		std::make_error_code(std::errc::invalid_argument)
	};
	LIBGS_TEST_CHECK(not direct_error);
	LIBGS_TEST_CHECK(direct_error.error() == std::errc::invalid_argument);

#if LIBGS_HAS_STD_EXPECTED
	std::expected<int,std::string> &expected_base = success;
	LIBGS_TEST_CHECK_EQ(expected_base.value(), 42);
	libgs::expected<int,std::string> from_standard(
		std::expected<int,std::string>(17)
	);
	LIBGS_TEST_CHECK((from_standard == std::expected<int,std::string>(17)));
	auto standard_chain = libgs::expected<int,std::string>(
		libgs::unexpect, "standard-error"
	).and_then([](int value) {
		return std::expected<long,std::string>(value);
	});
	static_assert(std::same_as <
		decltype(standard_chain),std::expected<long,std::string>
	>);
	LIBGS_TEST_CHECK_EQ(standard_chain.error(), "standard-error");
#endif
}

template <libgs::atomic_mutex_policy Policy>
void atomic_locks_with_policy()
{
	libgs::basic_atomic_mutex<Policy> mutex;
	LIBGS_TEST_CHECK(mutex.try_lock());
	LIBGS_TEST_CHECK(not mutex.try_lock());
	mutex.unlock();

	size_t count = 0;
	std::vector<std::thread> workers;
	for(size_t worker = 0; worker < 8; ++worker)
	{
		workers.emplace_back([&]
		{
			for(size_t index = 0; index < 5'000; ++index)
			{
				std::lock_guard lock(mutex);
				++count;
			}
		});
	}
	for(auto &worker : workers)
		worker.join();
	LIBGS_TEST_CHECK_EQ(count, 40'000U);

	libgs::basic_atomic_shared_mutex<Policy> shared_mutex;
	size_t shared_value = 0;
	std::atomic_size_t writers_done {0};
	std::atomic_size_t try_writes {0};
	std::atomic_bool try_writer_done {false};
	std::atomic_bool invalid_read {false};
	workers.clear();
	for(size_t writer = 0; writer < 2; ++writer)
	{
		workers.emplace_back([&]
		{
			for(size_t index = 0; index < 5'000; ++index)
			{
				std::unique_lock lock(shared_mutex);
				++shared_value;
			}
			writers_done.fetch_add(1, std::memory_order_release);
		});
	}
	workers.emplace_back([&]
	{
		for(size_t index = 0; index < 50'000; ++index)
		{
			if( shared_mutex.try_lock() )
			{
				++shared_value;
				try_writes.fetch_add(1, std::memory_order_relaxed);
				shared_mutex.unlock();
			}
			else
				std::this_thread::yield();
		}
		try_writer_done.store(true, std::memory_order_release);
	});
	for(size_t reader = 0; reader < 4; ++reader)
	{
		workers.emplace_back([&]
		{
			size_t observed = 0;
			while( writers_done.load(std::memory_order_acquire) != 2 or
				not try_writer_done.load(std::memory_order_acquire) )
			{
				std::shared_lock lock(shared_mutex);
				if( shared_value < observed )
					invalid_read.store(true, std::memory_order_relaxed);
				observed = shared_value;
			}
		});
	}
	for(auto &worker : workers)
		worker.join();
	LIBGS_TEST_CHECK(not invalid_read.load(std::memory_order_relaxed));
	LIBGS_TEST_CHECK_EQ(shared_value,
		10'000U + try_writes.load(std::memory_order_relaxed));
	LIBGS_TEST_CHECK(shared_mutex.try_lock_shared());
	LIBGS_TEST_CHECK(not shared_mutex.try_lock());
	shared_mutex.unlock_shared();
	LIBGS_TEST_CHECK(shared_mutex.try_lock());
	LIBGS_TEST_CHECK(not shared_mutex.try_lock_shared());
	shared_mutex.unlock();

	shared_mutex.lock_shared();
	std::atomic_bool writer_started {false};
	std::atomic_bool writer_entered {false};
	std::thread blocked_writer([&]
	{
		writer_started.store(true, std::memory_order_release);
		writer_started.notify_one();
		std::unique_lock lock(shared_mutex);
		writer_entered.store(true, std::memory_order_release);
	});

	writer_started.wait(false, std::memory_order_acquire);
	bool writer_pending = false;
	const auto pending_deadline =
		std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while( std::chrono::steady_clock::now() < pending_deadline )
	{
		if( not shared_mutex.try_lock_shared() )
		{
			writer_pending = true;
			break;
		}
		shared_mutex.unlock_shared();
		std::this_thread::yield();
	}

	std::atomic_bool late_reader_entered {false};
	std::atomic_bool late_reader_saw_writer {false};
	std::thread late_reader;
	if( writer_pending )
	{
		late_reader = std::thread([&]
		{
			std::shared_lock lock(shared_mutex);
			late_reader_saw_writer.store(
				writer_entered.load(std::memory_order_acquire),
				std::memory_order_release
			);
			late_reader_entered.store(true, std::memory_order_release);
		});
	}

	shared_mutex.unlock_shared();
	blocked_writer.join();
	if( late_reader.joinable() )
		late_reader.join();

	LIBGS_TEST_CHECK(writer_pending);
	LIBGS_TEST_CHECK(writer_entered.load(std::memory_order_acquire));
	LIBGS_TEST_CHECK(late_reader_entered.load(std::memory_order_acquire));
	LIBGS_TEST_CHECK(late_reader_saw_writer.load(std::memory_order_acquire));
}

void atomic_locks()
{
	atomic_locks_with_policy<libgs::atomic_mutex_policy::balanced>();
	atomic_locks_with_policy<libgs::atomic_mutex_policy::low_latency>();
}

} //namespace

int main(int argc, const char *const argv[])
{
	return libgs::test::run(argc, argv, {
		{"meta fields macros", meta_fields_macros},
		{"type names", type_names},
		{"percent encoding", percent_encoding},
		{"wildcard matching", wildcard_matching},
		{"SHA-1 vectors", sha1_vectors},
		{"UUID values", uuid_values},
		{"arithmetic and byte order", arithmetic_and_byte_order},
		{"string tools", string_tools},
		{"buffer copying", buffer_copying},
		{"optional and expected", optional_and_expected},
		{"atomic locks", atomic_locks},
	});
}
