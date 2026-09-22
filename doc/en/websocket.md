# WebSocket

Language: English | [简体中文](../zh_CN/websocket.md)

Target: `gs.websocket`. Enable `LIBGS_BUILD_WEBSOCKET`, include
`<libgs/websocket.h>`, and link `gs.websocket`. The module implements
RFC 6455 over HTTP/1.1 and depends on `gs.http`.

## Public API map

| Header | Purpose |
| --- | --- |
| `<libgs/websocket/client.h>` | WS/WSS open, redirects, cookies, proxies, diagnostics |
| `<libgs/websocket/server.h>` | Owned server and HTTP Upgrade |
| `<libgs/websocket/stream.h>` | Message/frame I/O, control frames, close, lifecycle |
| `<libgs/websocket/retry.h>` | Explicit retry after an opening failure |
| `<libgs/websocket/protocol/...>` | Opening handshake and frame codecs |
| `<libgs/websocket/types.h>` | Stream, message, close, and compression configuration |

`LIBGS_OPENSSL_SUPPORT` enables WSS. WebSocket zlib support enables RFC 7692
`permessage-deflate`.

## Client and recovery

`websocket::client` owns an HTTP client. `client.open()` opens one
connection; the free `websocket::open(http_client, ...)` reuses an
application-owned HTTP client and its connector, cookies, pool, and proxy
policy.

`connect_request` configures headers/authentication, proxy, stream limits,
handshake timeout, redirects, subprotocols, and extensions. The default proxy
policy checks `ws_proxy`/`wss_proxy`, then HTTP/HTTPS proxy variables, then
`all_proxy`, while honoring `no_proxy`.

Opening is attempted once. After application code decides that an active stream
is unusable, `retry_open()` can retry new opening attempts with exponential
backoff, cap, jitter, attempt limit, decision callback, and observer.
`retry_open()` does not monitor streams, replay messages, restore
authentication/subscriptions, or run the business receive loop.

## Server and HTTP Upgrade

`websocket::server` owns an HTTP listener and opening handshakes. Deliver
connections either through `accept()` or registered
`on_connection()`/`on_default()` handlers; do not mix both models on one
server.

For a mixed service, route with `http::server`, check
`is_upgrade_request()`, and call `websocket::upgrade()`. A successful
upgrade returns the stream, an immutable request snapshot, and selected
subprotocol/extensions.

`upgrade_options` configures stream limits, handshake timeout, response
headers, protocol/extension allowlists, and synchronous or awaitable validators
and selectors. Synchronous `upgrade()` cannot execute awaitable callbacks.

## Stream

| Operation | Result |
| --- | --- |
| `read<Buffer>()` | One complete text or binary message |
| `consume()` | One message delivered in temporary chunks |
| `read_frame<Buffer>()` | One data frame |
| `write_text()`, `write_binary()`, `write()` | One complete message |
| `write_frame()` | One explicitly fragmented data frame |
| `ping()`, `pong()` | Manual control frames |
| `close()` | RFC close handshake |
| `shutdown()` | Immediate transport shutdown |
| `wait_written()` | Observe accepted queued writes |
| `wait_closed()`, `on_closed()` | Observe terminal close information |

Rules for one stream:

- Only one of `read()`, `consume()`, or `read_frame()` may be active.
- One read and one write may overlap. Message writes are serialized through
  byte- and operation-bounded queues.
- `message_chunk::body` is valid only during its `consume()` callback.
- Control callbacks run while an active read processes control frames.
- Complete-message reads assemble continuation frames and enforce message
  limits; frame reads preserve frame boundaries.
- Serialize all access on one strand when the event loop has multiple threads.

`stream_config` controls frame/message limits, buffering, fragmentation,
queued writes, close timeout, keepalive, and compression. With a positive
`ping_interval`, automatic Ping/Pong is enabled and an active read must process
Pong frames. Manual `ping()`/`pong()` is available only when the interval is
zero.

## Compression and boundaries

Both peers must negotiate `permessage-deflate`. Configure its window/context
policy with `permessage_deflate_options`; configure automatic message
compression and thresholds with `compression_config`.

The module supports masking, fragmentation, UTF-8 validation, Ping/Pong/Close,
WS/WSS, redirects, cookies, subprotocols, HTTP and SOCKS5 proxies, timeouts,
cancellation, and bounded writes. It does not provide WebSocket over HTTP/2 or
HTTP/3, extensions other than `permessage-deflate`, persistent replay, or
application routing.

General ownership and concurrency rules are in
[Execution and I/O model](io-model.md).

## Examples

The [WebSocket example index](../../examples/README.md#websocket) lists owned
clients/servers, mixed HTTP Upgrade, retry, proxies, codecs, and WSS.
