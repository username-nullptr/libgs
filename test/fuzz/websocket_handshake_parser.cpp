// SPDX-FileCopyrightText: 2026 Xiaoqiang <username_nullptr@163.com>
// SPDX-License-Identifier: MIT

#include <libgs/websocket/protocol/handshake.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if( size == 0 )
		return 0;

	const std::string value(reinterpret_cast<const char*>(data + 1), size - 1);
	libgs::http::headers headers {
		{libgs::http::header::connection, "Upgrade"},
		{libgs::http::header::upgrade, "websocket"},
		{"Sec-WebSocket-Version", "13"},
		{"Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ=="},
	};
	switch(data[0] % 6)
	{
	case 0: headers[libgs::http::header::connection] = value; break;
	case 1: headers[libgs::http::header::upgrade] = value; break;
	case 2: headers["Sec-WebSocket-Version"] = value; break;
	case 3: headers["Sec-WebSocket-Key"] = value; break;
	case 4: headers["Sec-WebSocket-Protocol"] = value; break;
	default: headers["Sec-WebSocket-Extensions"] = value; break;
	}

	auto request = libgs::websocket::parse_opening_request(
		libgs::http::method::get, libgs::http::version::v11, headers);
	if( not request )
		return 0;

	auto generated = libgs::websocket::make_opening_request_headers(*request);
	if( generated )
		libgs::ignore_unused(libgs::websocket::parse_opening_request(
			libgs::http::method::get, libgs::http::version::v11, *generated));

	libgs::websocket::opening_response response;
	if( not request->subprotocols.empty() )
		response.subprotocol = request->subprotocols.front();
	if( not request->extensions.empty() )
		response.extensions.push_back(request->extensions.front());
	libgs::ignore_unused(
		libgs::websocket::make_opening_response_headers(*request, response));
	return 0;
}
