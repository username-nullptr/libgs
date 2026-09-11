# LibGS Documentation

Language: English | [Simplified Chinese](../zh_CN/README.md)

LibGS is a modular C++20 foundation for asynchronous applications and services.
The project is organized in layers: a core execution and utility layer,
coroutine-oriented synchronization, protocol modules, and higher-level
application utilities.

HTTP/HTTPS is an important implemented module, but it is not the boundary of
the project. The protocol layer is intended to expand over time; WebSocket
support is currently under development, with protocol and basic stream capabilities available.

## Documentation map

| Document | Contents |
| --- | --- |
| [Getting started](getting-started.md) | Requirements, build options, installation, linking, and API conventions |
| [Core](core.md) | Event loop, scheduling, values, INI files, algorithms, containers, and system helpers |
| [Coroutines](coroutines.md) | Coroutine tasks, synchronization primitives, waiting, and executor switching |
| [HTTP](http.md) | HTTP client, server, routing, sessions, TLS, gzip, and completion styles |
| [Utilities](utilities.md) | Logging, settings, signals, observers, modules, processes, and the extensible soft bus |
| [Roadmap](roadmap.md) | Implemented scope, planned WebSocket support, and project direction |

## Module map

| CMake target | Main headers | Role |
| --- | --- | --- |
| `gs.core` | `<libgs/core.h>` and `<libgs/core/...>` | Runtime and general-purpose foundation |
| `gs.core` | `<libgs/coro.h>` and `<libgs/coro/...>` | Header-only coroutine facilities backed by the core runtime |
| `gs.http` | `<libgs/http.h>` and `<libgs/http/...>` | HTTP protocol, client, server, and optional TLS support |
| `gs.utils` | `<libgs/utils.h>` and `<libgs/utils/...>` | Reusable application services and utilities |

`<libgs.h>` includes the four top-level module headers. Individual headers are
recommended when compile time and dependency boundaries matter. Some utility
APIs, including process and signal/observer types, are exposed through their
individual headers rather than the `<libgs/utils.h>` umbrella.

## Common API conventions

### Executors and scheduling

The default runtime owns a process-wide `asio::io_context`, exposed by
`libgs::io_context()` and `libgs::get_executor()`. Many types can also be
constructed with a compatible external executor. Work can therefore stay on
the default event loop or be directed to an application-owned execution
context.

### Completion styles

Asynchronous operations generally follow Asio completion-token conventions.
Depending on the operation, callers can use:

- a callback;
- `libgs::use_awaitable` from a C++20 coroutine;
- `libgs::detached` for fire-and-forget work;
- an error-code token for non-throwing synchronous handling; or
- the synchronous default offered by selected client and utility APIs.

Check the declaration of a specific operation for its accepted tokens and
completion signature.

### Errors

Throwing synchronous overloads report I/O failures with `std::system_error`.
Non-throwing variants accept an error-code token or return `sys_expected<T>`.
Asynchronous callbacks receive an error code as part of their completion
signature.

### API status

The project has not reached a 1.0 API
stability milestone, so public interfaces may still evolve. Implemented and
planned features are kept separate throughout these documents.

## Source references

- Public headers: [`libgs/`](../../libgs)
- Examples: [`examples/`](../../examples)
- Build configuration: [`CMakeLists.txt`](../../CMakeLists.txt)
- Third-party sources: [`3rd_party/SOURCE.txt`](../../3rd_party/SOURCE.txt)
