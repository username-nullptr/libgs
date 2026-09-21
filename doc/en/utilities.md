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
provide the in-process transport. When `LIBGS_BUILD_UTILITIES_SBUS_UDP` is
enabled, `udp_interface`, `udp_subscriber`, and `udp_cache` provide an Asio UDP
multicast transport. Publish through it explicitly with
`publish<udp_interface>()`.

`LIBGS_UTILS_SBUS_DEFAULT_INTERFACE` selects `local` (the default) or `udp` for
unqualified `publish()` / `subscribe()` calls and the `default_interface` /
`default_subscriber` aliases. Code that requires a particular transport should
use its explicit interface or subscriber type so that changing this build option
does not change its behavior.

The transport enables loopback delivery. `udp_interface::config_t` defaults to
administratively scoped group `239.255.71.83`, port `57183`, and
`msg_range::process`. A process-scoped frame carries a per-process token and
uses multicast TTL 0; `lan` uses TTL 1, and `internet` uses TTL 64. A receiver's
range is hierarchical: it selects the widest publish range that is accepted,
while process-scoped frames from other processes remain rejected. The
`internet` range still requires a multicast-enabled routed network and does not
imply public-Internet availability.

The versioned wire frame supports topics up to 4 KiB and fragmented payloads
up to 16 MiB. Each interface applies global and per-source packet/byte token
buckets before parsing, tracks at most 1024 source addresses, ignores topics
without a matching topic or global subscriber, and limits one source to 8
incomplete messages and 16 MiB of reassembly memory. Incomplete messages are
expired by a periodic five-second timeout. The interface-wide caps are
128 incomplete messages and 64 MiB per interface. Completed messages enter a
bounded 256-message/32-MiB queue and user callbacks run on a separate delivery
thread, so a slow callback cannot stop socket receive processing indefinitely.
`udp_interface::statistics()` reports invalid and rate-limited datagrams,
reassembly evictions, delivery-queue drops, and delivery progress. Endpoint,
range, socket buffers, message-size bounds, rate/burst limits, source tracking,
reassembly limits/timeouts, and delivery-queue bounds are configurable directly
in `config_t`. `set_config()` changes subsequent static publishes and the
default used by subsequently constructed receivers. An existing receiver keeps
the snapshot supplied to its constructor; pass a `config_t` explicitly when
different receiver policies must coexist.

UDP delivery is best effort and overload protection intentionally drops
traffic. It is neither authenticated nor encrypted; use LAN and routed modes
only on an appropriate trusted network and enforce network-level source and
multicast ACLs at untrusted boundaries.
`basic_subscriber<Interface>`, `cache<Subscriber>`, and
`publish<Interface>()` remain available for custom transports.

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
- [UDP soft bus](../../examples/utils/soft_bus_udp.cpp)
- [Custom soft-bus transport](../../examples/utils/soft_bus_transport.cpp)
