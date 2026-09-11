# Getting Started

Language: English | [Simplified Chinese](../zh_CN/getting-started.md)

This guide covers the current source-build and installation workflow for
LibGS.

## Requirements

| Component | Requirement |
| --- | --- |
| CMake | 3.13 or newer |
| Language mode | C++20 |
| GCC | 13 or newer |
| Clang | 17 or newer, with `std::format` support |
| MSVC | 19.30 or newer / Visual Studio 2022 |

The build currently rejects MSVC 19.50 because of a known compiler issue.

Standalone Asio, spdlog, and nlohmann/json are vendored in the repository.
OpenSSL and zlib are optional system dependencies.

## Build from source

```shell
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

For a multi-config generator, select the configuration during build and
installation:

```shell
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

The default build produces shared `gs.core`, `gs.http`, `gs.websocket`, and
`gs.utils` libraries. Generated files and binaries are placed below
`build/output`.

## Build options

| Option | Default | Applies to | Purpose |
| --- | :---: | --- | --- |
| `LIBGS_BUILD_STATIC` | `OFF` | All libraries | Build static instead of shared libraries |
| `LIBGS_ADD_LIBRARY_VERSION` | `ON` | Shared builds | Add project and ABI versions to library names |
| `LIBGS_BUILD_EXAMPLES` | `OFF` | Examples | Build the examples enabled by the current CMake configuration |
| `LIBGS_BUILD_FUZZERS` | `OFF` | Tests | Build Clang libFuzzer parser harnesses; requires `BUILD_TESTING=ON` |
| `LIBGS_OPENSSL_SUPPORT` | `OFF` | Core, HTTP, and WebSocket | Enable TLS, HTTPS, and WSS support through OpenSSL |
| `LIBGS_HTTP_ZLIB_SUPPORT` | `OFF` | HTTP and WebSocket | Enable HTTP gzip and let WebSocket inherit zlib support |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | `OFF`* | WebSocket | Enable `permessage-deflate` when HTTP zlib support is disabled |
| `LIBGS_USE_LIBCXX` | `OFF` | Clang | Compile and link with libc++ |
| `LIBGS_USE_LLD` | `OFF` | Clang | Link with lld |
| `LIBGS_ENABLE_LTO` | `OFF` | GCC | Enable link-time optimization |

Enable optional protocol dependencies only when needed:

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON
cmake --build build --parallel
```

`LIBGS_WEBSOCKET_ZLIB_SUPPORT` is offered only while
`LIBGS_HTTP_ZLIB_SUPPORT=OFF`. Enabling HTTP zlib support automatically enables
the WebSocket zlib-backed profile, so no second option is needed. With HTTP
zlib disabled, enable only WebSocket compression with
`-DLIBGS_WEBSOCKET_ZLIB_SUPPORT=ON`.

Do not define the generated feature macros manually. Configure the CMake
options so the library and installed configuration headers agree.

## Build and run examples

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Example executables are written to subdirectories of
`build/output/examples`. The source examples are grouped under:

- [`examples/core`](../../examples/core)
- [`examples/coro`](../../examples/coro)
- [`examples/http`](../../examples/http)
- [`examples/websocket`](../../examples/websocket)
- [`examples/utils`](../../examples/utils)

All listed examples, including the HTTP servers, are enabled by
`LIBGS_BUILD_EXAMPLES`. HTTPS and WSS additionally require
`LIBGS_OPENSSL_SUPPORT=ON`.
See the [examples guide](../../examples/README.md) for run instructions and
capability coverage.

## Use an installed build

Installation places public and generated headers below `<prefix>/include` and
shared libraries below the platform library or runtime directory. The project
does not currently install a CMake package configuration, so consumers must
provide the include and library locations explicitly.

A minimal CMake integration for an installed HTTP build is:

```cmake
find_path(LIBGS_INCLUDE_DIR NAMES libgs.h REQUIRED)
find_library(LIBGS_CORE_LIBRARY NAMES gs.core REQUIRED)
find_library(LIBGS_HTTP_LIBRARY NAMES gs.http REQUIRED)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_include_directories(my_app PRIVATE "${LIBGS_INCLUDE_DIR}")
target_link_libraries(my_app PRIVATE
    "${LIBGS_HTTP_LIBRARY}"
    "${LIBGS_CORE_LIBRARY}"
)
```

Set `CMAKE_PREFIX_PATH` to the install prefix if CMake cannot locate the files.
Applications using `gs.utils` should locate and link `gs.utils` together with
`gs.core`. Applications using WebSocket should also locate and link
`gs.websocket`; its public dependency on `gs.http` supplies the HTTP upgrade
layer. Ensure the shared-library directory is available to the platform runtime
loader when launching the application.

## Start the default runtime

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post([]
    {
        // Immediate work on the default event loop.
    });

    libgs::post(1s, []
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

`libgs::exec()` blocks while it runs the default event loop. `libgs::exit()`
requests termination and sets the value returned by `exec()`.

## Choose an error style

Selected APIs offer a throwing synchronous default and a non-throwing
error-code form. For example:

```cpp
#include <libgs/http/client.h>

int main()
{
    std::error_code error;
    libgs::http::client client;

    auto context = client.request_get("http://example.com/", error);
    if (error)
    {
        // Handle the connection or request error.
        return 1;
    }

    return context ? 0 : 1;
}
```

Coroutine callers pass `libgs::use_awaitable` and `co_await` the result. Callback
forms use the completion signature documented by the corresponding header.

## Next steps

- Learn the [core runtime and facilities](core.md).
- Use [coroutine synchronization](coroutines.md).
- Build an [HTTP client or server](http.md).
- Explore [application utilities](utilities.md).
