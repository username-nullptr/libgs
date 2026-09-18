# Getting Started

Language: English | [简体中文](../zh_CN/getting-started.md)

This path builds the default Core and Coroutines modules, runs a checked-in
example, and shows how an application links LibGS.

## 1. Build the repository

LibGS requires CMake 3.15, C++20, and GCC 13+, Clang 17+, or MSVC 19.30+.
Bundled dependencies require no separate download.

```sh
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

The default configuration builds `gs.core`, `gs.coro`, and their examples. Run
the Core execution example:

```sh
./build/output/examples/core/execution
```

For Visual Studio and other multi-config generators, add `--config Release` to
the build command.

## 2. Run a minimal application

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

`libgs::post()` submits work to the process-wide default context.
`libgs::exec()` runs that context and blocks until `libgs::exit()` stops it.
Executor-aware overloads can instead use an application-owned Asio context.

When LibGS is part of the same CMake build, link the module used by the program:

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app LANGUAGES CXX)

add_subdirectory(path/to/libgs)

add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_20)
target_link_libraries(my_app PRIVATE gs.core)
```

## 3. Enable application modules

HTTP, WebSocket, and Utilities are opt-in. A build containing every module and
all matching examples is configured with:

```sh
cmake -S . -B build-full -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_BUILD_HTTP=ON \
  -DLIBGS_BUILD_WEBSOCKET=ON \
  -DLIBGS_BUILD_UTILITIES=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build-full --parallel
```

Link the highest module the application uses. For example, linking
`gs.websocket` also supplies its public HTTP, Coroutine, and Core dependencies.

## Continue

- [Build and configuration](build.md): module switches, OpenSSL, zlib, io_uring,
  installation, and consumer setup.
- [Execution and I/O model](io-model.md): completion tokens, buffer lifetime,
  cancellation, strands, and per-object concurrency.
- [Examples](../../examples/README.md): runnable programs grouped by module.
- [Module guides](README.md#module-guides): current public capabilities.
