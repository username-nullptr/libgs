# 协程支持

语言：[English](../en/coroutines.md) | 简体中文

LibGS 基于 standalone Asio 和 Core 运行时提供适合协程使用的调度与同步功能。
通过 `<libgs/coro.h>` 引入，并将应用链接到 `gs.coro`；该目标会公开链接
`gs.core`。

## 启动协程

`libgs::dispatch` 和 `libgs::post` 可以接收返回 `libgs::awaitable<T>` 的 callable：

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

时长字面量 `_y`、`_mon`、`_d`、`_h`、`_min`、`_s`、`_ms`、`_us` 和 `_ns`
会生成可等待的延时操作。

## 同步原语

| 类型 | 用途 |
| --- | --- |
| `libgs::coro::mutex` | 支持 try 和超时操作的协程独占锁 |
| `libgs::coro::unique_lock` | 可移动的协程互斥量 RAII 所有权包装器 |
| `libgs::coro::shared_mutex` | 支持独占或共享锁定的协程互斥量 |
| `libgs::coro::shared_lock` | 共享锁的 RAII 所有权包装器 |
| `libgs::coro::semaphore` | 最大值默认为 `size_t` 上限的计数信号量 |
| `libgs::coro::binary_semaphore` | 最大计数为一的信号量 |
| `libgs::coro::condition_variable` | 支持谓词、超时、单个通知和全部通知的等待 |

这些原语会挂起等待中的协程，而不是有意阻塞执行器线程。

### Mutex 示例

```cpp
#include <libgs/coro.h>

libgs::coro::mutex gate;

libgs::awaitable<void> update_shared_state()
{
    libgs::coro::unique_lock lock(gate);
    co_await lock.lock();

    // 在 lock 持有 gate 时访问共享状态。
}
```

当 lock 包装器离开作用域时，它会释放已经获得的锁。`try_lock_for` 和
`try_acquire_for` 等超时操作返回可等待的 `bool`。

### 条件变量

条件变量与 `libgs::coro::unique_lock` 一起使用：

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

应在持有对应 mutex 时更新谓词，然后调用 `notify_one()` 或 `notify_all()`。

## 等待外部任务

`libgs::coro::wait` 可以将支持的阻塞式完成对象适配为协程等待。目前的重载包括
`std::future<T>`、`asio::thread_pool` 和 `std::thread`，适合用于集成没有提供
Asio completion token 的现有 API。

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

## 切换执行上下文

`libgs::coro::goto_exec` 会挂起协程，并在兼容的目标执行器上恢复。它返回原先的
执行器，方便调用方切换回来。`libgs::coro::goto_thread` 会将后续任务移动到线程
上下文。

```cpp
#include <libgs/coro.h>

libgs::awaitable<void> move_between_executors(asio::thread_pool &workers)
{
    auto previous = co_await libgs::coro::goto_exec(workers);
    // 后续任务在 worker pool 中运行。

    co_await libgs::coro::goto_exec(previous);
    // 后续任务回到之前的执行器。
}
```

除非已经明确设计好所有权和执行顺序，否则不要在持有协程 mutex 时切换执行器。

## 取消与生命周期

- 在协程结束之前，必须保证被引用对象仍然有效。
- detached 任务应优先捕获有所有权的值。
- 除非 API 明确说明会取得所有权，否则传给异步操作的 buffer 应视为借用。
- 不允许无限等待的操作应使用带超时的形式。
- 只有在未完成任务结束或取消之后，才能关闭执行器。

## 相关示例

- [`examples/coro/basics.cpp`](../../examples/coro/basics.cpp)
- [`examples/coro/mutex.cpp`](../../examples/coro/mutex.cpp)
- [`examples/coro/shared_mutex.cpp`](../../examples/coro/shared_mutex.cpp)
- [`examples/coro/semaphore.cpp`](../../examples/coro/semaphore.cpp)
- [`examples/coro/condition_variable.cpp`](../../examples/coro/condition_variable.cpp)
