# LibGS Documentation

Language: English | [简体中文](../zh_CN/README.md)

## Start here

| Need | Guide |
| --- | --- |
| Build, select modules, or integrate LibGS | [Build and configuration](build.md) |
| Understand executors, completion tokens, lifetimes, and concurrency | [Execution and I/O model](io-model.md) |
| Find a runnable program | [Examples](../../examples/README.md) |
| Build or select tests | [Tests](../../test/README.md) |

## Modules

| Module | Target | Umbrella header | Guide |
| --- | --- | --- | --- |
| Core | `gs.core` | `<libgs/core.h>` | [Core](core.md) |
| Coroutines | `gs.coro` | `<libgs/coro.h>` | [Coroutines](coroutines.md) |
| HTTP | `gs.http` | `<libgs/http.h>` | [HTTP](http.md) |
| WebSocket | `gs.websocket` | `<libgs/websocket.h>` | [WebSocket](websocket.md) |
| Utilities | `gs.utils` | `<libgs/utils.h>` | [Utilities](utilities.md) |

Dependency direction:

```text
gs.core
└── gs.coro
    ├── gs.http
    │   └── gs.websocket
    └── gs.utils
```

Link only the highest module used directly; its public dependencies propagate
through CMake.

## API conventions

- Asynchronous APIs use Asio completion tokens. `libgs::use_awaitable` selects
  coroutine completion where the declaration permits it.
- Synchronous APIs report errors as declared: by `std::system_error`, an
  explicit `std::error_code&`, or an expected-style result.
- Borrowed buffers and referenced state must outlive asynchronous completion.
- Stateful I/O objects are not implicitly thread-safe. Serialize access to one
  object, normally with a strand.
- Public headers under [`libgs/`](../../libgs) define the exact API surface.
