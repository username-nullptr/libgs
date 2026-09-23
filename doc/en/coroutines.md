# Coroutines

Language: English | [简体中文](../zh_CN/coroutines.md)

Target: `gs.coro`. Include `<libgs/coro.h>`. The module provides
executor-aware waits and synchronization without intentionally blocking an
executor thread.

## Start work

`libgs::dispatch()` and `libgs::post()` accept callables returning
`libgs::awaitable<T>`:

```cpp
#include <libgs/coro.h>

using namespace libgs::coro::literals;

int main()
{
    libgs::dispatch([]() -> libgs::awaitable<void>
    {
        co_await 250_ms;
        libgs::exit();
    });

    return libgs::exec();
}
```

Delay literals are available from years down to nanoseconds:
`_y`, `_mon`, `_d`, `_h`, `_min`, `_s`, `_ms`, `_us`, and
`_ns`.

## Facilities

| API | Purpose |
| --- | --- |
| `coro::mutex`, `coro::unique_lock` | Exclusive locking and RAII ownership |
| `coro::shared_mutex`, `coro::shared_lock` | Shared/exclusive locking |
| `coro::semaphore`, `coro::binary_semaphore` | Counting and binary permits |
| `coro::condition_variable` | Predicate, timed, notify-one/all waits |
| `coro::wait()` | Await a `std::future`, `std::thread`, `libgs::jthread`, or `asio::thread_pool` |
| `coro::goto_exec()` | Resume on another executor and return the previous one |
| `coro::goto_thread()` | Resume on a worker thread |

Timed lock and semaphore operations return an awaitable `bool`.

## Rules

- A synchronization object must outlive all of its waiters.
- Detached coroutines should own captured state.
- Update a condition-variable predicate while holding its mutex.
- Do not carry a lock across an executor switch unless that ordering is
  intentional.
- Complete or cancel pending work before stopping its executor.

See [Execution and I/O model](io-model.md) for general ownership and concurrency
rules, and [Coroutine examples](../../examples/README.md#coroutines) for runnable
programs.
