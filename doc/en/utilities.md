# Utilities

Language: English | [简体中文](../zh_CN/utilities.md)

`gs.utils` provides application-level services on top of `gs.coro`.

## Public structure

| Header | Facility |
| --- | --- |
| `<libgs/utils/logger.h>` | Named console and rotating-file loggers |
| `<libgs/utils/settings.h>` | Named INI-backed settings and change signals |
| `<libgs/utils/signal_slot.h>` | Synchronous, asynchronous, and backpressure signal delivery |
| `<libgs/utils/observer.h>` | ID-addressed callbacks dispatched through an executor |
| `<libgs/utils/modules.h>` | Dependency-ordered application initialization |
| `<libgs/utils/process.h>` | Child process lifecycle and standard-stream I/O |
| `<libgs/utils/sbus.h>` | Extensible publish/subscribe and cached topic state |

`<libgs/utils.h>` includes logger, settings, modules, and soft bus. Include
process, signal/slot, and observer headers directly.

## Facilities

### Logging

`utils::logger` manages named instances and console, daily, warning, error, and
critical sinks. Configuration controls paths, levels, rotation, timestamps,
and formatting. Logging macros capture `std::source_location`. File sinks are
asynchronous; use `logger::flush()` when completion must be observed.

### Settings

`utils::settings` wraps `libgs::ini` as a named instance and emits `changed`
and `loaded` signals. Use `get()`/`set()` for values and `ini()` for direct
access to persistence controls.

### Signals and observers

`utils::signal<Signature>` supports three delivery modes:

| Mode | Behavior |
| --- | --- |
| `sync` | Invoke slots directly and block the caller |
| `async` | Queue slots on an executor |
| `backpressure` | Queue on an executor and block until delivery |

Do not use backpressure from the same executor thread that must perform the
delivery. Signals own their dispatch state and value arguments, but not objects
referenced through views, pointers, references, or slot captures.

`utils::observer` routes indexed callback signatures by stable object ID. The
observer is shared-owned and unregisters itself on destruction.

### Modules

`utils::modules` registers named initializers and orders them by parent/child
dependencies. Initializers may accept command-line arguments and return `void`
or `bool`. `do_init()` supports synchronous and asynchronous completion;
`sprint()` prints the dependency graph.

### Processes

`utils::process` and `utils::wprocess` support start/run, join, detach,
terminate, kill, cancellation, timeouts, environment/working-directory setup,
single-instance locks, and standard input/output/error streams. Keep the
process object alive while asynchronous I/O is pending and inspect returned
errors for platform-specific failures. Following the
[execution and I/O model](io-model.md), serialize lifecycle calls with I/O
initiation and keep at most one stdin write, one stdout read, and one stderr
read outstanding on a process object.

### Soft bus

The soft bus separates its typed publish/subscribe/cache API from the
transport. `sbus::local_interface`, `local_subscriber`, and `local_cache`
provide the built-in in-process transport. `basic_subscriber<Interface>`,
`cache<Subscriber>`, and `publish<Interface>()` allow another transport to use
the same API; no distributed transport is bundled.

`utils::thread_pool()` returns the module's shared Asio thread pool. Prefer an
owned executor when isolation or shutdown order matters.

## Examples

- [Logger](../../examples/utils/logger.cpp)
- [Settings](../../examples/utils/settings.cpp)
- [Signals](../../examples/utils/signal_slot.cpp)
- [Observer](../../examples/utils/observer.cpp)
- [Modules](../../examples/utils/modules)
- [Process](../../examples/utils/process.cpp)
- [Local soft bus](../../examples/utils/soft_bus_local.cpp)
- [Custom soft-bus transport](../../examples/utils/soft_bus_transport.cpp)
