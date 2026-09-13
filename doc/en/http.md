# HTTP Client and Server

Language: English | [Simplified Chinese](../zh_CN/http.md)

The HTTP module implements HTTP/1.0 and HTTP/1.1 protocol utilities, clients,
servers, connections, routing, and optional TLS and gzip support. Link it as
`gs.http`; it publicly depends on `gs.coro` and `gs.core`.

WebSocket framing is not implemented by this module. The server exposes a
generic HTTP Upgrade connection handover boundary used by the separate,
implemented [`gs.websocket`](websocket.md) module.

## Header guide

| Header | Purpose |
| --- | --- |
| `<libgs/http/client.h>` | High-level HTTP client |
| `<libgs/http/server.h>` | HTTP and optional HTTPS server aliases |
| `<libgs/http/client/connection_pool.h>` | Reusable pooled connections |
| `<libgs/http/protocol/types.h>` | Methods, statuses, redirects, and request-target forms |
| `<libgs/http/protocol/utils.h>` | Parsers, generators, cookies, forms, compression, and protocol helpers |
| `<libgs/http.h>` | Client and server umbrella |

## Optional features

Enable TLS and gzip when configuring LibGS:

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

`LIBGS_OPENSSL_SUPPORT` enables HTTPS client connections, TLS connection types,
and HTTPS server aliases. `LIBGS_HTTP_ZLIB_SUPPORT` enables automatic HTTP gzip
compression and decompression. Both options require the corresponding system
package during configuration.

## HTTP server

The following server listens on port 8080 and handles `GET /hello`:

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
            "/hello",
            [](libgs::http::server::context_t &context)
                -> libgs::awaitable<void>
            {
                constexpr std::string_view body = "Hello from LibGS!\n";
                co_await context.response().write(
                    asio::buffer(body), libgs::use_awaitable
                );
            }
        )
        .start();

    return libgs::exec();
}
```

The server owns neither a hidden process thread nor a separate mandatory event
loop. The acceptor and service work use compatible Asio executors, and
`libgs::exec()` runs the default context in this example.

### Routing

`on_request` accepts one or more HTTP methods and a path rule. Rules support:

- `*` for a multi-character wildcard;
- `?` for a single-character wildcard; and
- `{name}` for a named path argument.

For example, `/users/{user_id}` exposes `user_id` through
`context.request().path_arg("user_id")`. Request data also includes the method,
HTTP version, target, path, query parameters, headers, cookies, endpoints, and
body-reading operations.

Use `on_default` for requests that do not match a registered route. Server-wide
and per-service errors can be handled with `on_server_error` and
`on_service_error`.

Routes may still be added with `on_request` or removed with `unbound_request`
after the server starts. These updates are safe to perform concurrently with
request handling: each individual route update is published atomically, so
subsequent requests observe either the complete old route table or the complete
new one, while requests that already selected a handler continue using it.
Literal paths use a fast exact-match index; wildcard rules and rules ending in
path arguments keep the matching and precedence behavior described above.

### Responses

The response API can:

- set status, headers, and cookies;
- write fixed or chunked bodies;
- stream files and handle supported range conditions;
- redirect or send `100 Continue`;
- finish chunked output with optional trailing headers; and
- enable automatic compression when zlib support is available.

As with Asio I/O, non-detached asynchronous writes borrow the supplied body
buffer until completion. Keep that storage alive across the `co_await` or
callback. Detached response writes make an owned copy.

### Resource root

`server_config::resource_root` is the base directory for relative file paths
passed to both `response.send_file()` and `request.save_file()`:

```cpp
auto config = server.config();
config.resource_root = "/srv/my-service";
server.set_config(config);

co_await context.response().send_file(
    "public/index.html", libgs::use_awaitable
);
co_await context.request().save_file(
    "uploads/item.bin", libgs::use_awaitable
);
```

Absolute paths bypass the root. An empty root preserves the earlier behavior,
where relative paths use the executable directory. `resource_root` resolves
paths; it is not a static-file router or a security sandbox. Validate any path
derived from request data and reject traversal such as `..` before file I/O.

Path-based `send_file()` calls cache raw file data and negotiated gzip variants
for files up to 2 MiB. The in-process cache is bounded to 32 MiB and validates
entries with file size and modification time on every request. Larger files,
range responses, and caller-owned streams continue to use streaming I/O. This
trades bounded memory for avoiding repeated reads and compression on hot static
assets.

### Middleware and sessions

`basic_aop` supplies coroutine `before` and `after` hooks plus exception
handling. `basic_ctrlr_aop` adds a controller-style `service` operation. A route
can receive one or more AOP objects.

`context.session()` and `context.session_or()` integrate with the server session
manager. Sessions support typed extension through inheritance, arbitrary
attributes, expiration, lifecycle extension, timeout callbacks, and a
configurable cookie key.

### HTTP Upgrade handover

For a valid upgrade request, `context.hand_over_connection()` transfers the
underlying connection away from normal HTTP request processing. The caller then
owns protocol handling and any pending bytes returned by the request. This is a
low-level ownership boundary; applications normally use
`websocket::upgrade()` rather than implementing RFC 6455 framing themselves.

## Synchronous HTTP client

High-level client methods use synchronous behavior by default:

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;
    auto context = client.request_get("http://example.com/");

    auto status = context->wait_reply();
    auto body = context->reply()->read<std::string>();

    std::cout << static_cast<unsigned>(status) << '\n';
    std::cout << body;
}
```

The default token throws `std::system_error` on failure. Pass a
`std::error_code` object to request, reply, and body operations for non-throwing
synchronous handling.

## Coroutine HTTP client

Pass `libgs::use_awaitable` to use the same operations from a coroutine:

```cpp
#include <libgs/http/client.h>

#include <iostream>
#include <string>

int main()
{
    libgs::http::client client;

    libgs::dispatch([&client]() -> libgs::awaitable<void>
    {
        auto context = co_await client.request_get(
            "http://example.com/", libgs::use_awaitable
        );
        co_await context->wait_reply(libgs::use_awaitable);

        auto body = co_await context->reply()->read<std::string>(
            libgs::use_awaitable
        );
        std::cout << body;

        libgs::exit();
    });

    return libgs::exec();
}
```

Callback tokens are also supported. Their completion signatures include an
error code followed by the operation result.

## Request configuration

`libgs::http::request_arg` stores request headers, cookies, chunk attributes,
and authentication. It provides helpers for Basic, Bearer, and proxy Basic
authentication.

`libgs::http::client::req_info` combines the URL and request arguments with
per-request behavior:

```cpp
#include <libgs/http/client.h>

int main()
{
    libgs::http::request_arg arguments;
    arguments.set_header("Accept", "application/json");

    libgs::http::client client;
    libgs::http::client::req_info info(
        "http://example.com/api", arguments
    );
    info.follow_redirects(5).auto_decompress(true);

    auto context = client.request_get(std::move(info));
    return context ? 0 : 1;
}
```

`set_proxy` selects an HTTP proxy for the request. Redirect following is
disabled until `follow_redirects` is called. Automatic decompression is enabled
by default but only performs gzip decoding when zlib support was compiled in.

The high-level client maintains a cookie jar and a connection pool. Cookies
received through `Set-Cookie` are stored after the reply is consumed and are
selected automatically for later matching URLs. The jar is available through
`client::cookie_store()`. A custom pool or executor can be supplied when the
application needs different limits, timeouts, routing, or execution ownership.

## Bodies and files

Request contexts expose body writes for methods that accept a body. Reply
objects support reads into caller-provided buffers, typed buffer containers,
byte vectors, and files. The high-level client also provides upload and download
helpers with optional progress callbacks.

HTTP write counts cover caller-supplied body bytes only (source-file bytes for
file operations), excluding HTTP headers, chunk framing, and multipart
boundaries. The `error_code&` and asynchronous forms preserve a partial body
count alongside an error. Header-only operations and `chunk_end()` return zero.

Read or save the complete reply body before dropping the request context when a
connection should be reused. Cancellation is available on clients, contexts,
replies, connections, and pools.

## HTTPS

When OpenSSL support is enabled, an `https://` URL uses the same high-level
client interface. HTTPS servers use `libgs::https::server` with an
`asio::ssl::context` that is configured with the application's certificate,
private key, protocol options, and verification policy.

TLS policy is application-specific. Do not copy development certificate or
verification settings into production without reviewing them.

## Protocol scope

The HTTP module implements HTTP/1.0 and HTTP/1.1. HTTP/2 and HTTP/3 are not
implemented. RFC 6455 framing is provided by the separate
[`gs.websocket`](websocket.md) module over HTTP/1.1 Upgrade.

## Related examples

- [`examples/http/client_sync.cpp`](../../examples/http/client_sync.cpp)
- [`examples/http/client_awaitable.cpp`](../../examples/http/client_awaitable.cpp)
- [`examples/http/client_cookies.cpp`](../../examples/http/client_cookies.cpp)
- [`examples/http/client_file.cpp`](../../examples/http/client_file.cpp)
- [`examples/http/protocol.cpp`](../../examples/http/protocol.cpp)
- [`examples/http/server.cpp`](../../examples/http/server.cpp)
- [`examples/http/server_aop.cpp`](../../examples/http/server_aop.cpp)
- [`examples/http/server_file.cpp`](../../examples/http/server_file.cpp)
- [`examples/http/server_session.cpp`](../../examples/http/server_session.cpp)
- [`examples/http/https_server.cpp`](../../examples/http/https_server.cpp)
