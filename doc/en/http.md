# HTTP

Language: English | [简体中文](../zh_CN/http.md)

Target: `gs.http`. Enable `LIBGS_BUILD_HTTP`, include `<libgs/http.h>`, and
link `gs.http`. The module implements HTTP/1.0 and HTTP/1.1 and depends on
`gs.coro`.

## Public API map

| Header area | Purpose |
| --- | --- |
| `<libgs/http/client.h>` | Client, request contexts, replies, cookies, pooling, connectors, proxies |
| `<libgs/http/server.h>` | HTTP/HTTPS server, routing, request/response, AOP, sessions |
| `<libgs/http/protocol/...>` | Values, headers, cookies, parsers, generators, ranges, forms |
| `<libgs/http/utils/...>` | TCP/TLS connections and I/O helpers |

`LIBGS_OPENSSL_SUPPORT` enables TLS/HTTPS. `LIBGS_HTTP_ZLIB_SUPPORT`
enables gzip.

## Client

A high-level request has three stages:

1. Call `request_get()`, `request_post()`, or another request method.
2. Call `wait_reply()` on the returned request context.
3. Read or save the reply body.

The default completion token is synchronous. Pass `libgs::use_awaitable` at
each stage for coroutine use, or pass a compatible callback. Consume the full
body before releasing a context when its pooled connection should be reused.

| Configuration | Controls |
| --- | --- |
| `request_arg` | Headers, cookies, chunk attributes, Basic/Bearer auth, proxy auth |
| `client::req_info` | URL, request arguments, proxy, redirects, decompression |
| `client_config` | Default proxy and `TCP_NODELAY` |
| `connection_pool_config` | Pool size and connection lifetime |
| `connector` | Direct, TLS, HTTP CONNECT, SOCKS5, or custom connection creation |

The global proxy policy reads lowercase and uppercase
`http_proxy`/`https_proxy`/`all_proxy` and honors `no_proxy`. A request
can inherit it, bypass it, or provide an explicit forward/tunnel proxy.

Upload/download helpers report body bytes; headers, chunk framing, and multipart
boundaries are not included.

## Server

`http::server` owns an acceptor and routes requests by method and path. Route
patterns support literals, `*`, `?`, and named `{arguments}`.

| Area | Main API |
| --- | --- |
| Routing | `on_request()`, `on_default()` |
| Errors | `on_server_error()`, `on_service_error()` |
| Middleware | `basic_aop`, `basic_ctrlr_aop` |
| Request | Method/version/target, headers, cookies, arguments, body and file reads |
| Response | Status, headers, cookies, fixed/chunked writes, redirect, files, ranges |
| Sessions | `service_context::session()`, `session_or()`, session manager |

`server_config::resource_root` resolves relative paths used by file APIs. It
is not a security sandbox; validate every path derived from a request.

For protocol upgrade, `service_context::hand_over_connection()` transfers the
connection and pending bytes out of HTTP handling. WebSocket services should
normally use [`websocket::upgrade()`](websocket.md).

## I/O constraints

- One connection may have one read and one write active at the same time; do
  not overlap two reads or two writes.
- Serialize request/reply phase changes and access to each client, server, or
  context object.
- With multiple event-loop threads, use a strand for a service and initiate its
  operations from that strand.
- Non-detached asynchronous writes borrow the buffer until completion. Detached
  response writes own a copy.
- Cancellation does not release borrowed state before completion.
- Applications configure certificates, verification, and TLS policy in
  `asio::ssl::context`.

HTTP/2 and HTTP/3 are outside this module. General rules are in
[Execution and I/O model](io-model.md).

## Examples

The [HTTP example index](../../examples/README.md#http) lists clients, servers,
files, cookies, sessions, AOP, proxies, protocol codecs, and HTTPS.
