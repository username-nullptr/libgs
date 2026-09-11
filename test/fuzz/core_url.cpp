// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/core/algorithm/misc.h>
#include <libgs/core/url.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if( size == 0 )
		return 0;

	const std::string input(reinterpret_cast<const char*>(data), size);

	const auto encoded = libgs::to_percent_encoding(input);
	if( libgs::from_percent_encoding(encoded) != input )
		std::abort();

	libgs::url parsed(input);
	if( not parsed.is_valid() )
		return 0;

	const auto serialized = parsed.to_string();
	libgs::url reparsed(serialized);
	if( not reparsed.is_valid() or reparsed.to_string() != serialized )
		std::abort();

	libgs::url copied(parsed);
	libgs::url moved(std::move(copied));
	if( moved.to_string() != serialized )
		std::abort();

	const auto split = size / 2;
	libgs::ignore_unused(libgs::url::resolve(
		parsed, std::string_view(input).substr(split)));
	return 0;
}
