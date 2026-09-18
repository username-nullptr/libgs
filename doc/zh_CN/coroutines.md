# 协程

语言：[English](../en/coroutines.md) | 简体中文

包含 `<libgs/coro.h>` 并链接 `gs.coro`。该模块基于 Asio executor 提供
非阻塞等待与同步原语。

Executor 与生命周期行为遵循[执行与 I/O 模型](io-model.md)。同步原语支持
并发 wait/notify，但对象必须比所有等待者存活更久。

## 启动协程

`libgs::dispatch()` 和 `libgs::post()` 可接收返回 `libgs::awaitable<T>`
的可调用对象：

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

`_y`、`_mon`、`_d`、`_h`、`_min`、`_s`、`_ms`、`_us` 与 `_ns`
字面量会生成可等待的延时。

## 功能

| API | 作用 |
| --- | --- |
| `coro::mutex`、`coro::unique_lock` | 互斥锁与 RAII 所有权 |
| `coro::shared_mutex`、`coro::shared_lock` | 独占/共享锁 |
| `coro::semaphore`、`coro::binary_semaphore` | 计数/二元信号量 |
| `coro::condition_variable` | 条件、超时、单个/全部通知等待 |
| `coro::wait()` | 等待 `std::future`、`std::thread` 或 `asio::thread_pool` |
| `coro::goto_exec()` | 切换 executor，并返回原 executor |
| `coro::goto_thread()` | 切换到工作线程继续执行 |

锁与等待操作会挂起协程，而不是主动阻塞 executor 线程。带超时的锁和信号量
操作返回可等待的 `bool`。

```cpp
libgs::coro::mutex gate;

libgs::awaitable<void> update()
{
    libgs::coro::unique_lock lock(gate);
    co_await lock.lock();
    // 受保护的操作。
}
```

## 使用规则

- 被引用的状态必须存活到协程结束。
- 分离运行的协程应优先捕获自有数据。
- 修改条件变量的谓词时应持有关联 mutex。
- 除非明确需要该顺序，否则不要持锁切换 executor。
- 关闭 executor 前先完成或取消未结束的任务。

## 示例

- [协程基础](../../examples/coro/basics.cpp)
- [Mutex](../../examples/coro/mutex.cpp)
- [Shared mutex](../../examples/coro/shared_mutex.cpp)
- [Semaphore](../../examples/coro/semaphore.cpp)
- [Condition variable](../../examples/coro/condition_variable.cpp)
