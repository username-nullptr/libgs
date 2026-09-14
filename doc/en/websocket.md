# WebSocket

Language: English | [简体中文](../zh_CN/websocket.md)

`gs.websocket` implements RFC 6455 over HTTP/1.1. It provides protocol codecs,
a message stream, owned clients and servers, and helpers for upgrading an
existing HTTP connection.

## Public structure

| Header | Purpose |
| --- | --- |
| `<libgs/websocket/protocol/...>` | Opening handshakes and frame parsing/generation |
| `<libgs/websocket/stream.h>` | Message, frame, control, close, and lifecycle I/O |
| `<libgs/websocket/client.h>` | WS/WSS opening, redirects, cookies, proxies, and diagnostics |
| `<libgs/websocket/server.h>` | Owned server and HTTP Upgrade helpers |
| `<libgs/websocket/types.h>` | Stream, message, frame, compression, and close configuration |
| `<libgs/websocket.h>` | Client and server umbrella |

Link `gs.websocket`; it publicly depends on `gs.http`.
`LIBGS_OPENSSL_SUPPORT` enables WSS and TLS server aliases.

## Client

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

`client` owns an HTTP/1.1 client. The free `websocket::open(http_client, ...)`
operation reuses an application-owned HTTP client, connector, cookie jar, and
connection policy.

`connect_request` controls headers/authentication, proxy selection, stream
configuration, handshake timeout, redirects, subprotocols, and extensions.
`open_diagnostics` retains the final endpoint and HTTP reply when an opening
handshake needs inspection.

The default proxy policy reads `ws_proxy`/`wss_proxy`, then the corresponding
HTTP/HTTPS proxy variables, and finally `all_proxy`; `no_proxy` is honored. An
explicit `proxy_config` supports HTTP forward/CONNECT and SOCKS5 routing with
Basic credentials; HTTP proxies also support Bearer credentials.

## Server and Upgrade

`websocket::server` owns an HTTP listener and opening handshakes. It can deliver
connections either through `accept()` or through path handlers registered with
`on_connection()` and `on_default()`; do not mix the two delivery models on one
server.

For a mixed HTTP/WebSocket service, route with `http::server`, test
`is_upgrade_request()`, and call `websocket::upgrade(context, options)`. A
successful upgrade returns `accept_result`, containing the stream, an immutable
request snapshot, and the selected subprotocol/extensions.

`upgrade_options` controls:

- stream configuration and handshake timeout;
- supported or required subprotocols and extensions;
- additional response headers;
- synchronous or awaitable request/Origin validation; and
- synchronous or awaitable subprotocol/extension selection.

Synchronous `upgrade()` cannot run awaitable validators or selectors. A
selected protocol or extension must also belong to the configured allowlist.

## Stream

| Operation | Result |
| --- | --- |
| `read<Buffer>()` | One complete text or binary message |
| `consume()` | One message delivered in temporary chunks |
| `read_frame<Buffer>()` | One text, binary, or continuation data frame |
| `write_text()`, `write_binary()`, `write()` | One complete message |
| `write_frame()` | One data frame with explicit fragmentation state |
| `ping()`, `pong()` | Explicit control frame |
| `close()` | RFC close handshake |
| `shutdown()` | Immediate transport shutdown |
| `wait_written()` | Completion/errors of accepted queued writes |
| `wait_closed()`, `on_closed()` | Terminal close information |

Important rules:

- Only one of `read()`, `consume()`, or `read_frame()` may be active.
- `message_chunk::body` is valid only during its `consume()` callback.
- Ping/Pong and close callbacks observe control traffic processed by an active
  read; they do not start their own transport read.
- Complete-message reads assemble continuation frames and validate message
  limits. Frame reads preserve data-frame boundaries.
- Writes are serialized and bounded by `max_queued_write_bytes` and
  `max_queued_write_operations`.
- Shared access from multiple threads must be serialized with a strand or lock.

`stream_config` sets frame/message limits, read buffer size, outgoing fragment
size, write-queue limits, automatic Ping/Pong, close timeout, and compression
policy.

## Compression

RFC 7692 `permessage-deflate` is available when WebSocket zlib support is
enabled directly or inherited from HTTP zlib. Both peers must negotiate it:

```cpp
auto extension = libgs::websocket::permessage_deflate_extension();

libgs::websocket::connect_request request("ws://127.0.0.1:8080/echo");
request.extensions = {extension};

libgs::websocket::upgrade_options options;
options.supported_extensions = {extension};
```

`permessage_deflate_options` controls context takeover and window bits.
`compression_config` controls automatic message selection, threshold, and zlib
level; `write_options` can override compression for one complete message.

## Implemented boundaries

The module supports client/server masking rules, fragmentation, UTF-8 checks,
Ping/Pong/Close, WS/WSS, redirects, cookies, subprotocol negotiation,
`permessage-deflate`, HTTP proxies, SOCKS5, timeouts, cancellation, and bounded
queues.

It does not implement WebSocket over HTTP/2 or HTTP/3, extensions other than
`permessage-deflate`, Pong-deadline policy, reconnect, or application routing.

## Examples

- [Client](../../examples/websocket/client.cpp)
- [Server](../../examples/websocket/server.cpp)
- [Mixed HTTP client](../../examples/websocket/mixed_http_client.cpp)
- [Mixed HTTP server](../../examples/websocket/mixed_http_server.cpp)
- [Proxy client](../../examples/websocket/proxy_client.cpp)
- [Protocol codecs](../../examples/websocket/protocol.cpp)
- [WSS client](../../examples/websocket/wss_client.cpp)
- [WSS server](../../examples/websocket/wss_server.cpp)
