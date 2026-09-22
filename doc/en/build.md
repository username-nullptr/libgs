# Build and Configuration

Language: English | [简体中文](../zh_CN/build.md)

This page is the single reference for build requirements, CMake switches,
outputs, installation, and supported integration boundaries.

## Requirements

| Component | Requirement |
| --- | --- |
| CMake | 3.15 or newer |
| Language | C++20 |
| GCC | 13 or newer |
| Clang | 17 or newer |
| MSVC | 19.30 or newer (Visual Studio 2022+) |

Standalone Asio, spdlog, and nlohmann/json are bundled. OpenSSL, zlib, and
liburing are optional system dependencies used only when their features are
enabled.

## Module selection

| Switch | Default | Target | Dependency |
| --- | :---: | --- | --- |
| Core (always built) | ON | `gs.core` | — |
| `LIBGS_BUILD_CORO` | ON | `gs.coro` | Core |
| `LIBGS_BUILD_HTTP` | OFF | `gs.http` | Coroutines |
| `LIBGS_BUILD_WEBSOCKET` | OFF | `gs.websocket` | HTTP |
| `LIBGS_BUILD_UTILITIES` | OFF | `gs.utils` | Coroutines |

CMake rejects an enabled module whose dependency is disabled. Targets expose
their dependencies publicly, so consumers link only the highest module they use.

Default build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

All modules and examples:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

## Feature switches

| Switch | Default | Effect |
| --- | :---: | --- |
| `LIBGS_BUILD_STATIC` | OFF* | Build static instead of shared libraries |
| `LIBGS_ADD_LIBRARY_VERSION` | ON | Add version metadata to shared libraries |
| `LIBGS_OPENSSL_SUPPORT` | OFF | Enable TLS, HTTPS, and WSS; requires OpenSSL |
| `LIBGS_HTTP_ZLIB_SUPPORT` | OFF | Enable HTTP gzip; requires zlib |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | OFF | Enable WebSocket `permessage-deflate`; requires zlib |
| `LIBGS_BUILD_UTILITIES_SBUS_UDP` | ON | Build the UDP soft-bus transport |
| `LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` | `local` | Select `local` or `udp` for unqualified soft-bus APIs |
| `LIBGS_IO_URING_SUPPORT` | OFF | Use Asio io_uring on Linux; requires liburing |
| `LIBGS_BUILD_EXAMPLES` | OFF | Build examples for enabled modules |

HTTP zlib automatically enables WebSocket zlib when WebSocket is built. The
standalone WebSocket switch is offered only when HTTP zlib is off.

*On Windows with a GNU toolchain, the default changes to static if a shared
`libstdc++-6.dll` cannot be found.*

Toolchain controls:

| Switch | Applies to | Effect |
| --- | --- | --- |
| `LIBGS_USE_LIBCXX` | Clang | Compile and link with libc++ |
| `LIBGS_USE_LLD` | Clang | Link with lld |
| `LIBGS_ENABLE_LTO` | GCC | Enable LTO |
| `LIBGS_HEAVY_COMPILE_JOBS` | Tests/examples | Limit concurrent HTTP/WebSocket compilations; `0` disables the limit |
| `LIBGS_LOW_MEMORY_DEBUG_INFO` | GCC Debug builds | Use `-g1` to reduce compiler memory |

Test-only switches are documented in [Tests](../../test/README.md).

## Output and installation

Single-config builds write:

| Output | Path |
| --- | --- |
| Shared libraries and executables | `build/output/bin` |
| Static and import libraries | `build/output/lib` |
| Examples | `build/output/examples/<module>` |
| Generated configuration headers | `build/output/config_include` |

Multi-config generators may add a configuration directory.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

Visual Studio and other multi-config generators require `--config Release` on
build and install commands.

## Consume the source tree

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

set(LIBGS_BUILD_HTTP ON CACHE BOOL "")
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.http)
```

Set LibGS options before `add_subdirectory()`. Bundled headers and transitive
module/system dependencies are supplied by the linked target.

## Consume an install tree

Add the install prefix to `CMAKE_PREFIX_PATH`, then load the installed package
and link the highest-level module the application uses:

```cmake
find_package(LibGS 0.16 CONFIG REQUIRED COMPONENTS http)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE LibGS::http)
```

For example, configure the consuming project with
`-DCMAKE_PREFIX_PATH=/path/to/libgs-install`. Available component names and
imported targets are `core`, `coro`, `http`, `websocket`, and `utils`. A
requested component must have been enabled when LibGS was built. The package
also provides the legacy target names (`gs.core`, `gs.coro`, and so on) for
source-tree compatibility and restores required optional system dependencies.

## Supported boundaries

- LibGS is built as shared or static libraries; it is not header-only.
- Source and install trees both support target-based CMake integration.
- The runtime is Asio: use the process-wide default context or an
  application-owned Asio executor.
- TLS requires an application-configured OpenSSL context. LibGS does not own
  certificate policy.
- HTTP supports 1.0 and 1.1, not HTTP/2 or HTTP/3.
- WebSocket implements RFC 6455 over HTTP/1.1.
- Public APIs are pre-1.0 and may change between releases.
