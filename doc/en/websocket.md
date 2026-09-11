# WebSocket Client, Server, and Stream

Language: English | [Simplified Chinese](../zh_CN/websocket.md)

The `gs.websocket` module implements the RFC 6455 WebSocket baseline over
HTTP/1.1. It provides standalone WS/WSS clients and servers, upgrade helpers for
mixed HTTP applications, protocol codecs, complete-message and data-frame I/O,
control frames, timeouts, cancellation, and bounded write queues.

Link `gs.websocket`; it publicly depends on `gs.http` and `gs.core`. Include
`<libgs/websocket.h>` for the client/server umbrella or a narrower header when
only protocol codecs or `websocket::stream` are needed.

## Implemented scope

- `ws://` and, with `LIBGS_OPENSSL_SUPPORT`, `wss://` endpoints;
- HTTP/1.1 client and server opening handshakes;
- standalone `websocket::client` and `websocket::server` ownership;
- mixed HTTP/WebSocket use through `websocket::open()` and
  `websocket::upgrade()`;
- text, binary, continuation, Ping, Pong, and Close frames;
- fragmented-message assembly and configurable outgoing fragmentation;
- complete-message `read()` and data-frame `read_frame()` interfaces;
- client masking, UTF-8 validation, frame/message size limits, and protocol
  failure Close responses;
- subprotocol negotiation, synchronous/asynchronous Upgrade validators,
  redirects, cookies, and opening diagnostics;
- optional RFC 7692 `permessage-deflate` with context takeover disabled in both
  directions;
- synchronous and Asio completion-token operation styles; and
- cancellation, close deadlines, write-queue limits, and write barriers.

## Client example

```cpp
#include <libgs/websocket/client.h>

#include <iostream>

int main()
{
    libgs::websocket::client client;

    libgs::dispatch([&client]() -> libgs::awaitable<void>
    {
        auto stream = co_await client.open(
            "ws://127.0.0.1:8080/echo", libgs::use_awaitable
        );
        co_await stream.write_text("hello", libgs::use_awaitable);
        auto reply = co_await stream.read<std::string>(libgs::use_awaitable);
        std::cout << reply.body << '\n';
        co_await stream.close(libgs::use_awaitable);
        libgs::exit();
    });

    return libgs::exec();
}
```

`websocket::client` owns its HTTP client. To share an HTTP connection policy,
cookie jar, or connector with ordinary requests, pass an HTTP/1.1 client to the
free `websocket::open()` operation instead.

## Server and mixed upgrades

`websocket::server` owns the listener and can deliver accepted streams through
`accept()` or route handlers registered with `on_connection()`/`on_default()`.
These delivery modes are mutually exclusive for one server.

An existing `http::server` route can call `websocket::upgrade(context, options)`.
On success the helper validates and writes the opening response, transfers the
connection and any pending bytes out of HTTP, and returns an adopted WebSocket
stream.

`upgrade_options` supports request and Origin validators, subprotocol policy,
additional non-protocol response headers, stream limits, and a handshake
deadline. `request_validator` and `origin_validator` are synchronous;
`async_request_validator` and `async_origin_validator` may suspend in an
asynchronous `upgrade()` or owned-server handshake. A synchronous `upgrade()`
reports `std::errc::operation_not_supported` if an asynchronous validator is
configured. Selectors remain synchronous.

## Stream behavior

`stream::read<Buffer>()` returns one complete text or binary message. It hides
TCP segmentation and continuation frames, and consumes control frames while
assembling the message. Keep a read active if the application needs incoming
Ping/Pong processing; `wait_ctrl()` observes those events but does not start a
separate transport read.

`stream::read_frame<Buffer>()` instead returns each text, binary, or continuation
data frame. The result carries `fin`, `continuation`, and the effective message
type inherited by continuation frames. It processes control frames in the same
way as `read()` and is mutually exclusive with another active read. Because
decompression changes frame payload boundaries, `read_frame()` reports
`std::errc::operation_not_supported` on a compressed connection.

Incoming Ping frames are answered automatically by default. The stream exposes
`ping()` and `pong()`, but does not schedule periodic Ping, idle timeouts, or a
Pong deadline. Applications can compose those policies with Asio timers and
cancellation slots.

Concurrent writes are serialized at frame boundaries and bounded by
`stream_config` queue limits. `wait_written()` observes errors from previously
accepted writes, including detached writes. `close()` performs the RFC close
handshake; `shutdown()` immediately aborts operations and closes the transport.

Stream, client, and server objects follow Asio shared-object-unsafe rules. Calls
from multiple threads must be serialized with a strand or an external lock.

## Optional permessage-deflate

When `LIBGS_HTTP_ZLIB_SUPPORT=ON`, WebSocket inherits zlib support. If HTTP zlib
support is off, `LIBGS_WEBSOCKET_ZLIB_SUPPORT=ON` enables compression only for
WebSocket. Both peers opt into the implemented profile explicitly:

```cpp
auto compression = libgs::websocket::permessage_deflate_extension();

libgs::websocket::connect_request request("ws://127.0.0.1:8080/echo");
request.extensions = {compression};

libgs::websocket::upgrade_options options;
options.supported_extensions = {compression};
```

The server selects the profile only when the client offers it; a client accepts
an empty extension response when the server declines. The negotiated value is
available through `stream::negotiated_extensions()` and
`accept_result::handshake.extensions`.

## Current limits

The implemented RFC 7692 profile requires both
`server_no_context_takeover` and `client_no_context_takeover`. Context takeover,
window-bit negotiation, other parameter combinations, and other WebSocket
extensions are rejected as unsupported.

HTTP/2 and HTTP/3 extended CONNECT, WebSocket-specific proxy fields, automatic
keepalive, reconnect, and application message routing are not implemented.
Proxy or routing behavior supplied by an injected HTTP connector remains usable.

## Related material

- [WebSocket design and observable behavior](../../libgs/websocket/design.md)
- [Examples](../../examples/websocket)
- [Functional tests](../../test/functional/websocket)
- [Project roadmap](roadmap.md)
