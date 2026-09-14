# Getting Started

Language: English | [简体中文](../zh_CN/getting-started.md)

## Requirements

| Component | Minimum |
| --- | --- |
| CMake | 3.15 |
| C++ | C++20 |
| GCC | 13 |
| Clang | 17 |
| MSVC | 19.30 / Visual Studio 2022 |

The repository includes standalone Asio, spdlog, and nlohmann/json. Optional
features find their system dependencies during CMake configuration.

## Configure the modules

All modules are enabled by default. Their switches and dependencies are:

| Switch | Default | Result |
| --- | :---: | --- |
| `LIBGS_BUILD_CORO` | `ON` | Build `gs.coro`; required by HTTP and Utilities |
| `LIBGS_BUILD_HTTP` | `ON` | Build `gs.http`; requires Coroutines |
| `LIBGS_BUILD_WEBSOCKET` | `ON` | Build `gs.websocket`; requires HTTP |
| `LIBGS_BUILD_UTILITIES` | `ON` | Build `gs.utils`; requires Coroutines |
| `LIBGS_BUILD_STATIC` | `OFF`* | Build static instead of shared libraries |
| `LIBGS_ADD_LIBRARY_VERSION` | `ON` | Version shared-library file names |
| `LIBGS_BUILD_EXAMPLES` | `OFF` | Build example executables |

Feature switches:

| Switch | Platform/dependency | Result |
| --- | --- | --- |
| `LIBGS_OPENSSL_SUPPORT` | OpenSSL | HTTPS, WSS, and TLS connection types |
| `LIBGS_HTTP_ZLIB_SUPPORT` | zlib | HTTP gzip and WebSocket compression support |
| `LIBGS_WEBSOCKET_ZLIB_SUPPORT` | zlib | WebSocket `permessage-deflate` without HTTP gzip |
| `LIBGS_IO_URING_SUPPORT` | Linux and liburing | Use Asio's io_uring backend |
| `LIBGS_USE_LIBCXX` | Clang | Build with libc++ |
| `LIBGS_USE_LLD` | Clang | Link with lld |
| `LIBGS_ENABLE_LTO` | GCC | Enable link-time optimization |

When HTTP zlib is enabled, WebSocket inherits zlib support and its separate
switch is not exposed. Configure features through CMake rather than defining
generated feature macros in application code.

On Windows, a GNU toolchain without shared `libstdc++` defaults to a static
build. Test, sanitizer, and fuzzer switches are documented in the
[test guide](../../test/README.md).

## Build and install

```sh
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

For a multi-config generator:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

Libraries are generated below `build/output/bin` or `build/output/lib`.
Examples are generated below `build/output/examples`.

## Link an application

Link the highest module that the application uses; its public dependencies are
propagated when LibGS is included with `add_subdirectory`:

```cmake
add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE gs.http)
```

The install tree currently does not provide a CMake package configuration.
Installed consumers must supply the include path and locate the required
libraries explicitly:

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

## Run the default context

```cpp
#include <libgs/core/execution.h>

int main()
{
    libgs::post([]
    {
        libgs::exit();
    });

    return libgs::exec();
}
```

`libgs::exec()` blocks while it runs the default `asio::io_context`.
`libgs::exit(code)` stops that context and sets the value returned by `exec()`.
Executor-aware overloads let an application use its own context.

## Completion and errors

Read each declaration for its supported completion tokens. The common forms
are:

| Form | Selection |
| --- | --- |
| Coroutine | pass `libgs::use_awaitable`, then `co_await` |
| Callback | pass a compatible completion handler |
| Detached | pass `libgs::detached` where fire-and-forget is supported |
| Synchronous, throwing | use the default synchronous token |
| Synchronous, non-throwing | pass a `std::error_code&` |

## Next steps

- [Core runtime and facilities](core.md)
- [Coroutine support](coroutines.md)
- [HTTP client and server](http.md)
- [WebSocket](websocket.md)
- [Application utilities](utilities.md)
