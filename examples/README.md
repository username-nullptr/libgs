# LibGS Examples

The examples are small, buildable programs grouped by public module. Use this
page for build and naming conventions, then open the module guide for program
arguments and runnable combinations.

## Module guides

| Module | Target dependency | Guide | Scope |
| --- | --- | --- | --- |
| Core | `gs.core` | [Core examples](core/README.md) | Runtime, values, files, algorithms, queues, application paths, arguments, and dynamic libraries |
| Coroutines | `gs.coro` | [Coroutine examples](coro/README.md) | Awaitable execution and synchronization |
| HTTP | `gs.http` | [HTTP examples](http/README.md) | Protocol, clients, servers, middleware, sessions, files, proxies, and HTTPS |
| WebSocket | `gs.websocket` | [WebSocket examples](websocket/README.md) | Protocol, WS/WSS pairs, HTTP Upgrade, recovery, and proxies |
| Utilities | `gs.utils` | [Utilities examples](utils/README.md) | Logging, settings, signals, observers, modules, processes, and soft bus |

## Build

`LIBGS_BUILD_EXAMPLES=ON` adds examples only for enabled modules. The default
module configuration builds Core and Coroutine examples:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Enable every module for the complete non-TLS example set:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

HTTPS and WSS examples additionally require
`LIBGS_OPENSSL_SUPPORT=ON` and OpenSSL. Compression changes the enabled
protocol paths but does not add separate example executables.

See [Build and configuration](../doc/en/build.md) for every library and
toolchain switch.

## Targets and output

An example named `<module>/<name>` has:

| Item | Pattern |
| --- | --- |
| CMake target | `libgs.example.<module>.<name>` |
| Single-config executable | `build/output/examples/<module>/<name>` |
| Install location | `examples/<module>/<name>` |

Multi-config generators may add a configuration directory such as `Release/`.
The Core dynamic-library example also builds a companion plugin beside its
executable.

Build one example target:

```sh
cmake --build build --target libgs.example.core.execution
```

## Running examples

- Offline examples run independently and normally finish immediately.
- Local network examples bind loopback addresses or connect to loopback by
  default; start the matching server first.
- Server and recovery examples keep running until interrupted.
- File-producing examples accept an output path so the caller controls where
  data is written.
- Proxy examples require a separately running HTTP or SOCKS5 proxy.
- Programs with required arguments print a usage line and exit with status 2
  when arguments are missing.

Start with:

```sh
./build/output/examples/core/execution
./build/output/examples/coro/basics
```

For client/server pairs and exact arguments, use the module guides above.

## Adding an example

1. Keep one public concept or one small integration path per executable.
2. Use loopback endpoints and local files by default; do not require a public
   service.
3. Accept environment-specific paths, ports, endpoints, and credentials as
   arguments.
4. Register the target in [`examples/CMakeLists.txt`](CMakeLists.txt).
5. Add the program to its module guide, including side effects and its matching
   server or client.
