// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/http/protocol/cookie.h>
#include <libgs/http/protocol/types.h>
#include <libgs/http/protocol/version.h>
#include <libgs/http/client.h>

#include <cstdint>
#include <string>

namespace
{

void protocol_enums()
{
	using namespace libgs::http;

	LIBGS_TEST_CHECK_EQ(method::from_string("GET"), method::get);
	LIBGS_TEST_CHECK_EQ(std::string(method::string(method::delet)), "DELETE");
	LIBGS_TEST_CHECK_EQ(
		std::string(status::description(status::not_found)), "Not Found"
	);
	LIBGS_TEST_CHECK_EQ(version::from_string("1.1"), version::v11);
	LIBGS_TEST_CHECK_EQ(std::string(version::string(version::v10)), "1.0");
	LIBGS_TEST_CHECK_EQ(version(version::v11).number(), 1.1);

	constexpr methods safe_methods {method::get, method::head};
	static_assert(safe_methods.test_flag(method::get));
	static_assert(safe_methods.test_flag(method::head));
	static_assert(not safe_methods.test_flag(method::post));
}

void cookie_values()
{
	libgs::http::cookie item("session-token");
	item.set_domain("example.test")
		.set_path("/account")
		.set_same_site("Strict")
		.set_priority("High")
		.set_expires(3600)
		.set_max_age(1800)
		.set_size(13)
		.set_http_only(true)
		.set_secure(true);

	LIBGS_TEST_CHECK_EQ(item.value<std::string>(), "session-token");
	LIBGS_TEST_CHECK_EQ(*item.domain(), "example.test");
	LIBGS_TEST_CHECK_EQ(*item.path(), "/account");
	LIBGS_TEST_CHECK_EQ(*item.same_site(), "Strict");
	LIBGS_TEST_CHECK_EQ(*item.priority(), "High");
	LIBGS_TEST_CHECK_EQ(*item.expires(), std::uint64_t {3600});
	LIBGS_TEST_CHECK_EQ(*item.max_age(), std::uint64_t {1800});
	LIBGS_TEST_CHECK_EQ(*item.size(), size_t {13});
	LIBGS_TEST_CHECK(*item.http_only());
	LIBGS_TEST_CHECK(*item.secure());

	item.unset_domain().unset_http_only().unset_secure();
	LIBGS_TEST_CHECK(not item.domain());
	LIBGS_TEST_CHECK(not item.http_only());
	LIBGS_TEST_CHECK(not item.secure());

	libgs::http::cookie copied(item);
	copied.set_path("/other");
	LIBGS_TEST_CHECK_EQ(*item.path(), "/account");
	LIBGS_TEST_CHECK_EQ(*copied.path(), "/other");
}

void client_url_validation()
{
	libgs::io_context_t context;
	libgs::http::client client(context.get_executor());
	std::error_code error;

	auto fragmented = client.request_get(
		libgs::url("http://example.test/path#fragment"), error
	);
	LIBGS_TEST_CHECK(not fragmented);
	LIBGS_TEST_CHECK(error == std::errc::invalid_argument);

	error.clear();
	auto wrong_scheme = client.request_get(
		libgs::url("ws://example.test/path"), error
	);
	LIBGS_TEST_CHECK(not wrong_scheme);
	LIBGS_TEST_CHECK(error == std::errc::protocol_not_supported);

	error.clear();
	auto missing_host = client.request_get(
		libgs::url("http:///path"), error
	);
	LIBGS_TEST_CHECK(not missing_host);
	LIBGS_TEST_CHECK(error == std::errc::invalid_argument);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"protocol enums", protocol_enums},
		{"cookie values", cookie_values},
		{"client URL validation", client_url_validation},
	});
}
