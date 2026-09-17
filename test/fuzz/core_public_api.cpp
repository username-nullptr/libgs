// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/url.h>
#include <libgs/core/value.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size == 0 or size > 4'096)
		return 0;

	const std::string input(reinterpret_cast<const char*>(data), size);
	const size_t split = size_t(data[0]) % size;
	const auto left = std::string_view(input).substr(0, split);
	const auto right = std::string_view(input).substr(split);

	const char percent = static_cast<char>((data[0] % 94) + 33);
	const auto encoded = libgs::to_percent_encoding(input, {}, {}, percent);
	if(libgs::from_percent_encoding(encoded, percent) != input)
		std::abort();
	libgs::ignore_unused(libgs::from_percent_encoding(input, percent));
	libgs::ignore_unused(libgs::wildcard_match(left, right));

	libgs::value value(input);
	const size_t base = size_t(data[0] % 35) + 2;
	libgs::ignore_unused(value.to_bool(base));
	libgs::ignore_unused(value.to_int(base));
	libgs::ignore_unused(value.to_uint(base));
	libgs::ignore_unused(value.to_long(base));
	libgs::ignore_unused(value.to_ulong(base));
	libgs::ignore_unused(value.to_float());
	libgs::ignore_unused(value.to_double());
	libgs::ignore_unused(value.to_ldouble());
	libgs::ignore_unused(value.is_alpha());
	libgs::ignore_unused(value.is_digit());
	libgs::ignore_unused(value.is_rlnum());
	libgs::ignore_unused(value.is_alnum());
	libgs::ignore_unused(value.is_ascii());
	value.set("{}:{}", left, right);

	libgs::url parsed(input);
	parsed.set_address(std::string(left));
	parsed.set_port(static_cast<uint16_t>((size > 1 ? data[1] : data[0]) * 257U));
	try {
		parsed.set_path(right);
	}
	catch(const libgs::invalid_argument&) {}
	try {
		parsed.set_fragment(left);
	}
	catch(const libgs::invalid_argument&) {}
	parsed.set_parameter(std::string(left), std::string(right));
	libgs::ignore_unused(parsed.parameter(std::string(left)));
	libgs::ignore_unused(parsed.encoded_path());
	libgs::ignore_unused(parsed.encoded_query());
	const auto serialized = parsed.to_string();
	if(parsed.is_valid())
	{
		libgs::url reparsed(serialized);
		if(not reparsed.is_valid() or reparsed.to_string() != serialized)
			std::abort();
	}
	parsed.clear_fragment().unset_parameter(std::string(left));
	libgs::ignore_unused(libgs::url::resolve(parsed, right));
	return 0;
}
