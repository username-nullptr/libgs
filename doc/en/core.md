# Core Runtime and Facilities

Language: English | [Simplified Chinese](../zh_CN/core.md)

The core module supplies the execution model and general-purpose facilities
used by every other LibGS module. Link it as `gs.core`.

## Header guide

| Header | Main facilities |
| --- | --- |
| `<libgs/core/execution.h>` | Default event loop, dispatch, post, timers, sleeps, and completion initiation |
| `<libgs/core/value.h>` | String-backed typed values and conversions |
| `<libgs/core/url.h>` | Protocol-neutral hierarchical URL parsing, query parameters, and reference resolution |
| `<libgs/core/ini.h>` | In-memory INI data with synchronous and asynchronous persistence |
| `<libgs/core/args_parser.h>` | Command-line groups, flags, version, and help handling |
| `<libgs/core/algorithm.h>` | UUID, SHA-1, wildcard matching, percent encoding, and math helpers |
| `<libgs/core/lock_free_queue.h>` | Linked and circular lock-free queues |
| `<libgs/core/shared_mutex.h>` | Standard and spin-based shared mutex aliases |
| `<libgs/core/system.h>` | Application paths, environment access, dynamic libraries, and CPU helpers |
| `<libgs/core/mime_type.h>` | MIME lookup, binary/text detection, and text encoding checks |
| `<libgs/core/string_list.h>` and related headers | Character-aware string lists, sets, vectors, and deques |

`<libgs/core.h>` is a convenient umbrella for the commonly used core APIs.
More specialized facilities can be included directly.

## Default execution runtime

LibGS owns a default `asio::io_context` for applications that do not supply
their own executor:

- `libgs::io_context()` returns the context;
- `libgs::get_executor()` returns its executor;
- `libgs::exec()` runs the event loop and blocks;
- `libgs::exit(code)` requests shutdown and sets the return code; and
- `libgs::is_run()` reports whether the default loop is running.

The main scheduling operations are:

| Operation | Behavior |
| --- | --- |
| `dispatch` | Runs immediately when already in the target context; otherwise queues the work |
| `post` | Always queues work on the target context |
| delayed `post` | Queues work for a relative or absolute time and returns a cancellation function |
| `start_timer` | Starts repeating work and returns a cancellation function |
| `local_dispatch` | Runs work through a temporary local execution context |
| `sleep_for`, `sleep_until` | Wait synchronously or asynchronously, depending on the overload and token |

```cpp
#include <libgs/core/execution.h>

#include <chrono>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;

    libgs::dispatch([]
    {
        std::cout << "dispatched\n";
    });

    auto cancel_timer = libgs::start_timer(250ms, []
    {
        std::cout << "tick\n";
    });

    libgs::post(1s, [cancel_timer = std::move(cancel_timer)]() mutable
    {
        cancel_timer();
        libgs::exit();
    });

    return libgs::exec();
}
```

Keep captured data alive until asynchronous work completes. This is especially
important for buffers and references captured by detached operations.

## External executors

Many scheduling functions have an overload whose first argument is a compatible
executor, scheduler, or execution context. HTTP, process, INI, observer, and
soft-bus types also expose executor-aware variants. Use these overloads when
the application owns its event loop or separates work across contexts.

## Values and optional conversions

`libgs::value` stores text while providing formatting, classification, and
typed conversion helpers. Wide and UTF character variants are also available.

```cpp
#include <libgs/core/value.h>

int main()
{
    libgs::value port = 8080;
    auto parsed_port = port.to_uint();

    libgs::value message("request {} completed", 42);
    auto text = message.to_string();

    return parsed_port && !text.empty() ? 0 : 1;
}
```

Conversions return LibGS optional values. Test the result or use the optional
fallback helpers rather than assuming arbitrary input is valid.

## URLs

`libgs::url` is a protocol-neutral hierarchical URL type. It parses the scheme,
host, port, path, and query parameters and resolves relative references. The
default value is the local URL `local:///`; HTTP-specific scheme validation is
performed by the HTTP client rather than by the Core type.

```cpp
#include <libgs/core/url.h>

libgs::url source("file:///tmp/report.txt");
libgs::url endpoint("https://example.test/api?page=1");

auto next = libgs::url::resolve(endpoint, "../status");
```

## INI data

`libgs::ini` provides group/key access, iteration, data replacement, and file
persistence. Load and sync operations accept LibGS completion tokens.

```cpp
#include <libgs/core/ini.h>

int main()
{
    libgs::ini config("./app.ini");
    config.load_or();

    config.write("server/host", "127.0.0.1");
    config.write("server/port", 8080);
    config.sync();

    auto host = config.read("server/host");
    return host ? 0 : 1;
}
```

`load_or` tolerates a missing source by keeping an empty data set, while `load`
reports the file error. Automatic periodic syncing and sync-on-destruction can
be configured with `set_sync_period` and `set_sync_on_delete`.

## Command-line parsing

`libgs::cmdline::args_parser` supports value-taking option groups, combined
flags, help output, version output, aliases, and collection of unmatched
arguments. Rules can contain comma-separated aliases such as `-f,--file`.

See [`examples/core/args_parser.cpp`](../../examples/core/args_parser.cpp) for a
complete example.

## Algorithms and identifiers

The algorithm umbrella includes:

- UUID generation for versions 4, 5, 6, and 7;
- incremental SHA-1 hashing with hexadecimal and Base64 output;
- wildcard matching with `*` and `?`;
- percent encoding and decoding; and
- arithmetic mean helpers.

```cpp
#include <libgs/core/algorithm.h>

auto id = libgs::uuid::generate();
auto match_weight = libgs::wildcard_match("lib*.so*", "libexample.so.1");

libgs::sha1 digest("payload");
auto hex = digest.finalize().hex();
```

SHA-1 is provided for protocol and compatibility uses. It should not be chosen
for new security-sensitive password or signature designs.

## Containers and synchronization

The core module includes linked and circular lock-free queues, string-specialized
containers, a spin mutex, and a spin-based shared mutex. These are thread-level
facilities. For code that must suspend without blocking an executor thread, use
the primitives described in [Coroutines](coroutines.md).

## System facilities

The `libgs::app` helpers expose executable paths, working-directory operations,
absolute-path handling, environment access, current-user lookup, and home
directory lookup. Most return `sys_expected<T>` so platform errors can be
handled without exceptions.

`libgs::library` wraps dynamic-library loading and typed symbol lookup. Always
keep the library object alive while calling functions obtained from it.

Platform-specific implementations are present for Windows and Unix-like
systems. Check the returned `sys_expected<T>` rather than assuming a facility
behaves identically on every platform.

## Related examples

- [`examples/core/execution.cpp`](../../examples/core/execution.cpp)
- [`examples/core/value.cpp`](../../examples/core/value.cpp)
- [`examples/core/ini.cpp`](../../examples/core/ini.cpp)
- [`examples/core/algorithms.cpp`](../../examples/core/algorithms.cpp)
- [`examples/core/lock_free_queue.cpp`](../../examples/core/lock_free_queue.cpp)
- [`examples/core/app_paths.cpp`](../../examples/core/app_paths.cpp)
- [`examples/core/args_parser.cpp`](../../examples/core/args_parser.cpp)
- [`examples/core/dynamic_library.cpp`](../../examples/core/dynamic_library.cpp)
