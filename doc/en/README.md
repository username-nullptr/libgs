# LibGS Documentation

Language: English | [简体中文](../zh_CN/README.md)

These pages describe the current public interface. Start with the task you need;
module pages can then be used as focused API guides.

## Start here

| Task | Document |
| --- | --- |
| Build and run a first program | [Getting started](getting-started.md) |
| Select modules, features, and dependencies | [Build and configuration](build.md) |
| Use executors, completion tokens, cancellation, and strands safely | [Execution and I/O model](io-model.md) |
| Check protocol, platform, and integration boundaries | [Support matrix](support.md) |

## Module guides

| Module | Target | Guide | Main header |
| --- | --- | --- | --- |
| Core | `gs.core` | [Runtime and common facilities](core.md) | `<libgs/core.h>` |
| Coroutines | `gs.coro` | [Awaitable synchronization](coroutines.md) | `<libgs/coro.h>` |
| HTTP | `gs.http` | [HTTP/1.x client and server](http.md) | `<libgs/http.h>` |
| WebSocket | `gs.websocket` | [RFC 6455 client, server, and stream](websocket.md) | `<libgs/websocket.h>` |
| Utilities | `gs.utils` | [Application services](utilities.md) | `<libgs/utils.h>` |

The dependency direction is:

```text
gs.core
├── gs.coro
│   ├── gs.http
│   │   └── gs.websocket
│   └── gs.utils
```

Link the highest module an application uses; public dependencies propagate when
LibGS is consumed through `add_subdirectory`.

## Working code and verification

- The [examples guide](../../examples/README.md) maps each capability to a
  buildable program.
- The [test guide](../../test/README.md) covers functional, stress, fuzz,
  performance, and sanitizer builds.
- Public headers in [`libgs/`](../../libgs) are the source of truth for exact
  signatures and overload availability.

## Common API conventions

- Asynchronous operations use Asio completion-token conventions.
- Pass `libgs::use_awaitable` for coroutine results. Callbacks and
  `libgs::detached` are available where the declaration accepts them.
- APIs with a synchronous form report failure either by `std::system_error` or
  through an explicit `std::error_code&` overload.
- Buffers and other borrowed arguments normally remain owned by the caller until
  completion.
- Executor-aware objects can use an application-owned context instead of the
  process-wide default context.

See [Execution and I/O model](io-model.md) for the full lifetime and concurrency
rules.
