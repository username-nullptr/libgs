# HTTP

Language: English | [简体中文](../zh_CN/http.md)

`gs.http` implements HTTP/1.0 and HTTP/1.1. Link it through `gs.http`; its
public dependency is `gs.coro`.

## Public structure

| Header area | Purpose |
| --- | --- |
| `<libgs/http/client.h>` | High-level client, request contexts, replies, cookies, pooling, connectors, and proxies |
| `<libgs/http/server.h>` | HTTP/HTTPS servers, routing, request/response, AOP, and sessions |
| `<libgs/http/protocol/...>` | HTTP values, headers, cookies, parsers, generators, ranges, forms, and compression |
| `<libgs/http/utils/...>` | TCP/TLS connections and I/O completion helpers |
| `<libgs/http.h>` | Client and server umbrella |

`LIBGS_OPENSSL_SUPPORT` enables TLS types, HTTPS clients, and HTTPS server
aliases. `LIBGS_HTTP_ZLIB_SUPPORT` enables HTTP gzip support.

## Client

A request has three stages: create/send the request, wait for the reply, then
consume the reply body.

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;
    std::error_code error;

    auto context = client.request_get("http://127.0.0.1:8080/", error);
    if (error)
        return 1;

    auto status = context->wait_reply(error);
    auto body = context->reply()->read<std::string>(error);
    if (error)
        return 1;

    std::cout << static_cast<unsigned>(status) << '\n' << body;
}
```

The default token is synchronous. Pass `libgs::use_awaitable` at each stage
for coroutine use, or pass a compatible callback. The high-level client keeps
a connection pool and cookie jar; consume the full body before releasing a
context when its connection should be reused.

### Request configuration

| Type | Controls |
| --- | --- |
| `request_arg` | Headers, cookies, chunk attributes, Basic/Bearer auth, and proxy Basic auth |
| `client::req_info` | URL, request arguments, per-request proxy, redirects, and decompression |
| `client_config` | Default proxy and `TCP_NODELAY` policy |
| `connection_pool_config` | Pool size and connection lifetime policy |
| `connector` | Direct, TLS, HTTP CONNECT, SOCKS5, or application-defined connection creation |

Redirects are disabled when `max_redirects` is zero. Automatic decompression is
enabled in `req_info`, but gzip decoding requires zlib support.

`client_config::default_proxy` defaults to `use_global_proxy`. The built-in
resolver reads `http_proxy`, `https_proxy`, and `all_proxy` (including uppercase
variants) and applies `no_proxy`. Set `no_proxy` to force direct access. A
request can inherit the client setting, bypass it, use an HTTP forward proxy,
or specify an HTTP CONNECT/SOCKS5 `proxy_tunnel`.

The client also provides upload/download helpers and progress callbacks. Body
byte counts exclude headers, chunk framing, and multipart boundaries.

## Server

```cpp
#include <libgs/http/server.h>

#include <string_view>

int main()
{
    asio::ip::tcp::acceptor acceptor(libgs::get_executor());
    libgs::http::server server(std::move(acceptor));

    server
        .bind({libgs::ip_type::v4, 8080})
        .on_request<libgs::http::method::get>(
            "/hello/{name}",
            [](libgs::http::server::context_t &context)
                -> libgs::awaitable<void>
            {
                auto name = context.request().path_arg("name");
                const auto body = name ? name->to_string() : "unknown";
                co_await context.response().write(
                    asio::buffer(body), libgs::use_awaitable
                );
            }
        )
        .start();

    return libgs::exec();
}
```

Route rules support literal paths, `*`, `?`, and named `{arguments}`. Use
`on_default`, `on_server_error`, and `on_service_error` for unmatched requests
and failures. Routes can attach `basic_aop` before/after/exception hooks or a
controller-style `basic_ctrlr_aop`.

The request API exposes method, version, target, path/query/path arguments,
headers, cookies, endpoints, and body/file reads. The response API provides
status, headers, cookies, fixed or chunked writes, redirects, file responses,
ranges, and optional compression.

`server_config::resource_root` is the base for relative paths passed to
`request.save_file()` and `response.send_file()`. It is path resolution, not a
security sandbox; validate paths derived from request data.

Sessions are available through `service_context::session()` and
`session_or()`. The session manager controls the cookie key, expiration, and
timeout handling.

## HTTP Upgrade

`service_context::hand_over_connection()` transfers an Upgrade connection and
pending bytes out of the HTTP request lifecycle. WebSocket applications should
normally call [`websocket::upgrade()`](websocket.md) instead of using this
low-level boundary directly.

## I/O rules

- HTTP I/O follows the [execution and I/O model](io-model.md): distinct
  objects are safe, while access to one stateful object must be serialized.
- One connection may have one read and one write outstanding at the same time;
  do not overlap two reads or two writes. Request and reply phase transitions
  must also be serialized.
- With multiple `io_context::run()` threads, use a strand as the client or
  server service executor and initiate operations from that strand.
- Non-detached asynchronous writes borrow the supplied buffer until completion;
  detached response writes own a copy.
- Error-code and asynchronous write forms preserve a partial body-byte count
  when an error occurs.
- Cancellation is exposed by clients, request contexts, replies, connections,
  pools, requests, and responses where applicable.
- Configure TLS certificates, verification, and protocol policy in the
  application's `asio::ssl::context`.

HTTP/2 and HTTP/3 are not implemented. WebSocket framing belongs to the
separate `gs.websocket` module.

## Examples

- [Synchronous client](../../examples/http/client_sync.cpp)
- [Coroutine client](../../examples/http/client_awaitable.cpp)
- [Cookies](../../examples/http/client_cookies.cpp)
- [Files](../../examples/http/client_file.cpp)
- [Proxy client](../../examples/http/proxy_client.cpp)
- [Protocol codecs](../../examples/http/protocol.cpp)
- [Server](../../examples/http/server.cpp)
- [AOP](../../examples/http/server_aop.cpp)
- [File server](../../examples/http/server_file.cpp)
- [Sessions](../../examples/http/server_session.cpp)
- [HTTPS server](../../examples/http/https_server.cpp)
