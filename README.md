# LibGS

LibGS is a C++20 library for asynchronous applications. It is built on
standalone Asio and split into independent modules for runtime facilities,
coroutines, HTTP, WebSocket, and application utilities.

## Modules

| Target | Depends on | Main header | Purpose |
| --- | --- | --- | --- |
| `gs.core` | — | `<libgs/core.h>` | Execution, data types, algorithms, containers, and system helpers |
| `gs.coro` | `gs.core` | `<libgs/coro.h>` | Coroutine waits and synchronization |
| `gs.http` | `gs.coro` | `<libgs/http.h>` | HTTP/1.0 and HTTP/1.1 clients, servers, and protocol utilities |
| `gs.websocket` | `gs.http` | `<libgs/websocket.h>` | RFC 6455 clients, servers, streams, and codecs |
| `gs.utils` | `gs.coro` | `<libgs/utils.h>` | Logging, settings, modules, processes, signals, observers, and soft bus |

`<libgs.h>` includes every module enabled by the build configuration.

## Requirements

- CMake 3.15 or newer
- C++20
- GCC 13+, Clang 17+, or MSVC 19.30+ (Visual Studio 2022+)

Asio, spdlog, and nlohmann/json are included in `3rd_party/`. OpenSSL, zlib,
and liburing are optional system dependencies.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build
```

The default configuration builds all five modules and normally uses shared
libraries. A Windows GNU toolchain without shared `libstdc++` defaults to a
static build. Common switches are:

```sh
-DLIBGS_BUILD_STATIC=ON
-DLIBGS_BUILD_EXAMPLES=ON
-DLIBGS_OPENSSL_SUPPORT=ON
-DLIBGS_HTTP_ZLIB_SUPPORT=ON
```

Module switches and dependency rules are documented in
[Getting started](doc/en/getting-started.md).

## Minimal program

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post(1s, []
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

Link this program with `gs.core`. `libgs::exec()` runs the default event loop;
applications that own an Asio executor can pass it to executor-aware APIs.

## Documentation

- [English documentation](doc/en/README.md)
- [简体中文文档](doc/zh_CN/README.md)
- [Examples](examples/README.md)
- [Tests](test/README.md)

## License

LibGS is licensed under the [MIT License](LICENSE.txt). Bundled dependencies
retain their own licenses and are listed in
[3rd_party/SOURCE.txt](3rd_party/SOURCE.txt).
