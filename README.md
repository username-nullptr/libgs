# LibGS

LibGS is a modular C++20 foundation for asynchronous applications and services,
built on standalone Asio. It combines an execution runtime, coroutine
synchronization, general-purpose core facilities, application utilities, and
network protocol implementations behind a consistent API model.

LibGS is not limited to HTTP. HTTP/HTTPS is the first protocol family included
in the repository, while the architecture and roadmap cover a broader set of
application and networking capabilities.

## Modules

| Area | What it provides |
| --- | --- |
| Core | Execution, scheduling, timers, values, INI data, argument parsing, algorithms, lock-free queues, dynamic libraries, and system helpers |
| Coroutines | Coroutine mutexes, shared mutexes, semaphores, condition variables, sleeps, future waiting, and executor switching |
| HTTP | HTTP/1.0 and HTTP/1.1 clients, servers, protocol parsing, routing, sessions, connection pooling, file transfer, cookies, redirects, and proxies |
| Utilities | Logging, persistent settings, signals and observers, module initialization, process management, and an extensible soft bus with a built-in in-process transport |

Most asynchronous APIs follow Asio completion-token conventions, allowing the
same operation to be used with callbacks, C++20 coroutines, or explicit error
handling. Selected client and utility APIs also provide synchronous defaults.

## Requirements

| Component | Minimum version or requirement |
| --- | --- |
| CMake | 3.13 |
| C++ | C++20 |
| GCC | 13 |
| Clang | 17, with `std::format` support |
| MSVC | 19.30 / Visual Studio 2022 |

MSVC 19.50 is currently rejected by the build configuration because of a known
compiler issue.

Standalone Asio, spdlog, and nlohmann/json are included in `3rd_party`. OpenSSL
and zlib are only required when their corresponding features are enabled.

## Build and install

Clone the repository:

```shell
git clone https://gitee.com/jin-xiaoqiang/libgs.git
cd libgs
```

Configure, build, and install a shared Release build:

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/libgs-install
cmake --build build --parallel
cmake --install build
```

For multi-config generators such as Visual Studio, select the configuration
while building and installing:

```shell
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=C:/libgs
cmake --build build --config Release --parallel
cmake --install build --config Release
```

Installed artifacts include the public headers, generated configuration
headers, bundled dependency headers, and the enabled LibGS shared libraries.

### CMake options

| Option | Default | Description |
| --- | :---: | --- |
| `LIBGS_BUILD_STATIC` | `OFF` | Build static libraries instead of shared libraries. |
| `LIBGS_ADD_LIBRARY_VERSION` | `ON` | Add version information to shared-library names. |
| `LIBGS_BUILD_EXAMPLES` | `OFF` | Build and install the example programs. |
| `LIBGS_OPENSSL_SUPPORT` | `OFF` | Enable HTTPS/TLS support. Requires OpenSSL. |
| `LIBGS_HTTP_ZLIB_SUPPORT` | `OFF` | Enable HTTP gzip support. Requires zlib. |
| `LIBGS_USE_LIBCXX` | `OFF` | Use libc++ when compiling with Clang. |
| `LIBGS_USE_LLD` | `OFF` | Use lld when compiling with Clang. |
| `LIBGS_ENABLE_LTO` | `OFF` | Enable link-time optimization when compiling with GCC. |

For example, enable TLS, gzip, and examples with:

```shell
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIBGS_OPENSSL_SUPPORT=ON \
  -DLIBGS_HTTP_ZLIB_SUPPORT=ON \
  -DLIBGS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Example executables are written below `build/output/examples`.

## Quick start

The core runtime can schedule immediate and delayed work without requiring an
HTTP component:

```cpp
#include <libgs/core/execution.h>

#include <chrono>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;

    libgs::post([]
    {
        std::cout << "The LibGS event loop is running.\n";
    });

    libgs::post(1s, []
    {
        std::cout << "Stopping after one second.\n";
        libgs::exit();
    });

    return libgs::exec();
}
```

See the documentation below for coroutine synchronization, HTTP client and
server examples, configuration, logging, process control, and other modules.

## Libraries and headers

| Library or header | Purpose |
| --- | --- |
| `gs.core` / `<libgs/core.h>` | Execution, algorithms, values, configuration, containers, and system helpers |
| `<libgs/coro.h>` | Header-only coroutine synchronization primitives |
| `gs.http` / `<libgs/http.h>` | HTTP protocol, client, server, and TLS APIs |
| `gs.utils` / `<libgs/utils.h>` | Utilities library; the umbrella exposes logging, settings, modules, and soft-bus APIs |
| `<libgs.h>` | Umbrella header for all public modules |

Link `gs.http` or `gs.utils` together with `gs.core`. On Unix-like systems,
LibGS also links against the platform thread and dynamic-loader libraries.

## Documentation

- [Documentation index](doc/README.md)
- [Getting started](doc/en/getting-started.md)
- [Core runtime and facilities](doc/en/core.md)
- [Coroutine support](doc/en/coroutines.md)
- [HTTP client and server](doc/en/http.md)
- [Application utilities](doc/en/utilities.md)
- [Project roadmap](doc/en/roadmap.md)

## Examples

See the [examples guide](examples/README.md) for build and run instructions.
Sources are grouped by public module:

- [Core utilities](examples/core)
- [Coroutine primitives](examples/coro)
- [HTTP and HTTPS](examples/http)
- [Application utilities](examples/utils)

Configure the project with `-DLIBGS_BUILD_EXAMPLES=ON` to build them.

## Project direction

LibGS is intended to grow as a reusable asynchronous application foundation,
not as an HTTP-only package. WebSocket support is planned as a future protocol
module. Planned features are documented separately from implemented APIs; see
the [roadmap](doc/en/roadmap.md) for the current direction.

## License

LibGS is available under the [MIT License](LICENSE.txt). The bundled third-party
components retain their respective licenses; their upstream sources are listed
in [3rd_party/SOURCE.txt](3rd_party/SOURCE.txt).
