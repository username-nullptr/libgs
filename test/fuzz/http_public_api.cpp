// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/http/protocol/utils.h>
#include <libgs/http/protocol/utils/core/range.h>
#include <libgs/http/utils/connection.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size == 0 or size > LIBGS_FUZZ_MAX_LENGTH)
		return 0;

	const std::string input(reinterpret_cast<const char*>(data), size);
	const size_t split = size_t(data[0]) % size;
	const auto left = std::string_view(input).substr(0, split);
	const auto right = std::string_view(input).substr(split);

	libgs::http::endpoint endpoint;
	if(endpoint.from_string(input))
	{
		libgs::http::endpoint reparsed;
		if(not reparsed.from_string(endpoint.to_string()) or
			reparsed.to_string() != endpoint.to_string())
			std::abort();
	}
	const auto parsed_method = libgs::http::method::from_string(input);
	if(parsed_method == libgs::http::method::none)
	{
		bool rejected = false;
		try {
			libgs::ignore_unused(libgs::http::method::from_string(input, true));
		}
		catch(const libgs::invalid_argument&) {
			rejected = true;
		}
		if(not rejected)
			std::abort();
	}
	else if(libgs::http::method::from_string(input, true) != parsed_method)
	{
		std::abort();
	}
	else if(libgs::http::method::from_string(
		libgs::http::method::string(parsed_method), true) != parsed_method)
	{
		std::abort();
	}
	const auto ranges = libgs::http::parse_range_header(input);
	if(ranges)
	{
		const size_t complete_length = 1 +
			(size > 1 ? (size_t(data[0]) << 8U) | data[1] : data[0]);
		for(const auto &range : libgs::http::resolve_byte_ranges(
			*ranges, complete_length))
		{
			if(range.total == 0 or range.begin >= complete_length or
				range.total > complete_length - range.begin)
				std::abort();
		}
	}
	const auto content_range = libgs::http::parse_content_range(input);
	if(content_range and content_range->satisfied)
	{
		if(content_range->length() == 0 or
			content_range->length() != content_range->last - content_range->first + 1)
			std::abort();
		if(content_range->complete_length)
		{
			const auto formatted = libgs::http::format_content_range(
				{content_range->first, content_range->length()},
				*content_range->complete_length);
			const auto reparsed = libgs::http::parse_content_range(formatted);
			if(not reparsed or reparsed->first != content_range->first or
				reparsed->last != content_range->last or
				reparsed->complete_length != content_range->complete_length)
				std::abort();
		}
	}
	libgs::ignore_unused(libgs::http::parse_multipart_byte_ranges_boundary(input));
	libgs::ignore_unused(libgs::http::parse_entity_tag(input));
	libgs::ignore_unused(libgs::http::parse_http_date(input));
	libgs::ignore_unused(libgs::http::content_coding_quality(input, right));
	libgs::ignore_unused(libgs::http::form_data_boundary(input));
	libgs::ignore_unused(libgs::http::parse_multipart_form_data(left, right));

	libgs::http::headers headers;
	headers[libgs::http::header::connection] = std::string(left);
	headers[libgs::http::header::upgrade] = std::string(right);
	headers[libgs::http::header::if_match] = input;
	libgs::ignore_unused(libgs::http::header_has_token(
		headers, libgs::http::header::connection, right));
	libgs::ignore_unused(libgs::http::is_upgrade_request(headers));
	libgs::ignore_unused(libgs::http::is_upgrade_response(
		libgs::http::status::switching_protocols, headers));
	libgs::ignore_unused(libgs::http::upgrade_protocol(headers));
	libgs::ignore_unused(libgs::http::evaluate_preconditions(
		libgs::http::method::get, headers, headers, (data[0] & 1U) != 0));

	libgs::http::cookie cookie(input);
	cookie.set_domain(std::string(left)).set_path(std::string(right));
	cookie.set_same_site(input).set_priority(std::string(left));
	cookie.set_expires(size).set_max_age(size).set_size(size);
	cookie.set_http_only((data[0] & 1U) != 0).set_secure((data[0] & 2U) != 0);
	cookie.set_attribute(std::string(left), std::string(right));
	libgs::ignore_unused(cookie.attribute(std::string(left)));
	libgs::ignore_unused(cookie.attributes());
	cookie.unset_domain().unset_path().unset_same_site().unset_priority();
	cookie.unset_expires().unset_max_age().unset_size();
	cookie.unset_http_only().unset_secure().unset_attribute(std::string(left));
	return 0;
}
