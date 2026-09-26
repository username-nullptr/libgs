# Core

Language: English | [简体中文](../zh_CN/core.md)

Target: `gs.core`. Core is always built and underpins every other module.

## Public API map

| Header | Main facilities |
| --- | --- |
| `<libgs/core/execution.h>` | Default context, executors, scheduling, timers, sleeps, async work |
| `<libgs/core/value.h>`, `container.h` | Typed text values and parameter containers |
| `<libgs/core/url.h>`, `ini.h` | URL parsing/resolution and INI persistence |
| `<libgs/core/args_parser.h>` | Command-line groups, options, flags, help, version |
| `<libgs/core/algorithm.h>` | UUID, SHA-1, wildcard, encoding, and math helpers |
| `<libgs/core/lock_free_queue.h>` | Linked and circular lock-free queues |
| `<libgs/core/atomic_mutex.h>`, `shared_mutex.h` | Atomic exclusive/shared locks |
| `<libgs/core/jthread.h>` | Portable C++20 joining thread and stop tokens |
| `<libgs/core/system.h>` | Application paths, environment, CPU, dynamic libraries |
| `<libgs/core/mime_type.h>` | MIME lookup and text/binary classification |
| `<libgs/core/cxx/...>` | Concepts, traits, formatting, expected/optional compatibility |

`<libgs/core.h>` includes the common data, algorithm, synchronization, and
system headers. Include execution, value, MIME, queue, or other specialized
headers directly when needed.

## Default runtime

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post(1s, [] { libgs::exit(); });
    return libgs::exec();
}
```

| API | Role |
| --- | --- |
| `io_context()`, `get_executor()` | Access the process-wide default runtime |
| `exec()`, `exit(code)` | Run and stop the default runtime |
| `dispatch()`, `post()` | Submit immediate or delayed work |
| `start_timer()` | Start periodic work and return a cancellation function |
| `sleep_for()`, `sleep_until()` | Synchronous or token-based delay |

Scheduling functions also accept external Asio executors. Ownership and strand
rules are defined in [Execution and I/O model](io-model.md).

## Data, files, and system facilities

- `value` stores text and performs checked conversions.
- `url` parses hierarchical URLs, queries, and relative references. Protocol
  modules decide which schemes they accept.
- `ini` provides group/key access plus synchronous and completion-token-based
  file operations.
- `cmdline::args_parser` handles aliases, value options, combined flags,
  positional arguments, help, and version output.
- `libgs::app` exposes paths, working directory, environment, and user/home
  helpers.
- `libgs::library` loads shared libraries and symbols. The library object must
  outlive symbols obtained from it.

Use Core locks and queues for thread synchronization; use
[Coroutine](coroutines.md) primitives when waiting must suspend a coroutine.
`atomic_mutex` and `atomic_shared_mutex` use the balanced policy by default.
The low-latency policy spins and is suitable only for short, bounded critical
sections on controlled threads.

`libgs::jthread`, `stop_token`, `stop_source`, and `stop_callback` map directly
to their standard-library counterparts when they are available. Otherwise,
Core supplies the same cooperative-stop and automatic-join interface on top of
`std::thread`.

## Examples

See the [Core example index](../../examples/README.md#core) for execution,
values, INI, algorithms, queues, paths, argument parsing, and dynamic libraries.
