# Core

Language: English | [简体中文](../zh_CN/core.md)

`gs.core` contains the runtime and shared facilities used by every other
module.

## Public structure

| Header area | Facilities |
| --- | --- |
| `<libgs/core/execution.h>` | Default context, executors, scheduling, timers, sleeps, and async work |
| `<libgs/core/value.h>`, `<libgs/core/container.h>` | Typed string values and parameter containers |
| `<libgs/core/url.h>`, `<libgs/core/ini.h>` | URL handling and INI persistence |
| `<libgs/core/args_parser.h>` | Command-line groups, flags, help, and version handling |
| `<libgs/core/algorithm.h>` | UUID, SHA-1, wildcard matching, encoding, and math helpers |
| `<libgs/core/lock_free_queue.h>`, `<libgs/core/shared_mutex.h>` | Thread-level queues and locks |
| `<libgs/core/system.h>` | Application paths, CPU information, environment, and dynamic libraries |
| `<libgs/core/mime_type.h>` | MIME lookup and text/binary checks |
| `<libgs/core/cxx/...>` | Compatibility types, concepts, traits, formatting, and expected/optional support |

`<libgs/core.h>` includes the commonly used Core facilities. Execution and
some specialized headers are included directly.

## Execution runtime

The default runtime owns one process-wide `asio::io_context`:

| API | Role |
| --- | --- |
| `io_context()` | Return the default context |
| `get_executor()` | Return its executor |
| `exec()` | Run the context and block |
| `exit(code)` | Stop it and set the return code |
| `dispatch()` | Run immediately when compatible, otherwise enqueue |
| `post()` | Enqueue immediate or delayed work |
| `start_timer()` | Start periodic work and return a canceller |
| `sleep_for()`, `sleep_until()` | Synchronous or token-based wait |

```cpp
#include <libgs/core/execution.h>

#include <chrono>
#include <utility>

int main()
{
    using namespace std::chrono_literals;

    auto timer = libgs::start_timer(250ms, []
    {
        // Runs repeatedly on the default context.
    });

    libgs::post(1s, [timer = std::move(timer)]() mutable
    {
        timer();
        libgs::exit();
    });

    return libgs::exec();
}
```

Scheduling functions and many higher-level types also accept an external Asio
executor. Keep captured objects and borrowed buffers alive until asynchronous
completion.

## Data and configuration

- `value` stores text and provides checked conversion to numeric and other
  supported types.
- `url` parses hierarchical URLs, query parameters, and relative references.
  Protocol modules decide which schemes they accept.
- `ini` provides group/key access and synchronous or completion-token-based
  persistence.
- `cmdline::args_parser` supports aliases, value options, combined flags, help,
  version output, and unmatched arguments.
- String container aliases preserve the same character type across lists,
  vectors, sets, and deques.

## Algorithms, synchronization, and system APIs

The algorithm headers provide UUID versions 4/5/6/7, SHA-1, percent encoding,
wildcard matching, and arithmetic helpers. SHA-1 is suitable for protocol
compatibility, not new password or signature designs.

Core locks and lock-free queues are thread synchronization tools. Use
[Coroutine](coroutines.md) primitives when a waiter must suspend without
blocking an executor thread.

`libgs::app` exposes executable paths, working-directory and environment
operations, home/user lookup, and absolute-path helpers. `libgs::library`
loads dynamic libraries and symbols; the library object must outlive every
symbol obtained from it. Platform failures are generally returned as
`sys_expected<T>`.

## Examples

- [Execution](../../examples/core/execution.cpp)
- [Values](../../examples/core/value.cpp)
- [INI](../../examples/core/ini.cpp)
- [Algorithms](../../examples/core/algorithms.cpp)
- [Lock-free queues](../../examples/core/lock_free_queue.cpp)
- [Application paths](../../examples/core/app_paths.cpp)
- [Argument parsing](../../examples/core/args_parser.cpp)
- [Dynamic libraries](../../examples/core/dynamic_library.cpp)
