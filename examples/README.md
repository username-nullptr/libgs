# LibGS examples

The examples are grouped by public module. Each source file focuses on one API
or one small integration path and is built as an independent executable.

| Directory | What it demonstrates |
| --- | --- |
| [`core`](core) | Execution, values, INI files, algorithms, queues, application paths, command-line parsing, and dynamic libraries |
| [`coro`](coro) | Awaitable basics and coroutine synchronization primitives |
| [`http`](http) | Offline HTTP parsing, clients, servers, middleware, sessions, and optional HTTPS |
| [`websocket`](websocket) | Standalone WS/WSS clients and servers, offline protocol flow, and a live HTTP/WebSocket mixed application |
| [`utils`](utils) | Logging, settings, signals, observers, modules, processes, and the extensible soft bus |

## Build

From the repository root:

```bash
cmake -S . -B build -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build -j
```

Executables are written below `build/output/examples/<module>/`. HTTPS and WSS
examples are built only when `LIBGS_OPENSSL_SUPPORT=ON` and OpenSSL is available.

## Run

Most examples are self-contained and complete in well under one second.
Examples that create a file accept an optional output path, so they need no
machine-specific directory. Network clients use a local server by default and
never require a public endpoint.

Start `server` before the basic and Cookie clients:

```bash
./build/output/examples/http/server
./build/output/examples/http/client_sync
./build/output/examples/http/client_cookies
```

Run the file server and client with any local source file:

```bash
./build/output/examples/http/server_file README.md
./build/output/examples/http/client_file README.md
```

`client_sync` and `client_awaitable` accept a URL. `client_cookies` accepts a
base URL, and `client_file` accepts source, destination, and base URL arguments.
Servers accept a port. `https_server` additionally requires certificate and
private-key paths; run an executable without arguments to see required inputs.

The `dynamic_library` executable loads the companion plugin from its own
directory. You may instead pass another plugin path as the first argument.

The mixed WebSocket examples keep `http::server` and `http::client` as their
root objects. The same `/mixed` route first handles an ordinary HTTP request,
then hands an Upgrade request to `websocket::upgrade`. The client upgrades
the same `http://` URL through `websocket::open` (which also accepts `ws://`)
and completes a text-frame echo.

```bash
# Terminal 1
./build/output/examples/websocket/mixed_http_server

# Terminal 2
./build/output/examples/websocket/mixed_http_client
```

For a dedicated WebSocket service, the higher-level `websocket::server` owns
the listener and performs each opening handshake, while `websocket::client`
owns the HTTP connector used by `open()`:

```bash
# Terminal 1
./build/output/examples/websocket/server

# Terminal 2
./build/output/examples/websocket/client
```

The secure pair uses the same WebSocket API with an injected client TLS context
and a TLS server. The server takes a certificate and private key; the optional
second client argument adds a private CA or self-signed server certificate to
the trust store:

```bash
# Terminal 1
./build/output/examples/websocket/wss_server server.pem server-key.pem

# Terminal 2
./build/output/examples/websocket/wss_client \
  wss://127.0.0.1:8443/echo server.pem
```

## Coverage

| Capability | Examples |
| --- | --- |
| Core runtime and data | `core/execution`, `value`, `ini`, `algorithms`, `lock_free_queue` |
| Application integration | `core/app_paths`, `args_parser`, `dynamic_library` |
| Coroutine synchronization | `coro/basics`, `mutex`, `shared_mutex`, `semaphore`, `condition_variable` |
| HTTP clients and protocol | `http/client_sync`, `client_awaitable`, `client_cookies`, `client_file`, `protocol` |
| HTTP servers | `http/server`, `server_aop`, `server_file` (uploads, downloads, and `resource_root`), `server_session`, and optional `https_server` |
| WebSocket protocol | `websocket/client` and `server` use the owned high-level API; optional `wss_client` and `wss_server` add TLS; `protocol` performs an offline handshake/frame round trip; the `mixed_http_*` pair shares one HTTP route |
| Utilities | `utils/logger`, `settings`, `signal_slot`, `observer`, `modules`, `process` |
| Soft bus | `utils/soft_bus_local` uses the built-in in-process transport; `soft_bus_transport` shows the interface used to plug in DDS, IPC, or another transport |
