# LibGS examples

The examples are grouped by public module. Each source file focuses on one API
or one small integration path and is built as an independent executable.

| Directory | What it demonstrates |
| --- | --- |
| [`core`](core) | Execution, values, INI files, algorithms, queues, application paths, command-line parsing, and dynamic libraries |
| [`coro`](coro) | Awaitable basics and coroutine synchronization primitives |
| [`http`](http) | Offline HTTP parsing, clients, servers, middleware, sessions, and optional HTTPS |
| [`utils`](utils) | Logging, settings, signals, observers, modules, processes, and the extensible soft bus |

## Build

From the repository root:

```bash
cmake -S . -B build -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build -j
```

Executables are written below `build/output/examples/<module>/`. HTTPS is built
only when `LIBGS_OPENSSL_SUPPORT=ON` and OpenSSL is available.

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

## Coverage

| Capability | Examples |
| --- | --- |
| Core runtime and data | `core/execution`, `value`, `ini`, `algorithms`, `lock_free_queue` |
| Application integration | `core/app_paths`, `args_parser`, `dynamic_library` |
| Coroutine synchronization | `coro/basics`, `mutex`, `shared_mutex`, `semaphore`, `condition_variable` |
| HTTP clients and protocol | `http/client_sync`, `client_awaitable`, `client_cookies`, `client_file`, `protocol` |
| HTTP servers | `http/server`, `server_aop`, `server_file` (uploads, downloads, and `resource_root`), `server_session`, and optional `https_server` |
| Utilities | `utils/logger`, `settings`, `signal_slot`, `observer`, `modules`, `process` |
| Soft bus | `utils/soft_bus_local` uses the built-in in-process transport; `soft_bus_transport` shows the interface used to plug in DDS, IPC, or another transport |
