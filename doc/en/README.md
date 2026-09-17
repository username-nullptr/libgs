# LibGS Documentation

Language: English | [简体中文](../zh_CN/README.md)

## Start here

| Document | Use it for |
| --- | --- |
| [Getting started](getting-started.md) | Requirements, configuration, building, installation, and linking |
| [Asio-compatible I/O model](io-model.md) | Executors, completion handlers, strands, cancellation, and per-object concurrency |
| [Core](core.md) | Runtime, scheduling, data types, algorithms, containers, and system APIs |
| [Coroutines](coroutines.md) | Awaitable waits and non-blocking synchronization |
| [HTTP](http.md) | HTTP protocol, client, server, files, sessions, TLS, gzip, and proxies |
| [WebSocket](websocket.md) | RFC 6455 client, server, stream, Upgrade, compression, and proxies |
| [Utilities](utilities.md) | Logging, settings, signals, observers, modules, processes, and soft bus |
| [Roadmap](roadmap.md) | Current boundaries and planned areas |

The [examples guide](../../examples/README.md) maps each feature to a small
executable. Use the public headers in [`libgs/`](../../libgs) as the API source
of truth.

## Module structure

| Module | Public dependency |
| --- | --- |
| `gs.core` | — |
| `gs.coro` | `gs.core` |
| `gs.http` | `gs.coro` |
| `gs.websocket` | `gs.http` |
| `gs.utils` | `gs.coro` |

Each module has an umbrella header, but narrower headers are preferred when an
application uses only one facility. `<libgs.h>` follows the module switches in
the generated configuration header.

## API conventions

- Most asynchronous operations follow Asio completion-token conventions.
- `libgs::use_awaitable` selects a coroutine result; callbacks and
  `libgs::detached` are accepted where the declaration permits them.
- Selected APIs default to synchronous execution. Their throwing form reports
  I/O failures with `std::system_error`; error-code overloads are non-throwing.
- Asynchronous I/O normally borrows buffers until completion unless the API
  explicitly states that it takes a copy.
- Types that accept an executor can run on an application-owned context instead
  of the process-wide default context.

LibGS is pre-1.0; public interfaces may still change.
