# LibGS

LibGS is a modular C++20 library for asynchronous applications. It uses
standalone Asio for execution and provides runtime facilities, coroutine
synchronization, HTTP/1.x, WebSocket, and application utilities.

## Modules

| Module | CMake target | Enable switch | Default | Public dependency |
| --- | --- | --- | :---: | --- |
| Core | `gs.core` | always built | ON | — |
| Coroutines | `gs.coro` | `LIBGS_BUILD_CORO` | ON | `gs.core` |
| HTTP | `gs.http` | `LIBGS_BUILD_HTTP` | OFF | `gs.coro` |
| WebSocket | `gs.websocket` | `LIBGS_BUILD_WEBSOCKET` | OFF | `gs.http` |
| Utilities | `gs.utils` | `LIBGS_BUILD_UTILITIES` | OFF | `gs.coro` |

Each module has an umbrella header under `libgs/`. `<libgs.h>` includes the
modules enabled by the build configuration.

## Quick start

The default build produces Core and Coroutines:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Enable the optional modules explicitly when the application needs them:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON
cmake --build build --parallel
```

A minimal Core program runs work on the process-wide default context:

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

Link it with `gs.core`. See [Getting started](doc/en/getting-started.md) for a
complete first build and [Build and configuration](doc/en/build.md) for all
module, dependency, installation, and consumer options.

## Requirements

- CMake 3.15 or newer
- GCC 13+, Clang 17+, or MSVC 19.30+ (Visual Studio 2022+)
- A C++20 standard library

Standalone Asio, spdlog, and nlohmann/json are included in `3rd_party/`.
OpenSSL, zlib, and liburing are optional system dependencies.

## Documentation

- [English documentation](doc/en/README.md)
- [简体中文文档](doc/zh_CN/README.md)
- [Examples](examples/README.md)
- [Tests](test/README.md)

The module guides describe the supported API surface. Public declarations in
[`libgs/`](libgs) are authoritative when more detail is needed.

## License

LibGS is licensed under the [MIT License](LICENSE.txt). Bundled dependencies
retain their own licenses and are listed in
[3rd_party/SOURCE.txt](3rd_party/SOURCE.txt).
