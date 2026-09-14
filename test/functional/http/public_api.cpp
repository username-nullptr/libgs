// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include "test.h"

#include <libgs/http/client.h>
#include <libgs/http/cxx/configs.h>
#include <libgs/http/cxx/container.h>
#include <libgs/http/protocol/utils/core/compression.h>
#include <libgs/http/server/session_manager.h>
#include <libgs/http/utils/connection.h>
#include <libgs/http/utils/file_opt_token.h>
#include <libgs/http/utils/opt_token.h>

namespace
{

void endpoints_and_case_insensitive_containers()
{
	libgs::http::endpoint endpoint;
	LIBGS_TEST_CHECK(endpoint.from_string("127.0.0.1:8080"));
	LIBGS_TEST_CHECK_EQ(endpoint.to_string(), "127.0.0.1:8080");
	LIBGS_TEST_CHECK(endpoint.from_string("[::1]:443"));
	LIBGS_TEST_CHECK_EQ(endpoint.to_string(), "[::1]:443");
	LIBGS_TEST_CHECK(not endpoint.from_string("::1:443"));
	LIBGS_TEST_CHECK(not endpoint.from_string("127.0.0.1:65536"));
	LIBGS_TEST_CHECK(not endpoint.from_string("host.test:80"));

	libgs::http::value_map values {{"Content-Type", "text/plain"}};
	LIBGS_TEST_CHECK_EQ(
		libgs::http::value_map_get(values, "content-type")->to_string(),
		"text/plain"
	);
	LIBGS_TEST_CHECK(not libgs::http::value_map_get(values, "missing"));
	libgs::http::value_set names {"GZip", "br"};
	LIBGS_TEST_CHECK_EQ(
		libgs::http::value_set_get(names, "gzip")->to_string(), "GZip"
	);
}

void client_configuration()
{
	libgs::io_context_t context;
	libgs::http::client default_client(context.get_executor());
	LIBGS_TEST_CHECK(default_client.config().no_delay);

	libgs::http::client_config config;
	config.no_delay = false;
	libgs::http::client delayed_client(config);
	LIBGS_TEST_CHECK(not delayed_client.config().no_delay);
}

void content_coding_and_mime_policy()
{
	using namespace libgs::http;
	LIBGS_TEST_CHECK(std::abs(content_coding_quality(
		"br;q=0.4, gzip; q=0.8, *;q=0.1", "GZIP") - 0.8) < 0.0001);
	LIBGS_TEST_CHECK_EQ(content_coding_quality("*;q=0.5", "deflate"), 0.5);
	LIBGS_TEST_CHECK_EQ(content_coding_quality("gzip;q=1.1, *;q=0.2", "gzip"), 0.0);
	LIBGS_TEST_CHECK_EQ(content_coding_quality("br", "gzip"), 0.0);

	LIBGS_TEST_CHECK(is_compressible_mime_type("Text/Plain; charset=utf-8"));
	LIBGS_TEST_CHECK(is_compressible_mime_type("application/problem+json"));
	LIBGS_TEST_CHECK(not is_compressible_mime_type("image/png"));
	LIBGS_TEST_CHECK(is_precompressed_mime_type("application/pdf"));
	LIBGS_TEST_CHECK(not is_precompressed_mime_type("image/svg+xml"));
}

void gzip_codec()
{
	using namespace libgs::http;
	const std::string input = std::string(4096, 'a') + std::string(4096, 'b');
#if LIBGS_HTTP_ZLIB_SUPPORT
	auto compressed = gzip_compress(input);
	LIBGS_TEST_CHECK(compressed);
	LIBGS_TEST_CHECK(compressed->size() < input.size());
	LIBGS_TEST_CHECK_EQ(gzip_decompress(*compressed).value(), input);
	LIBGS_TEST_CHECK(not gzip_decompress("not gzip"));
	LIBGS_TEST_CHECK(not gzip_decompress(*compressed, input.size() - 1));

	gzip_encoder encoder;
	auto first = encoder.append(std::string_view(input).substr(0, 1024));
	auto last = encoder.append(std::string_view(input).substr(1024), true);
	LIBGS_TEST_CHECK(first and last and encoder.finished());
	gzip_decoder decoder(input.size());
	auto decoded_first = decoder.append(*first);
	auto decoded_last = decoder.append(*last, true);
	LIBGS_TEST_CHECK(decoded_first and decoded_last and decoder.finished());
	LIBGS_TEST_CHECK_EQ(*decoded_first + *decoded_last, input);
	LIBGS_TEST_CHECK_EQ(decoder.output_size(), input.size());
#else
	auto compressed = gzip_compress(input);
	LIBGS_TEST_CHECK(not compressed);
	LIBGS_TEST_CHECK(compressed.error() == std::errc::operation_not_supported);
	gzip_encoder encoder;
	LIBGS_TEST_CHECK(not encoder.append(input, true));
	LIBGS_TEST_CHECK(not encoder.finished());
	gzip_decoder decoder;
	LIBGS_TEST_CHECK(not decoder.append(input, true));
	LIBGS_TEST_CHECK_EQ(decoder.output_size(), 0U);
#endif
}

void file_option_tokens()
{
	using namespace libgs::http;
	using namespace libgs::http::operators;
	libgs::test::temporary_directory directory;
	const auto file = directory.path() / "payload.txt";
	{
		std::ofstream output(file, std::ios::binary);
		output << "0123456789";
	}

	auto single = make_file_opt_token(file, file_range {2, 4});
	static_assert(decltype(single)::optype == file_optype::single);
	LIBGS_TEST_CHECK(single.init(std::ios::in | std::ios::binary));
	LIBGS_TEST_CHECK_EQ(single.file_size, 10U);
	LIBGS_TEST_CHECK_EQ(single.mime_type, "text/plain");
	LIBGS_TEST_CHECK_EQ(single.range->begin, 2U);

	auto multiple = file | file_range {0, 2} | file_range {8, 2};
	static_assert(decltype(multiple)::optype == file_optype::multiple);
	LIBGS_TEST_CHECK(multiple.init(std::ios::in | std::ios::binary));
	LIBGS_TEST_CHECK_EQ(multiple.ranges.size(), 2U);
	LIBGS_TEST_CHECK_EQ(multiple.file_size, 10U);

	std::ifstream input(file, std::ios::binary);
	auto borrowed = make_file_opt_token(input, file_ranges {{1, 2}, {5, 3}});
	LIBGS_TEST_CHECK(borrowed.init(std::ios::in | std::ios::binary));
	LIBGS_TEST_CHECK(borrowed.stream == &input);
	LIBGS_TEST_CHECK_EQ(borrowed.ranges.size(), 2U);

	auto missing = make_file_opt_token(file.string() + ".missing");
	auto missing_result = missing.init(std::ios::in | std::ios::binary);
	LIBGS_TEST_CHECK(not missing_result);
	LIBGS_TEST_CHECK(missing_result.error() == std::errc::no_such_file_or_directory);

	path_opt_token paths("/one", "/two");
	LIBGS_TEST_CHECK_EQ(paths.paths.size(), 2U);
	LIBGS_TEST_CHECK_EQ(paths.paths[1], "/two");
}

void sessions()
{
	using namespace std::chrono_literals;
	libgs::io_context_t context;
	libgs::http::session_manager manager;
	manager.set_lifecycle(1500ms).set_cookie_key("sid");
	LIBGS_TEST_CHECK_EQ(manager.lifecycle(), 1s);
	LIBGS_TEST_CHECK_EQ(manager.cookie_key(), "sid");
	LIBGS_TEST_CHECK_THROWS(manager.set_cookie_key(""), libgs::runtime_error);

	auto session = manager.make(context.get_executor());
	const std::string id(session->id());
	LIBGS_TEST_CHECK(not id.empty());
	LIBGS_TEST_CHECK(session->is_valid());
	LIBGS_TEST_CHECK_EQ(session->lifecycle(), 1s);
	session->set_attribute("counter", 42).set_attribute("name", std::string("test"));
	LIBGS_TEST_CHECK_EQ(std::any_cast<int>(*session->attribute("COUNTER")), 42);
	LIBGS_TEST_CHECK_EQ(std::any_cast<std::string>(*session->attribute("name")), "test");
	session->unset_attribute("counter");
	LIBGS_TEST_CHECK(not session->attribute("counter"));
	LIBGS_TEST_CHECK(manager.get(id) == session);
	LIBGS_TEST_CHECK(not manager.get_or("missing"));
	LIBGS_TEST_CHECK_THROWS(manager.get("missing"), libgs::runtime_error);
	session->invalidate();
	LIBGS_TEST_CHECK(not session->is_valid());
}

} //namespace

int main()
{
	return libgs::test::run({
		{"endpoints and case-insensitive containers", endpoints_and_case_insensitive_containers},
		{"client configuration", client_configuration},
		{"content coding and MIME policy", content_coding_and_mime_policy},
		{"gzip codec", gzip_codec},
		{"file option tokens", file_option_tokens},
		{"sessions", sessions},
	});
}
