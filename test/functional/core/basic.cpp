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
#include <libgs/core/utils/byte_order.h>
#include <libgs/core/utils/streamer.h>
#include <libgs/core/utils/string_tools.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

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

	optional.emplace(3, 'x');
	LIBGS_TEST_CHECK(optional.has_value());
	LIBGS_TEST_CHECK_EQ(*optional, "xxx");
	const auto length = optional.transform([](const auto &value) {
		return value.size();
	});
	LIBGS_TEST_CHECK_EQ(*length, size_t {3});

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
	error.emplace(7);
	LIBGS_TEST_CHECK_EQ(*error, 7);
}

} //namespace

int main()
{
	return libgs::test::run({
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
	});
}
