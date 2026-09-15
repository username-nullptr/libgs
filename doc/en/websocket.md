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
| `<libgs/websocket/retry.h>` | Explicit retry of a failed connection opening |
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

### Explicit connection recovery

Connection validity is an application decision. The initial `client.open()` is
therefore attempted exactly once, and the library does not own the business
receive loop. After that loop determines that its stream is unusable, it can
pause application work and explicitly call `retry_open()`:

```cpp
libgs::websocket::client client;
auto stream = co_await client.open(
    libgs::websocket::connect_request("wss://example.test/events"),
    libgs::use_awaitable); // one initial attempt

auto error = co_await run_business(stream);
if (connection_is_invalid(error)) {
    pause_business();

    libgs::websocket::retry_open_options options;
    options.initial_delay = std::chrono::milliseconds(500);
    options.max_delay = std::chrono::seconds(30);
    options.max_attempts = 10;

    auto recovered = co_await libgs::websocket::retry_open(
        client,
        [](const libgs::websocket::retry_open_context &previous) {
            return refreshed_connect_request(previous);
        }, options, libgs::use_awaitable);

    stream = std::move(recovered.stream);
    co_await restore_authentication_and_subscriptions(stream);
    resume_business();
}
```

`retry_open()` only retries calls to the supplied client's `open()`; it does not
observe a stream, classify a business failure, run a session callback, or replay
messages. A fixed `connect_request` or a synchronous/awaitable request factory
may be supplied. The factory receives the preceding failure, allowing it to
refresh a URL, authentication header, or token before each attempt.

`retry_open_options` controls exponential backoff, cap, jitter, total attempt
limit, decision callback, and observer. `max_attempts = 0` means unbounded. The
first attempt is immediate; backoff occurs only after a failed attempt. By
default, transient network failures and HTTP 408/429/5xx are retried, while
protocol, authentication, and TLS failures stop. A successful result contains
the fresh stream, the number of attempts, final opening diagnostics, and the
last failure if one occurred. Cancel the completion token's associated
cancellation slot to stop the active opening or backoff wait.

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
| `ping()`, `pong()` | Explicit control frame in application-managed mode |
| `close()` | RFC close handshake |
| `shutdown()` | Immediate transport shutdown |
| `wait_written()` | Completion/errors of accepted queued writes |
| `wait_closed()`, `on_closed()` | Terminal close information |

Important rules:

- Only one of `read()`, `consume()`, or `read_frame()` may be active.
- `message_chunk::body` is valid only during its `consume()` callback.
- Ping/Pong and close callbacks observe control traffic processed by an active
  read; they do not start their own transport read.
- `on_ping()` and `on_pong()` accept synchronous callbacks or callbacks returning
  an `awaitable`. Return values are unrestricted and ignored. Their mutable
  `control_payload&` argument owns the received payload. `text()` exposes a
  convenient string view, `bytes()` exposes mutable/const byte spans,
  `as_mutable_buffer()` and `as_const_buffer()` expose Asio buffer views, and
  `assign()` or string assignment replaces the contents. These views remain valid
  only while the payload is not resized and the callback has not returned. In
  automatic mode, the payload left by `on_ping()` is used for the automatic Pong.
- Coroutine control callbacks require asynchronous `read()`, `consume()`, or
  `read_frame()`. A synchronous read that encounters one fails with
  `std::errc::operation_not_supported`.
- Complete-message reads assemble continuation frames and validate message
  limits. Frame reads preserve data-frame boundaries.
- Writes are serialized and bounded by `max_queued_write_bytes` and
  `max_queued_write_operations`.
- Shared access from multiple threads must be serialized with a strand or lock.

`stream_config` sets frame/message limits, read buffer size, outgoing fragment
size, write-queue limits, automatic Ping/Pong, close timeout, and compression
policy. A positive `ping_interval` is both the automatic Ping period and the
matching-Pong deadline; every Ping carries a unique eight-byte payload. An active
read must process the matching Pong before the next period or one consecutive
timeout is recorded. `pong_timeout_retries = N` tolerates the first N consecutive
timeouts and sends another Ping; timeout N+1 fails the stream with
`asio::error::timed_out`. A matching Pong resets the counter, and the default of
zero therefore fails on the first timeout.

`ping_interval = 0` disables both automatic Ping and automatic Pong, leaving
keepalive to application calls to `ping()` and `pong()`. When `ping_interval > 0`,
explicit `ping()` or `pong()` fails with `std::errc::operation_not_permitted` so
manual control frames cannot invalidate the automatic keepalive state machine.

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
`permessage-deflate`, transparent cross-connection message replay, or
application routing.

## Examples

- [Client](../../examples/websocket/client.cpp)
- [Explicit connection recovery](../../examples/websocket/retry_open.cpp)
- [Server](../../examples/websocket/server.cpp)
- [Mixed HTTP client](../../examples/websocket/mixed_http_client.cpp)
- [Mixed HTTP server](../../examples/websocket/mixed_http_server.cpp)
- [Proxy client](../../examples/websocket/proxy_client.cpp)
- [Protocol codecs](../../examples/websocket/protocol.cpp)
- [WSS client](../../examples/websocket/wss_client.cpp)
- [WSS server](../../examples/websocket/wss_server.cpp)
