#include "test.h"

#include <libgs/http/protocol/cookie.h>
#include <libgs/http/protocol/types.h>
#include <libgs/http/protocol/utils/client/url.h>
#include <libgs/http/protocol/version.h>

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

void url_parsing()
{
	libgs::http::url target("HTTPS://example.test:8443/a%20b/items?q=hello%20world&flag");
	LIBGS_TEST_CHECK(target.is_valid());
	LIBGS_TEST_CHECK_EQ(target.protocol(), "https");
	LIBGS_TEST_CHECK_EQ(target.host(), "example.test");
	LIBGS_TEST_CHECK_EQ(target.port(), 8443);
	LIBGS_TEST_CHECK_EQ(target.path(), "/a b/items");
	LIBGS_TEST_CHECK(target.contains_parameter("q", "hello world"));
	LIBGS_TEST_CHECK(target.contains_parameter("flag", "flag"));
	LIBGS_TEST_CHECK_EQ(
		target.to_string(),
		"https://example.test:8443/a%20b/items?q=hello%20world&flag=flag"
	);

	const libgs::http::url ipv6("http://[::1]/health");
	LIBGS_TEST_CHECK(ipv6.is_valid());
	LIBGS_TEST_CHECK_EQ(ipv6.host(), "::1");
	LIBGS_TEST_CHECK_EQ(ipv6.port(), 80);
	LIBGS_TEST_CHECK_EQ(ipv6.to_string(), "http://[::1]:80/health");

	const libgs::http::url invalid("example.test/no-scheme");
	LIBGS_TEST_CHECK(not invalid.is_valid());
	LIBGS_TEST_CHECK(invalid.to_string().empty());
}

void url_resolution()
{
	const libgs::http::url base("https://example.test:443/a/b/index.html?old=1");

	LIBGS_TEST_CHECK_EQ(
		libgs::http::url::resolve(base, "../image.png").to_string(),
		"https://example.test:443/a/image.png"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::http::url::resolve(base, "/status?q=ok").to_string(),
		"https://example.test:443/status?q=ok"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::http::url::resolve(base, "?fresh=1").to_string(),
		"https://example.test:443/a/b/index.html?fresh=1"
	);
	LIBGS_TEST_CHECK_EQ(
		libgs::http::url::resolve(base, "http://other.test/x").to_string(),
		"http://other.test:80/x"
	);
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

} //namespace

int main()
{
	return libgs::test::run({
		{"protocol enums", protocol_enums},
		{"URL parsing", url_parsing},
		{"URL resolution", url_resolution},
		{"cookie values", cookie_values},
	});
}
