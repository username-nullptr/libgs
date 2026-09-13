# Application Utilities

Language: English | [Simplified Chinese](../zh_CN/utilities.md)

The utilities module provides reusable services that commonly sit around an
application's event loop: logging, persistent settings, signals, observers,
module initialization, child processes, and publish/subscribe state. Link it as
`gs.utils`; it publicly depends on `gs.coro` and `gs.core`.

## Header guide

| Header | Purpose |
| --- | --- |
| `<libgs/utils/logger.h>` | Named console and rotating file loggers |
| `<libgs/utils/settings.h>` | Named INI-backed settings instances and change signals |
| `<libgs/utils/signal_slot.h>` | Synchronous, asynchronous, and backpressure signal/slot delivery |
| `<libgs/utils/observer.h>` | ID-addressed observer callbacks on an executor |
| `<libgs/utils/modules.h>` | Dependency-aware module registration and initialization |
| `<libgs/utils/process.h>` | Child-process lifecycle and standard-stream I/O |
| `<libgs/utils/sbus.h>` | Extensible soft bus, typed payloads, and cached topic state |
| `<libgs/utils.h>` | Logger, settings, modules, and soft-bus umbrella |

Process, signal/slot, and observer APIs should currently be included through
their individual headers.

## Logging

`libgs::utils::logger` manages named logger instances. A logger configuration
controls the output path, timestamp mode, line breaks, console and daily log
levels, and rotation limits for warning, error, and critical files.

```cpp
#include <libgs/utils/logger.h>

int main()
{
    libgs::utils::logger::config_t config {
        .path = "./logs"
    };

    libgs::utils::logger::instance().set_config(config);

    libgs_utils_log_info("service started on port {}", 8080);
    libgs_utils_clog_warning("network", "retry {}", 3);
}
```

The logging macros capture source file, function, and line information through
`std::source_location`. Named instances are created on first access unless
`instance(name, false)` is used.

## Settings

`libgs::utils::settings` wraps `libgs::ini` in a named singleton and exposes
`changed` and `loaded` signals.

```cpp
#include <libgs/utils/settings.h>

int main()
{
    auto &settings = libgs::utils::settings::instance();
    settings.load("./app.ini");

    settings.changed.connect(
        [](std::string_view path, const libgs::value &value)
        {
            // React to the changed setting.
        }
    );

    settings
        .set("server/host", "127.0.0.1")
        .set("server/port", 8080)
        .sync();
}
```

Use `get(path)` or a `{group, key}` pair to retrieve an optional value. The
underlying INI object remains available through `ini()` for iteration and
advanced persistence settings.

## Signals and slots

`libgs::utils::signal<Signature>` connects one signal to multiple compatible
callables. Slot arguments may be a compatible prefix of the signal arguments
and can use supported argument conversions.

Delivery modes are:

| Mode | Behavior |
| --- | --- |
| `sync` | Direct blocking invocation; the signal implementation documents it as thread-unsafe |
| `async` | Non-blocking delivery through an executor |
| `backpressure` | Thread-safe blocking delivery through an executor; misuse on the same blocked executor can deadlock |

Signals support free functions, lambdas, observer objects held by shared
pointers, explicit executors, coroutine slots, disconnect operations, and
temporary blocking.

An emission owns its internal state and slot snapshot until that dispatch
finishes, so a returned awaitable does not require the signal object to remain
alive. Arguments passed by value are owned by the operation as well. With an
observer overload, the implementation locks the weak pointer before invocation
and holds the resulting strong reference across the complete callback,
including coroutine suspension.

A signal cannot manage external objects captured by a slot or extend the
lifetime of data referenced through references, raw pointers, or views. The
caller must keep such non-owning data valid, and must not destroy the signal
object concurrently with another thread entering one of its member functions.

```cpp
#include <libgs/utils/signal_slot.h>

int main()
{
    libgs::utils::signal<void(int)> changed;

    changed.connect([](int value)
    {
        // Synchronous slot.
    });

    changed(42);
}
```

Choose asynchronous or backpressure delivery deliberately. In particular, do
not wait synchronously on work queued to the same executor thread.

## Observers

`libgs::utils::observer` provides ID-addressed callback groups. An observer is
created as a shared object, registers one or more indexed callback signatures,
and receives static `trigger<Index>(id, ...)` events through its executor.

This model is useful when producers know a stable object ID but should not own a
direct pointer to the receiver. Observer destruction removes the receiver from
future delivery.

## Dependency-aware modules

`libgs::utils::modules` registers initialization functions under a name and
orders them using parent and child dependencies. Initializers may receive
command-line arguments and return `void` or `bool`.

```cpp
#include <libgs/utils/modules.h>

LIBGS_UTILS_MODULE_INIT("storage", []
{
    // Initialize storage.
});

LIBGS_UTILS_MODULE_INIT(
    "api",
    {.parents = {"storage"}},
    []
    {
        // Runs after storage.
    }
);
```

Call `modules::do_init(...)` after all translation units have registered their
modules. The operation supports synchronous, future, detached, and callback
forms. Initialization failures report failed, missing, and dependent modules.
`modules::sprint()` returns a printable view of the registered graph.

## Processes

`libgs::utils::process` and `libgs::utils::wprocess` manage child processes with
a chosen executor. The API supports:

- start, run, join, detach, terminate, kill, and cancel;
- synchronous, timed, callback, and awaitable completion where supported;
- standard input, output, and error stream I/O;
- working-directory and environment overrides;
- process state, PID, and exit-code inspection; and
- single-instance lock files through `set_single`.

Check `sys_expected<T>` and error-code results for platform failures. Keep the
process object alive while asynchronous stream operations are outstanding.
`cancel()` stops pending waits and stream operations. With `cancel_option::none`
the child keeps running and remains joinable. `terminate` and `kill` signal the
child and release join ownership to the background reaper; `detach` releases
ownership without signalling the child.

## Soft bus

Sbus means **soft bus**. Its publish/subscribe and cache APIs are independent of
the transport. LibGS supplies `local_interface` as the default in-process
transport:

- `sbus::publish` sends raw, string, or supported typed payloads;
- `sbus::local_subscriber` subscribes by topic or to all topics;
- subscription callbacks run through the subscriber's executor;
- subscriptions can be cancelled by topic, ID, or subscriber; and
- `sbus::local_cache` keeps the latest topic values, emits changes, and can wait
  for a change.

Payload types can declare a stable topic with `LIBGS_UTILS_SBUS_TYPE` or the
metadata variants.

`basic_subscriber<Interface>`, `cache<Subscriber>`, and `publish<Interface>` are
transport-extensible. An interface provides static `publish`, topic and global
`subscribe`, and cancellation by topic, subscription ID, or all subscriptions.
Its optional `init()` method is called when a subscriber is created. For
example, a DDS adapter can extend the same API to inter-process or distributed
delivery:

```cpp
namespace sbus = libgs::utils::sbus;

using dds_subscriber = sbus::basic_subscriber<dds_interface>;
using dds_cache = sbus::cache<dds_subscriber>;

dds_subscriber subscriber;
subscriber.subscribe("sensor.state", on_state);
sbus::publish<dds_interface>("sensor.state", state);
```

DDS is an integration example, not a bundled dependency. The repository ships
only the in-process `local_interface` transport.

## Utility thread pool

`libgs::utils::thread_pool()` returns the utility module's shared
`asio::thread_pool`. Prefer an explicitly owned executor when task isolation or
shutdown ordering is important.

## Related examples

- [`examples/utils/logger.cpp`](../../examples/utils/logger.cpp)
- [`examples/utils/settings.cpp`](../../examples/utils/settings.cpp)
- [`examples/utils/signal_slot.cpp`](../../examples/utils/signal_slot.cpp)
- [`examples/utils/observer.cpp`](../../examples/utils/observer.cpp)
- [`examples/utils/modules`](../../examples/utils/modules)
- [`examples/utils/process.cpp`](../../examples/utils/process.cpp)
- [`examples/utils/soft_bus_local.cpp`](../../examples/utils/soft_bus_local.cpp)
- [`examples/utils/soft_bus_transport.cpp`](../../examples/utils/soft_bus_transport.cpp)
