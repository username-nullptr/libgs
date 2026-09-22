# LibGS

LibGS is a modular C++20 library built on standalone Asio. It provides a shared
execution runtime, coroutine synchronization, HTTP/1.x, WebSocket, and
application utilities.

## Modules

| Module | Target | Default | Depends on |
| --- | --- | :---: | --- |
| Core | `gs.core` | ON | — |
| Coroutines | `gs.coro` | ON | Core |
| HTTP | `gs.http` | OFF | Coroutines |
| WebSocket | `gs.websocket` | OFF | HTTP |
| Utilities | `gs.utils` | OFF | Coroutines |

Each module has an umbrella header under `libgs/`. `<libgs.h>` includes every
module enabled in the build configuration.

## Build

Requirements: CMake 3.15+, C++20, and GCC 13+, Clang 17+, or MSVC 19.30+
(Visual Studio 2022+).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The default configuration builds Core and Coroutines. Enable optional modules
explicitly:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

For module switches, optional dependencies, installation, and consumption, see
[Build and configuration](doc/en/build.md).

## Repository layout

| Path | Contents |
| --- | --- |
| [`libgs/`](libgs) | Public headers and module implementations |
| [`examples/`](examples/README.md) | Buildable examples grouped by module |
| [`test/`](test/README.md) | Functional, stress, fuzz, and performance tests |
| [`doc/en/`](doc/en/README.md) | English guides |
| [`doc/zh_CN/`](doc/zh_CN/README.md) | 简体中文指南 |
| [`3rd_party/`](3rd_party/SOURCE.txt) | Bundled dependencies and their sources |

Public declarations under `libgs/` are the API reference. The guides explain
module selection, usage patterns, ownership, and concurrency constraints.

## License

LibGS is licensed under the [MIT License](LICENSE.txt). Bundled dependencies
retain their own licenses.
