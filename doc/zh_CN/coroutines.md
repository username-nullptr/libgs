# 协程

语言：[English](../en/coroutines.md) | 简体中文

Target：`gs.coro`。包含 `<libgs/coro.h>`。该模块提供感知 executor 的等待和
同步，并通过挂起协程避免主动阻塞 executor 线程。

## 启动任务

`libgs::dispatch()` 与 `libgs::post()` 接受返回
`libgs::awaitable<T>` 的可调用对象：

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

延迟字面量从年到纳秒：`_y`、`_mon`、`_d`、`_h`、`_min`、
`_s`、`_ms`、`_us`、`_ns`。

## 功能

| API | 作用 |
| --- | --- |
| `coro::mutex`、`coro::unique_lock` | 独占锁与 RAII 所有权 |
| `coro::shared_mutex`、`coro::shared_lock` | 共享/独占锁 |
| `coro::semaphore`、`coro::binary_semaphore` | 计数与二元许可 |
| `coro::condition_variable` | 谓词、超时、单个/全部通知等待 |
| `coro::wait()` | 等待 `std::future`、`std::thread`、`libgs::jthread` 或 `asio::thread_pool` |
| `coro::goto_exec()` | 在另一 executor 上恢复，并返回原 executor |
| `coro::goto_thread()` | 在工作线程上恢复 |

带超时的锁和 semaphore 操作返回 awaitable `bool`。

## 规则

- 同步对象必须比它的所有等待者存活更久。
- Detached 协程应持有捕获状态。
- 修改 condition-variable 谓词时应持有对应互斥锁。
- 除非有明确的顺序设计，不要持锁切换 executor。
- 停止 executor 前，完成或取消未结束的任务。

通用所有权和并发规则见[执行与 I/O 模型](io-model.md)，可运行程序见
[协程示例](../../examples/README.md#coroutines)。
