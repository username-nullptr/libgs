# Coroutine Support

Language: English | [Simplified Chinese](../zh_CN/coroutines.md)

LibGS builds coroutine-friendly scheduling and synchronization on top of
standalone Asio and the core runtime. Include `<libgs/coro.h>` and link the
application with `gs.coro`; the target publicly links `gs.core`.

## Launch a coroutine

`libgs::dispatch` and `libgs::post` accept callables returning
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

The duration literals `_y`, `_mon`, `_d`, `_h`, `_min`, `_s`, `_ms`, `_us`,
and `_ns` produce awaitable delays.

## Synchronization primitives

| Type | Purpose |
| --- | --- |
| `libgs::coro::mutex` | Exclusive coroutine lock with try and timed-lock operations |
| `libgs::coro::unique_lock` | Movable RAII ownership wrapper for a coroutine mutex |
| `libgs::coro::shared_mutex` | Exclusive or shared coroutine locking |
| `libgs::coro::shared_lock` | RAII shared ownership wrapper |
| `libgs::coro::semaphore` | Counting semaphore whose maximum defaults to the `size_t` limit |
| `libgs::coro::binary_semaphore` | Semaphore with a maximum count of one |
| `libgs::coro::condition_variable` | Predicate, timed, single-notify, and notify-all waiting |

These primitives suspend the waiting coroutine instead of intentionally
blocking the executor thread.

### Mutex example

```cpp
#include <libgs/coro.h>

libgs::coro::mutex gate;

libgs::awaitable<void> update_shared_state()
{
    libgs::coro::unique_lock lock(gate);
    co_await lock.lock();

    // Access shared state while lock owns gate.
}
```

The lock wrapper releases an owned lock when it leaves scope. Timed operations
such as `try_lock_for` and `try_acquire_for` return an awaitable `bool`.

### Condition variables

A condition variable works with `libgs::coro::unique_lock`:

```cpp
#include <libgs/coro.h>

libgs::coro::mutex gate;
libgs::coro::condition_variable ready_changed;
bool ready = false;

libgs::awaitable<void> wait_until_ready()
{
    libgs::coro::unique_lock lock(gate);
    co_await lock.lock();
    co_await ready_changed.wait(lock, [] { return ready; });
}
```

Update the predicate while holding the associated mutex, then call
`notify_one()` or `notify_all()`.

## Waiting for external work

`libgs::coro::wait` adapts supported blocking completion objects to a coroutine.
Current overloads include `std::future<T>`, `asio::thread_pool`, and
`std::thread`. This is useful when integrating existing APIs that do not expose
an Asio completion token.

```cpp
#include <libgs/coro.h>

#include <future>

libgs::awaitable<int> wait_for_answer()
{
    auto future = std::async(std::launch::async, []
    {
        return 42;
    });

    co_return co_await libgs::coro::wait(future);
}
```

## Switch execution contexts

`libgs::coro::goto_exec` suspends the coroutine and resumes it on a compatible
target executor. It returns the previous executor so the caller can switch
back. `libgs::coro::goto_thread` moves continuation work to a thread context.

```cpp
#include <libgs/coro.h>

libgs::awaitable<void> move_between_executors(asio::thread_pool &workers)
{
    auto previous = co_await libgs::coro::goto_exec(workers);
    // Work now continues on the worker pool.

    co_await libgs::coro::goto_exec(previous);
    // Work continues on the earlier executor.
}
```

Avoid holding a coroutine mutex across an executor switch unless that ownership
and ordering are deliberate.

## Cancellation and lifetime

- Keep referenced objects alive until the coroutine finishes.
- Prefer owned values for detached work.
- Treat buffers passed to asynchronous operations as borrowed unless that API
  explicitly documents ownership.
- Use timed waits when an operation must not wait indefinitely.
- Shut down executors only after outstanding work has completed or been
  cancelled.

## Related examples

- [`examples/coro/basics.cpp`](../../examples/coro/basics.cpp)
- [`examples/coro/mutex.cpp`](../../examples/coro/mutex.cpp)
- [`examples/coro/shared_mutex.cpp`](../../examples/coro/shared_mutex.cpp)
- [`examples/coro/semaphore.cpp`](../../examples/coro/semaphore.cpp)
- [`examples/coro/condition_variable.cpp`](../../examples/coro/condition_variable.cpp)
