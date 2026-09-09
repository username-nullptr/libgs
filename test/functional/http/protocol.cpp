// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/http/protocol/utils/client/cookie_jar.h>
#include <libgs/http/protocol/utils/client/form_data.h>
#include <libgs/http/protocol/utils/client/generator.h>
#include <libgs/http/protocol/utils/client/parser.h>
#include <libgs/http/protocol/utils/core/conditional.h>
#include <libgs/http/protocol/utils/core/range.h>
#include <libgs/http/protocol/utils/core/upgrade.h>
#include <libgs/http/protocol/utils/server/generator.h>
#include <libgs/http/protocol/utils/server/parser.h>

#include <chrono>
#include <string>

namespace
{

libgs::const_buffer buffer(std::string_view value)
{
	return {value.data(), value.size()};
}

void parser_errors()
{
	using namespace libgs::http;
	const libgs::error_code empty = parse_errc::IDE;
	LIBGS_TEST_CHECK(empty == parse_errc::IDE);
	LIBGS_TEST_CHECK(parse_errc::IDE == empty);
	LIBGS_TEST_CHECK(empty != parse_errc::RE);
	LIBGS_TEST_CHECK(parse_errc::RE != empty);
	LIBGS_TEST_CHECK_EQ(empty, make_error_code(parse_errc::IDE));
	LIBGS_TEST_CHECK_EQ(empty.category(), parse_error_category());
	LIBGS_TEST_CHECK_EQ(empty.message(), "The inserted data is empty.");

	server_parser parser;
	auto result = parser.append({});
	LIBGS_TEST_CHECK(not result);
	LIBGS_TEST_CHECK(result.error() == parse_errc::IDE);

	server_parser malformed_header;
	result = malformed_header.append(buffer(
		"GET / HTTP/1.1\r\nMissing-Colon\r\n\r\n"
	));
	LIBGS_TEST_CHECK(not result);
	LIBGS_TEST_CHECK(result.error() == parse_errc::IHL);

	server_parser invalid_method;
	result = invalid_method.append(buffer("FETCH / HTTP/1.1\r\n\r\n"));
	LIBGS_TEST_CHECK(not result);
	LIBGS_TEST_CHECK(result.error() == parse_errc::IHM);

	server_parser conflicting_size;
	result = conflicting_size.append(buffer(
		"POST / HTTP/1.1\r\n"
		"Host: example.test\r\n"
		"Content-Length: 1\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
	));
	LIBGS_TEST_CHECK(not result);
	LIBGS_TEST_CHECK(result.error() == parse_errc::SFE);
}

void enum_input_validation()
{
	using namespace libgs::http;
	const auto invalid_status = static_cast<status_enum>(999);
	const auto invalid_method = static_cast<method_enum>(0x8000);
	const auto invalid_version = static_cast<version_enum>(0x0909);

	LIBGS_TEST_CHECK(not status::check(invalid_status, false));
	LIBGS_TEST_CHECK_EQ(std::string(status::description(invalid_status, false)), "");
	LIBGS_TEST_CHECK_THROWS(status::check(invalid_status), libgs::runtime_error);

	LIBGS_TEST_CHECK(not method::check(invalid_method, false));
	LIBGS_TEST_CHECK_EQ(std::string(method::string(invalid_method, false)), "");
	LIBGS_TEST_CHECK_THROWS(method::from_string("FETCH"), libgs::runtime_error);

	LIBGS_TEST_CHECK(not version::check(invalid_version, false));
	LIBGS_TEST_CHECK_EQ(version::number(invalid_version, false), 0.0);
	LIBGS_TEST_CHECK_THROWS(version::from_string("9.9"), libgs::runtime_error);
}

void parser_reuse()
{
	using namespace libgs::http;
	constexpr std::string_view request =
		"GET /repeat?q=42 HTTP/1.1\r\n"
		"Host: example.test\r\n\r\n";
	server_parser parser(32);

	for(size_t round = 0; round < 2'000; ++round)
	{
		auto result = parser.append(buffer(request));
		LIBGS_TEST_CHECK(result and *result);
		LIBGS_TEST_CHECK_EQ(parser.method(), method::get);
		LIBGS_TEST_CHECK_EQ(parser.path(), "/repeat");
		auto parameter = parser.parameter("q");
		LIBGS_TEST_CHECK(parameter);
		LIBGS_TEST_CHECK_EQ(parameter->to_int().value_or(0), 42);
		LIBGS_TEST_CHECK(parser.keep_alive());
		LIBGS_TEST_CHECK(parser.take_body().empty());
		parser.reset();
		LIBGS_TEST_CHECK_EQ(parser.stage(), stage::header);
	}
}

void request_parser()
{
	using namespace libgs::http;
	const std::string head =
		"POST /users/42?q=hello%20world&flag HTTP/1.1\r\n"
		"Host: example.test\r\n"
		"Accept-Encoding: br, gzip\r\n"
		"Cookie: sid=abc; theme=dark\r\n"
		"Content-Length: 5\r\n\r\n";
	server_parser parser;
	auto partial = parser.append(buffer(std::string_view(head).substr(0, 20)));
	LIBGS_TEST_CHECK(partial and not *partial);
	auto complete = parser.append(buffer(std::string_view(head).substr(20)));
	LIBGS_TEST_CHECK(complete and *complete);
	LIBGS_TEST_CHECK_EQ(parser.stage(), stage::body);
	complete = parser.append(buffer("hello"));
	LIBGS_TEST_CHECK(complete and *complete);
	LIBGS_TEST_CHECK_EQ(parser.stage(), stage::body);

	LIBGS_TEST_CHECK_EQ(parser.method(), method::post);
	LIBGS_TEST_CHECK_EQ(parser.version(), version::v11);
	LIBGS_TEST_CHECK_EQ(parser.path(), "/users/42");
	LIBGS_TEST_CHECK_EQ(parser.parameter("q")->to_string(), "hello world");
	LIBGS_TEST_CHECK(parser.contains_parameter("flag"));
	LIBGS_TEST_CHECK_EQ(parser.cookie("sid")->to_string(), "abc");
	LIBGS_TEST_CHECK(parser.support_gzip());
	LIBGS_TEST_CHECK(parser.keep_alive());
	LIBGS_TEST_CHECK(parser.path_match("/users/{id}") >= 0);
	LIBGS_TEST_CHECK_EQ(parser.path_arg("id")->to_string(), "42");
	LIBGS_TEST_CHECK_EQ(parser.take_body(), "hello");
	LIBGS_TEST_CHECK_EQ(parser.stage(), stage::finished);
}

void response_parser()
{
	using namespace libgs::http;
	const std::string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Set-Cookie: sid=abc; Path=/; HttpOnly\r\n\r\n"
		"5\r\nhello\r\n6; trace=yes\r\n world\r\n0\r\nX-End: yes\r\n\r\n";
	client_parser parser;
	auto parsed = parser.append(buffer(response));
	LIBGS_TEST_CHECK(parsed and *parsed);
	LIBGS_TEST_CHECK_EQ(parser.status(), status::ok);
	LIBGS_TEST_CHECK_EQ(parser.version(), version::v11);
	LIBGS_TEST_CHECK(parser.is_chunked());
	LIBGS_TEST_CHECK(parser.keep_alive());
	LIBGS_TEST_CHECK_EQ(parser.take_body(), "hello world");
	LIBGS_TEST_CHECK_EQ(parser.set_cookies().size(), 1U);
	LIBGS_TEST_CHECK_EQ(parser.set_cookies().front().first, "sid");
	LIBGS_TEST_CHECK_EQ(
		parser.set_cookies().front().second.value().to_string(), "abc"
	);
	LIBGS_TEST_CHECK_EQ(*parser.set_cookies().front().second.path(), "/");
	LIBGS_TEST_CHECK(*parser.set_cookies().front().second.http_only());
}

void generators_round_trip()
{
	using namespace libgs::http;
	request_arg argument;
	argument.set_header("X-Request", "yes")
		.set_cookie("sid", "token");
	client_generator request(libgs::url("http://example.test/api?q=42"), argument);
	std::string request_data = request.header_data(method::post, 7);
	request_data += request.body_data(buffer("payload"));
	LIBGS_TEST_CHECK_EQ(request.pro_state(), generator_state::finish);

	server_parser request_parser;
	auto request_result = request_parser.append(buffer(request_data));
	LIBGS_TEST_CHECK(request_result and *request_result);
	LIBGS_TEST_CHECK_EQ(request_parser.path(), "/api");
	LIBGS_TEST_CHECK_EQ(request_parser.parameter("q")->to_int().value_or(0), 42);
	LIBGS_TEST_CHECK_EQ(request_parser.header("X-Request")->to_string(), "yes");
	LIBGS_TEST_CHECK_EQ(request_parser.cookie("sid")->to_string(), "token");
	LIBGS_TEST_CHECK_EQ(request_parser.take_body(), "payload");

	server_generator response;
	response.set_status(status::created)
		.set_header("X-Response", "yes")
		.set_cookie("session", cookie("ready").set_path("/"));
	std::string response_data = response.header_data(2);
	response_data += response.body_data(buffer("ok"));
	LIBGS_TEST_CHECK_EQ(response.pro_state(), generator_state::finish);

	client_parser response_parser;
	auto response_result = response_parser.append(buffer(response_data));
	LIBGS_TEST_CHECK(response_result and *response_result);
	LIBGS_TEST_CHECK_EQ(response_parser.status(), status::created);
	LIBGS_TEST_CHECK_EQ(response_parser.header("X-Response")->to_string(), "yes");
	LIBGS_TEST_CHECK_EQ(response_parser.take_body(), "ok");
	LIBGS_TEST_CHECK_EQ(response_parser.set_cookies().front().first, "session");
}

void request_url_boundaries()
{
	using namespace libgs::http;
	const libgs::url target(
		"http://example.test/a%2Fb//c?flag&empty=#client-fragment"
	);

	client_generator origin_form(target, request_arg {});
	auto origin_head = origin_form.header_data(method::get, 0);
	LIBGS_TEST_CHECK(
		origin_head.starts_with("GET /a%2Fb//c?flag&empty= HTTP/1.1\r\n")
	);
	LIBGS_TEST_CHECK(
		origin_head.find("Host: example.test\r\n") != std::string::npos
	);
	LIBGS_TEST_CHECK(
		origin_head.find("example.test:0") == std::string::npos
	);
	LIBGS_TEST_CHECK(
		origin_head.find("client-fragment") == std::string::npos
	);

	client_generator absolute_form(target, request_arg {});
	absolute_form.set_target_form(request_target_form::absolute);
	auto absolute_head = absolute_form.header_data(method::get, 0);
	LIBGS_TEST_CHECK(
		absolute_head.starts_with(
			"GET http://example.test/a%2Fb//c?flag&empty= HTTP/1.1\r\n"
		)
	);
}

void range_headers()
{
	using namespace libgs::http;
	auto specifier = parse_range_header("bytes=0-9, 20-, -5");
	LIBGS_TEST_CHECK(specifier);
	LIBGS_TEST_CHECK_EQ(specifier->unit, "bytes");
	LIBGS_TEST_CHECK_EQ(specifier->ranges.size(), 3U);

	const auto resolved = resolve_byte_ranges(*specifier, 30);
	LIBGS_TEST_CHECK_EQ(resolved.size(), 3U);
	LIBGS_TEST_CHECK_EQ(resolved[0].begin, 0U);
	LIBGS_TEST_CHECK_EQ(resolved[0].total, 10U);
	LIBGS_TEST_CHECK_EQ(resolved[1].begin, 20U);
	LIBGS_TEST_CHECK_EQ(resolved[1].total, 10U);
	LIBGS_TEST_CHECK_EQ(resolved[2].begin, 25U);
	LIBGS_TEST_CHECK_EQ(resolved[2].total, 5U);
	LIBGS_TEST_CHECK_EQ(format_content_range(resolved[0], 30), "bytes 0-9/30");
	LIBGS_TEST_CHECK_EQ(format_unsatisfied_content_range(30), "bytes */30");

	auto content = parse_content_range("bytes 10-19/30");
	LIBGS_TEST_CHECK(content and content->satisfied);
	LIBGS_TEST_CHECK_EQ(content->length(), 10U);
	LIBGS_TEST_CHECK_EQ(*content->complete_length, 30U);
	LIBGS_TEST_CHECK(not parse_range_header("bytes=9-2"));
}

void conditional_and_upgrade_headers()
{
	using namespace libgs::http;
	auto tag = parse_entity_tag("W/\"revision-1\"");
	LIBGS_TEST_CHECK(tag and tag->weak);
	LIBGS_TEST_CHECK_EQ(tag->opaque, "revision-1");
	LIBGS_TEST_CHECK(not strong_entity_tag_equal("W/\"x\"", "\"x\""));
	LIBGS_TEST_CHECK(weak_entity_tag_equal("W/\"x\"", "\"x\""));

	const auto point = std::chrono::system_clock::time_point(std::chrono::seconds(784111777));
	const auto date = format_http_date(point);
	LIBGS_TEST_CHECK(parse_http_date(date).has_value());

	headers representation {{header::etag, "\"revision-1\""}};
	headers request {{header::if_none_match, "W/\"revision-1\""}};
	LIBGS_TEST_CHECK_EQ(
		evaluate_preconditions(method::get, request, representation),
		precondition_result::not_modified
	);
	LIBGS_TEST_CHECK_EQ(
		evaluate_preconditions(method::post, request, representation),
		precondition_result::precondition_failed
	);

	headers upgrade {
		{header::connection, "keep-alive, Upgrade"},
		{header::upgrade, "websocket"}
	};
	LIBGS_TEST_CHECK(header_has_token(upgrade, header::connection, "upgrade"));
	LIBGS_TEST_CHECK(is_upgrade_request(upgrade));
	LIBGS_TEST_CHECK(is_upgrade_response(status::switching_protocols, upgrade));
	LIBGS_TEST_CHECK_EQ(upgrade_protocol(upgrade).value_or(""), "websocket");
}

void form_data_and_authentication()
{
	using namespace libgs::http;
	multipart_form_data form("libgs-boundary");
	form.add_field("title", "hello")
		.add_file("upload", "a.txt", "file-data", "text/plain");
	LIBGS_TEST_CHECK_EQ(form.parts().size(), 2U);
	LIBGS_TEST_CHECK_EQ(form_data_boundary(form.content_type()).value_or(""), "libgs-boundary");
	auto parsed = parse_multipart_form_data(form.content_type(), form.body());
	LIBGS_TEST_CHECK(parsed);
	LIBGS_TEST_CHECK_EQ(parsed->size(), 2U);
	LIBGS_TEST_CHECK_EQ((*parsed)[0].name, "title");
	LIBGS_TEST_CHECK_EQ((*parsed)[0].data, "hello");
	LIBGS_TEST_CHECK_EQ((*parsed)[1].filename, "a.txt");
	LIBGS_TEST_CHECK_EQ((*parsed)[1].data, "file-data");

	request_arg argument;
	argument.set_basic_auth("user", "password");
	const auto basic = argument.header(header::authorization)->to_string();
	if(basic != "Basic dXNlcjpwYXNzd29yZA==")
		throw std::runtime_error("unexpected Basic credentials: " + basic);
	argument.set_bearer_auth("token");
	LIBGS_TEST_CHECK_EQ(
		argument.header(header::authorization)->to_string(), "Bearer token"
	);
	form.apply(argument);
	LIBGS_TEST_CHECK_EQ(argument.header(header::content_type)->to_string(), form.content_type());
}

void cookie_storage_policy()
{
	using namespace libgs::http;
	cookie_jar jar;
	const libgs::url origin("https://api.example.test/account/login");
	LIBGS_TEST_CHECK(jar.store(origin, "host", cookie("one").set_path("/account")));
	LIBGS_TEST_CHECK(jar.store(origin, "domain",
		cookie("two").set_domain("example.test").set_path("/")
	));
	LIBGS_TEST_CHECK(jar.store(origin, "secure", cookie("three").set_secure(true)));
	LIBGS_TEST_CHECK(not jar.store(
		libgs::url("http://api.example.test/"), "invalid-secure", cookie("x").set_secure(true)
	));
	LIBGS_TEST_CHECK_EQ(jar.size(), 3U);

	auto account = jar.cookies_for(libgs::url("https://api.example.test/account/profile"));
	LIBGS_TEST_CHECK_EQ(account.at("host").to_string(), "one");
	LIBGS_TEST_CHECK_EQ(account.at("domain").to_string(), "two");
	LIBGS_TEST_CHECK_EQ(account.at("secure").to_string(), "three");
	auto sibling = jar.cookies_for(libgs::url("https://www.example.test/"));
	LIBGS_TEST_CHECK(not sibling.contains("host"));
	LIBGS_TEST_CHECK(sibling.contains("domain"));
	auto plain = jar.cookies_for(libgs::url("http://api.example.test/account/profile"));
	LIBGS_TEST_CHECK(not plain.contains("secure"));

	LIBGS_TEST_CHECK(jar.store(origin, "host", cookie("gone").set_path("/account").set_max_age(0)));
	LIBGS_TEST_CHECK(not jar.cookies_for(origin).contains("host"));
	jar.clear();
	LIBGS_TEST_CHECK_EQ(jar.size(), 0U);
}

} //namespace

int main()
{
	return libgs::test::run({
		{"parser errors", parser_errors},
		{"enum input validation", enum_input_validation},
		{"parser reuse", parser_reuse},
		{"request parser", request_parser},
		{"response parser", response_parser},
		{"generators round trip", generators_round_trip},
		{"request URL boundaries", request_url_boundaries},
		{"range headers", range_headers},
		{"conditional and upgrade headers", conditional_and_upgrade_headers},
		{"form data and authentication", form_data_and_authentication},
		{"cookie storage policy", cookie_storage_policy},
	});
}
