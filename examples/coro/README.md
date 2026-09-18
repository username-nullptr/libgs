# Coroutine Examples

[Examples index](../README.md) · [Coroutine guide](../../doc/en/coroutines.md)

Coroutine examples require the default-enabled `gs.coro` module. Every program
is self-contained and runs on local executors.

## Programs

| Program | Demonstrates |
| --- | --- |
| `basics` | Starting an awaitable, waiting for a future, delaying, and moving to a worker thread |
| `mutex` | `coro::mutex` and `coro::unique_lock` across multiple workers |
| `shared_mutex` | Shared readers and an exclusive writer |
| `semaphore` | Limiting concurrent coroutine work |
| `condition_variable` | Waiting for a predicate and notifying a suspended coroutine |

## Run

```sh
./build/output/examples/coro/basics
./build/output/examples/coro/mutex
./build/output/examples/coro/shared_mutex
./build/output/examples/coro/semaphore
./build/output/examples/coro/condition_variable
```

These examples use the process-wide default context and call `libgs::exit()`
after their work completes. The synchronization objects remain alive until all
waiters have finished.
