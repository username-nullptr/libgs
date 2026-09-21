# Build and Configuration

Language: English | [简体中文](../zh_CN/build.md)

This page is the reference for build inputs and consuming LibGS. The
[quick start](getting-started.md) is the shorter path for a first build.

## Toolchain and dependencies

| Component | Requirement |
| --- | --- |
| CMake | 3.15 or newer |
| Language | C++20 |
| GCC | 13 or newer |
| Clang | 17 or newer |
| MSVC | 19.30 or newer / Visual Studio 2022+ |

Standalone Asio, spdlog, and nlohmann/json are included under `3rd_party/`.
The following system packages are needed only when the matching feature is
enabled:

| Dependency | Feature |
| --- | --- |
| OpenSSL | TLS connections, HTTPS, and WSS |
| zlib | HTTP gzip and WebSocket `permessage-deflate` |
| liburing | Asio io_uring backend on Linux |

## Module selection

| CMake switch | Default | Target | Requirement |
| --- | :---: | --- | --- |
| Core is always built | ON | `gs.core` | — |
| `LIBGS_BUILD_CORO` | ON | `gs.coro` | Core |
| `LIBGS_BUILD_HTTP` | OFF | `gs.http` | Coroutines must be ON |
| `LIBGS_BUILD_WEBSOCKET` | OFF | `gs.websocket` | HTTP must be ON |
| `LIBGS_BUILD_UTILITIES` | OFF | `gs.utils` | Coroutines must be ON |

CMake rejects a configuration that enables a module while disabling its
required lower-level module. Module targets publish their dependencies, so a
source-tree consumer links only the highest module it uses.

## Feature and toolchain switches

| CMake switch | Default | Availability | Effect |
| --- | :---: | --- | --- |
| `LIBGS_BUILD_STATIC` | OFF* | all platforms | Build static rather than shared libraries |
| `LIBGS_ADD_LIBRARY_VERSION` | ON | shared builds | Add version information to shared-library names |
| `LIBGS_OPENSSL_SUPPORT` | OFF | OpenSSL | Enable TLS connection types, HTTPS, and WSS |
| `LIBGS_HTTP_ZLIB_SUPPORT` | OFF | HTTP + zlib | Enable HTTP gzip; WebSocket inherits zlib support |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | OFF | WebSocket + zlib | Enable only WebSocket `permessage-deflate` |
| `LIBGS_BUILD_UTILITIES_SBUS_UDP` | ON | Utilities | Enable the Asio UDP multicast soft-bus interface |
| `LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` | `local` | Utilities | Select `local` or `udp` for the unqualified soft-bus API |
| `LIBGS_IO_URING_SUPPORT` | OFF | Linux + liburing | Use Asio's io_uring backend and disable epoll |
| `LIBGS_USE_LIBCXX` | OFF | Clang | Compile and link with libc++ |
| `LIBGS_USE_LLD` | OFF | Clang | Link with lld |
| `LIBGS_ENABLE_LTO` | OFF | GCC | Enable link-time optimization |
| `LIBGS_BUILD_EXAMPLES` | OFF | enabled modules | Build the repository examples |
| `LIBGS_HEAVY_COMPILE_JOBS` | platform default | supported build generators | Limit concurrent HTTP/WebSocket compilations; `0` disables the limit |
| `LIBGS_LOW_MEMORY_DEBUG_INFO` | OFF | GCC | Reduce debug information for lower compiler memory use |

`LIBGS_WEBSOCKET_ZLIB_SUPPORT` is exposed only when WebSocket is enabled and
HTTP zlib is disabled. Define features through CMake; do not define generated
configuration macros in application code.

*A Windows GNU toolchain without a shared `libstdc++-6.dll` defaults to a static
build. Requesting a shared build with that toolchain is rejected.*

Test, stress, fuzz, performance, and sanitizer switches are kept in the
[test guide](../../test/README.md).

## Common configurations

Default Core and Coroutines build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

All modules and examples, without optional system dependencies:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

HTTP, WebSocket, TLS, and compression:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

For Visual Studio and other multi-config generators, select the configuration
when building and installing:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

## Build output and installation

Build-tree artifacts are placed under:

| Artifact | Directory |
| --- | --- |
| Shared libraries and runtime binaries | `build/output/bin` |
| Static/import libraries | `build/output/lib` |
| Examples | `build/output/examples/<module>` |
| Generated configuration headers | `build/output/config_include` |

Install with an explicit prefix when the default system prefix is not desired:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

The install tree contains public headers, the libraries for enabled modules,
and examples when `LIBGS_BUILD_EXAMPLES=ON`.

## Consume the source tree

```cmake
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.http)
```

Set LibGS options before `add_subdirectory`; the example above requires
`LIBGS_BUILD_HTTP=ON` in the containing build.

## Consume an install tree

The install tree does not include a CMake package configuration. Locate the
headers and each required library explicitly:

```cmake
find_path(LIBGS_INCLUDE_DIR NAMES libgs.h REQUIRED)
find_library(LIBGS_CORE_LIBRARY NAMES gs.core REQUIRED)
find_library(LIBGS_CORO_LIBRARY NAMES gs.coro REQUIRED)
find_library(LIBGS_HTTP_LIBRARY NAMES gs.http REQUIRED)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_include_directories(my_app PRIVATE "${LIBGS_INCLUDE_DIR}")
target_link_libraries(my_app PRIVATE
  "${LIBGS_HTTP_LIBRARY}"
  "${LIBGS_CORO_LIBRARY}"
  "${LIBGS_CORE_LIBRARY}"
)
```

Supply optional system libraries required by the selected LibGS feature set as
part of the consuming application's link configuration.
