# LibGS Examples

Examples are small executables grouped by the module they link. This file is
the single index for build commands, arguments, and programs.

## Build

`LIBGS_BUILD_EXAMPLES=ON` builds examples only for enabled modules:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Example target names are `libgs.example.<module>.<name>`. Single-config
executables are written to `build/output/examples/<module>/`; multi-config
generators may add a configuration directory.

Build one program:

```sh
cmake --build build --target libgs.example.core.execution
```

## Core

| Program | Purpose | Arguments / side effects |
| --- | --- | --- |
| [`algorithms`](core/algorithms.cpp) | MIME, SHA-1, UUID, wildcard matching | None |
| [`app_paths`](core/app_paths.cpp) | Executable, working, home, absolute paths | None |
| [`args_parser`](core/args_parser.cpp) | Options, flags, help/version, positional values | `-o/--output`, `-v/--verbose`, `--version`, `-h/--help` |
| [`dynamic_library`](core/dynamic_library.cpp) | Load a shared library and resolve a symbol | Optional plugin path; defaults to the companion plugin |
| [`execution`](core/execution.cpp) | Dispatch, post, timers, cancellation, event loop | None; runs about one second |
| [`ini`](core/ini.cpp) | Load, update, and save INI | Optional path; writes `libgs-example.ini` by default |
| [`lock_free_queue`](core/lock_free_queue.cpp) | Concurrent producer/consumer queue | None |
| [`value`](core/value.cpp) | Text/numeric conversion and formatting | None |

`dynamic_library_plugin` is a fixture built beside `dynamic_library`, not a
standalone example.

## Coroutines

| Program | Purpose |
| --- | --- |
| [`basics`](coro/basics.cpp) | Awaitable start, future wait, delay, worker switch |
| [`mutex`](coro/mutex.cpp) | Coroutine mutex and unique lock |
| [`shared_mutex`](coro/shared_mutex.cpp) | Shared readers and exclusive writer |
| [`semaphore`](coro/semaphore.cpp) | Limit concurrent coroutine work |
| [`condition_variable`](coro/condition_variable.cpp) | Predicate wait and notification |

These programs are self-contained and take no arguments.

## HTTP

Requires `LIBGS_BUILD_HTTP=ON`.

| Program | Purpose | Arguments / default |
| --- | --- | --- |
| [`protocol`](http/protocol.cpp) | Offline parser/generator | None |
| [`client_sync`](http/client_sync.cpp) | Synchronous request/reply/body | `[url]`; local port 8080 |
| [`client_awaitable`](http/client_awaitable.cpp) | Coroutine request/reply/body | `[url]`; local port 8080 |
| [`client_cookies`](http/client_cookies.cpp) | Cookie storage and resend | `[base-url]`; local port 8080 |
| [`client_file`](http/client_file.cpp) | Upload and download | `<upload-file> [download-file] [base-url]` |
| [`proxy_client`](http/proxy_client.cpp) | HTTP proxy and optional Basic auth | `[target-url] [proxy-url] [user] [password]` |
| [`server`](http/server.cpp) | Routes, path arguments, cookies, errors | `[port]`; 8080 |
| [`server_aop`](http/server_aop.cpp) | Middleware and controller handler | `[port]`; 8081 |
| [`server_file`](http/server_file.cpp) | File response and upload save | `<download-file> [port] [upload-file]`; 8083 |
| [`server_session`](http/server_session.cpp) | Sessions and session cookies | `[port]`; 8082 |
| [`https_server`](http/https_server.cpp) | HTTPS server | `<certificate.pem> <private-key.pem> [port]`; 8443 |

`https_server` is built only with `LIBGS_OPENSSL_SUPPORT=ON`.

Basic pair:

```sh
# Start first
./build/output/examples/http/server

# Run in another terminal
./build/output/examples/http/client_sync
```

File pair:

```sh
./build/output/examples/http/server_file README.md 8083 /tmp/libgs-uploaded.bin
./build/output/examples/http/client_file \
  README.md /tmp/libgs-downloaded.md http://127.0.0.1:8083
```

Proxy examples require an external proxy. File paths derived from request data
must be validated by applications; `resource_root` is not a sandbox.

## WebSocket

Requires `LIBGS_BUILD_WEBSOCKET=ON`.

| Program | Purpose | Arguments / default |
| --- | --- | --- |
| [`protocol`](websocket/protocol.cpp) | Offline handshake and frame codecs | None |
| [`server`](websocket/server.cpp) | Owned echo server | `[port]`; 8080 at `/echo` |
| [`client`](websocket/client.cpp) | Open, message I/O, close | `[endpoint]`; local echo server |
| [`retry_open`](websocket/retry_open.cpp) | Application-controlled recovery | `[endpoint]`; local echo server |
| [`proxy_client`](websocket/proxy_client.cpp) | HTTP/SOCKS5 proxy | `[endpoint] [proxy-url] [user] [password]` |
| [`mixed_http_server`](websocket/mixed_http_server.cpp) | HTTP route plus Upgrade | `[port]`; 8080 at `/mixed` |
| [`mixed_http_client`](websocket/mixed_http_client.cpp) | HTTP request then Upgrade | `[http-url] [websocket-url]` |
| [`wss_server`](websocket/wss_server.cpp) | TLS echo server | `<certificate.pem> <private-key.pem> [port]`; 8443 |
| [`wss_client`](websocket/wss_client.cpp) | TLS client and trust setup | `[endpoint] [ca-certificate.pem]` |

WSS programs are built only with `LIBGS_OPENSSL_SUPPORT=ON`. Run either pair
with the server first:

```sh
./build/output/examples/websocket/server
./build/output/examples/websocket/client

./build/output/examples/websocket/mixed_http_server
./build/output/examples/websocket/mixed_http_client
```

`retry_open` is long-running. Proxy examples require an external proxy. The
basic and mixed servers both default to port 8080, so do not run them together
without changing a port.

## Utilities

Requires `LIBGS_BUILD_UTILITIES=ON`.

| Program | Purpose | Arguments / side effects |
| --- | --- | --- |
| [`logger`](utils/logger.cpp) | Default and named loggers | Optional directory; writes `./logs` by default |
| [`settings`](utils/settings.cpp) | Settings, signals, persistence | Optional INI path; writes `libgs-example-settings.ini` by default |
| [`signal_slot`](utils/signal_slot.cpp) | Function and lambda slots | None |
| [`observer`](utils/observer.cpp) | ID-addressed callback lifecycle | None |
| [`modules`](utils/modules) | Dependency graph and ordered initialization | None |
| [`process`](utils/process.cpp) | Child start, stdout, join, exit code | Runs the platform echo command |
| [`soft_bus_local`](utils/soft_bus_local.cpp) | In-process publish/subscribe/cache | None |
| [`soft_bus_udp`](utils/soft_bus_udp.cpp) | UDP multicast transport | Requires UDP transport and local multicast |
| [`soft_bus_transport`](utils/soft_bus_transport.cpp) | Custom transport boundary | None |

Pass explicit paths to keep generated files outside the source tree:

```sh
./build/output/examples/utils/logger /tmp/libgs-example-logs
./build/output/examples/utils/settings /tmp/libgs-example-settings.ini
```
