# Coroutines

Language: English | [简体中文](../zh_CN/coroutines.md)

Include `<libgs/coro.h>` and link `gs.coro`. The module builds non-blocking
waits and synchronization on Asio executors.

## Start a coroutine

`libgs::dispatch()` and `libgs::post()` accept callables that return
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

The `_y`, `_mon`, `_d`, `_h`, `_min`, `_s`, `_ms`, `_us`, and `_ns` literals
produce awaitable delays.

## Facilities

| API | Purpose |
| --- | --- |
| `coro::mutex`, `coro::unique_lock` | Exclusive lock and RAII ownership |
| `coro::shared_mutex`, `coro::shared_lock` | Exclusive/shared locking |
| `coro::semaphore`, `coro::binary_semaphore` | Counting and binary permits |
| `coro::condition_variable` | Predicate, timed, one/all notification waits |
| `coro::wait()` | Await `std::future`, `std::thread`, or `asio::thread_pool` completion |
| `coro::goto_exec()` | Resume on another executor and return the previous one |
| `coro::goto_thread()` | Resume on a worker thread |

Locking and waiting operations suspend the coroutine rather than intentionally
blocking its executor thread. Timed lock and semaphore operations return an
awaitable `bool`.

```cpp
libgs::coro::mutex gate;

libgs::awaitable<void> update()
{
    libgs::coro::unique_lock lock(gate);
    co_await lock.lock();
    // Protected work.
}
```

## Usage rules

- Keep referenced state alive until the coroutine completes.
- Prefer owned captures for detached coroutines.
- Update a condition-variable predicate while holding its mutex.
- Avoid carrying a lock across an executor switch unless the ordering is
  deliberate.
- Complete or cancel outstanding work before shutting down its executor.

## Examples

- [Coroutine basics](../../examples/coro/basics.cpp)
- [Mutex](../../examples/coro/mutex.cpp)
- [Shared mutex](../../examples/coro/shared_mutex.cpp)
- [Semaphore](../../examples/coro/semaphore.cpp)
- [Condition variable](../../examples/coro/condition_variable.cpp)
