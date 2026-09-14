# WebSocket Client, Server, and Stream

Language: English | [Simplified Chinese](../zh_CN/websocket.md)

The `gs.websocket` module implements the RFC 6455 WebSocket baseline over
HTTP/1.1. It provides standalone WS/WSS clients and servers, upgrade helpers for
mixed HTTP applications, protocol codecs, complete-message and streaming-message
reads, symmetric data-frame I/O, control frames, timeouts, cancellation, and
bounded write queues.

Link `gs.websocket`; its public dependency chain includes `gs.http`, `gs.coro`,
and `gs.core`. Include `<libgs/websocket.h>` for the client/server umbrella or a
narrower header when only protocol codecs or `websocket::stream` are needed.

## Implemented scope

- `ws://` and, with `LIBGS_OPENSSL_SUPPORT`, `wss://` endpoints;
- HTTP/1.1 client and server opening handshakes;
- standalone `websocket::client` and `websocket::server` ownership;
- mixed HTTP/WebSocket use through `websocket::open()` and
  `websocket::upgrade()`;
- text, binary, continuation, Ping, Pong, and Close frames;
- fragmented-message assembly and configurable outgoing fragmentation;
- complete-message `read()`, single-message streaming `consume()`, and
  data-frame `read_frame()` / `write_frame()` interfaces;
- client masking, UTF-8 validation, frame/message size limits, and protocol
  failure Close responses;
- subprotocol negotiation, synchronous/asynchronous Upgrade validators,
  redirects, cookies, and opening diagnostics;
- optional RFC 7692 `permessage-deflate` with context takeover disabled in both
  directions;
- synchronous and Asio completion-token operation styles; and
- Ping/Pong callbacks, periodic Ping, and configurable automatic Pong;
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

`websocket::client_config::no_delay` is a three-state option and defaults to
`true`: `true` enables `TCP_NODELAY`, `false` disables it, and `nullopt` keeps
the setting inherited from the underlying HTTP connection. The free
`websocket::open(http_client, ...)` operation always preserves the HTTP client
setting.

`client_config::stream` supplies defaults for streams created by the client;
`connect_request::stream_options` can override them for one connection. A
server uses `server_config::default_upgrade.stream` by default and may select
`upgrade_options::stream` for an individual Upgrade. The resulting stream
inherits the final configuration.

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
assembling the message. `on_ping()` and `on_pong()` observe or filter incoming
Ping/Pong frames: returning `true` accepts the frame and returning `false`
drops it. An unset callback accepts every frame. These callbacks do not start a
separate transport read, so keep one of `read()`, `read_frame()`, or `consume()`
active when control frames must be processed.

`stream::consume()` streams exactly one complete message per call. Its callback
receives `message_chunk`; `first` and `last` preserve message boundaries and
`offset` is the byte offset in the current message. A chunk is no larger than
`stream_config::read_buffer_size`; its actual boundary follows the current
transport read, not TCP segments or WebSocket frames. `body` is a non-owning
view valid only until the callback returns. The callback runs synchronously on
the stream executor and must return promptly; a later Ping cannot be read until
it returns. Copy data or hand it to application-owned bounded storage when it
must outlive the callback. If a later part of the message fails protocol
validation, is cancelled, or reaches EOF, earlier chunks have already been
delivered; treat successful completion as the message commit point. The
resulting `message_info` reports message type and total byte count.

```cpp
auto info = co_await stream.consume(
    [&output](const libgs::websocket::message_chunk &chunk)
    {
        output.write(static_cast<const char*>(chunk.body.data()),
            static_cast<std::streamsize>(chunk.body.size()));
    },
    libgs::use_awaitable
);
```

`stream::read_frame<Buffer>()` returns individual text, binary, or continuation
data frames; `stream::write_frame()` accepts the same
`basic_data_frame<Buffer>` shape. The value includes `fin`, `continuation`, and
the effective message type inherited by continuation frames. `write_frame()`
validates fragment ordering, text UTF-8 across frames, and whole-message size.
Wait for each frame write to complete before initiating the next frame of that
fragmented message. Frame I/O and complete-message I/O cannot be switched in the
middle of a fragmented message.

`read()`, `read_frame()`, and `consume()` cannot overlap. Because decompression
changes frame and chunk payload boundaries, compressed connections currently
return `std::errc::operation_not_supported` from `read_frame()`, `write_frame()`,
and `consume()`; complete-message `read()` and `write()` remain available.

`stream_config::auto_ping_interval` defaults to five seconds; zero disables
periodic Ping and a negative value is invalid. `stream_config::auto_pong`
defaults to enabled. An accepted Ping is answered with the same payload when
automatic Pong is enabled; a Ping rejected by `on_ping()` is not answered. A
Pong rejected by `on_pong()` is likewise dropped. Periodic Ping only sends the
control frame: it does not independently read Pong, enforce a Pong deadline, or
reconnect. Applications remain responsible for composing those policies. The
stream also exposes `ping()` and `pong()` for explicit control-frame writes.

Concurrent writes are serialized at frame boundaries and bounded by
`stream_config` queue limits. `wait_written()` observes errors from previously
accepted writes, including detached writes. `close()` performs the RFC close
handshake; `shutdown()` immediately aborts operations and closes the transport.
`wait_closed()` actively waits for the terminal result, while `on_closed()`
observes the same retained `close_info` without competing with any waiters. The
callback is delivered at most once; registering it after the stream has already
reached a terminal state delivers the retained result immediately. Like the
control callbacks, it does not start a transport read.

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

HTTP/2 and HTTP/3 extended CONNECT, WebSocket-specific proxy fields, Pong
deadlines, reconnect, and application message routing are not implemented.
Proxy or routing behavior supplied by an injected HTTP connector remains usable.

## Related material

- [WebSocket design and observable behavior](../../libgs/websocket/design.md)
- [Examples](../../examples/websocket)
- [Functional tests](../../test/functional/websocket)
- [Project roadmap](roadmap.md)
