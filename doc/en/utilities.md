# Utilities

Language: English | [简体中文](../zh_CN/utilities.md)

Target: `gs.utils`. Enable `LIBGS_BUILD_UTILITIES`, include
`<libgs/utils.h>`, and link `gs.utils`. The module depends on `gs.coro`.

## Public API map

| Header | Facility |
| --- | --- |
| `<libgs/utils/logger.h>` | Named console and rotating-file loggers |
| `<libgs/utils/settings.h>` | Named INI-backed settings and change signals |
| `<libgs/utils/signal_slot.h>` | Synchronous, asynchronous, and backpressure signals |
| `<libgs/utils/observer.h>` | ID-addressed callbacks dispatched by executor |
| `<libgs/utils/modules.h>` | Dependency-ordered application initialization |
| `<libgs/utils/process.h>` | Child lifecycle and standard-stream I/O |
| `<libgs/utils/sbus.h>` | Extensible publish/subscribe and cached topic state |

`<libgs/utils.h>` aggregates logger, settings, modules, and soft bus. Include
process, signal/slot, and observer headers directly.

## Logging and settings

`utils::logger` manages named console and file loggers. Configuration covers
paths, levels, rotation, timestamps, and formatting. File sinks are
asynchronous; call `logger::flush()` when persistence must be observed.

`utils::settings` wraps `libgs::ini` as a named instance and emits
`changed` and `loaded`. Use `get()`/`set()` for values and `ini()` for
direct persistence control.

## Signals, observers, and modules

`utils::signal<Signature>` supports:

| Mode | Delivery |
| --- | --- |
| `sync` | Invoke slots in the caller |
| `async` | Queue slots on an executor |
| `backpressure` | Queue on an executor and block until delivery |

Do not use backpressure from the same executor thread that must perform the
delivery. Value arguments are owned for dispatch; views, pointers, references,
and slot captures remain the caller's responsibility.

`utils::observer` routes indexed callbacks by stable object ID and unregisters
on destruction. `utils::modules` registers named initializers, orders them by
dependencies, and supports synchronous or asynchronous initialization.

## Processes

`utils::process` and `utils::wprocess` provide start/run, join, detach,
terminate, kill, cancellation, timeouts, working directory/environment,
single-instance locks, and stdin/stdout/stderr I/O.

Keep the process alive while I/O is pending. Serialize lifecycle calls with I/O
initiation, and keep at most one operation active per stream direction.

## Soft bus

The soft bus separates the typed publish/subscribe/cache API from transports:

| Transport | Types | Availability |
| --- | --- | --- |
| In-process | `local_interface`, `local_subscriber`, `local_cache` | Always |
| UDP multicast | `udp_interface`, `udp_subscriber`, `udp_cache` | `LIBGS_BUILD_UTILITIES_SBUS_UDP=ON` |
| Custom | `basic_subscriber<Interface>`, `cache<Subscriber>`, `publish<Interface>()` | User implementation |

`LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` selects `local` (default) or `udp` for
unqualified APIs. Code that requires a transport should name it explicitly.

`cache::wait_changed()` uses the common I/O completion-token model and waits for
the next change after initiation. It is edge-triggered: earlier changes are not
replayed. Use `changed()` for a persistent subscription. `cache::cancel()`
cancels current waits without disconnecting persistent subscriptions or
affecting waits initiated afterwards.

The UDP transport is versioned, fragmented, rate-limited, and bounded in source
tracking, reassembly, and callback delivery. It supports process, LAN, and
routed multicast scopes. Delivery is best effort: overload protection may drop
traffic, and the transport does not provide authentication, encryption, or
reliable replay. Configure network policy and ACLs outside LibGS.

`utils::thread_pool()` returns the module's shared Asio thread pool. Use an
owned executor when isolation or shutdown order matters.

General ownership and concurrency rules are in
[Execution and I/O model](io-model.md).

## Examples

The [Utilities example index](../../examples/README.md#utilities) lists logging,
settings, signals, observers, modules, processes, local/UDP bus, and a custom
transport.
