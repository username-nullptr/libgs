# 核心模块

语言：[English](../en/core.md) | 简体中文

Target：`gs.core`。Core 始终构建，是其他模块的基础。

## 公共 API

| 头文件 | 主要能力 |
| --- | --- |
| `<libgs/core/execution.h>` | 默认上下文、executor、调度、定时器、等待、异步任务 |
| `<libgs/core/value.h>`、`container.h` | 带类型转换的文本值与参数容器 |
| `<libgs/core/url.h>`、`ini.h` | URL 解析/合并与 INI 持久化 |
| `<libgs/core/args_parser.h>` | 命令行分组、选项、标志、帮助与版本 |
| `<libgs/core/algorithm.h>` | UUID、SHA-1、通配、编码与数学辅助 |
| `<libgs/core/lock_free_queue.h>` | 链式与环形无锁队列 |
| `<libgs/core/atomic_mutex.h>`、`shared_mutex.h` | 原子独占锁与共享锁 |
| `<libgs/core/jthread.h>` | 可移植的 C++20 自动汇合线程与停止令牌 |
| `<libgs/core/system.h>` | 应用路径、环境、CPU、动态库 |
| `<libgs/core/mime_type.h>` | MIME 查询与文本/二进制判断 |
| `<libgs/core/cxx/...>` | Concepts、traits、格式化、expected/optional 兼容层 |

`<libgs/core.h>` 聚合常用的数据、算法、同步和系统头文件。Execution、value、
MIME、queue 等专用能力需要直接包含对应头文件。

## 默认运行时

```cpp
#include <libgs/core/execution.h>

#include <chrono>

int main()
{
    using namespace std::chrono_literals;

    libgs::post(1s, [] { libgs::exit(); });
    return libgs::exec();
}
```

| API | 作用 |
| --- | --- |
| `io_context()`、`get_executor()` | 访问进程级默认运行时 |
| `exec()`、`exit(code)` | 运行和停止默认运行时 |
| `dispatch()`、`post()` | 提交立即或延迟任务 |
| `start_timer()` | 启动周期任务并返回取消函数 |
| `sleep_for()`、`sleep_until()` | 同步或 completion-token 等待 |

调度函数也接受外部 Asio executor。所有权和 strand 规则见
[执行与 I/O 模型](io-model.md)。

## 数据、文件与系统能力

- `value` 保存文本并执行受检查的类型转换。
- `url` 解析层次 URL、查询参数和相对引用；协议模块决定接受哪些 scheme。
- `ini` 提供 group/key 访问，以及同步和 completion-token 文件操作。
- `cmdline::args_parser` 处理别名、值选项、组合标志、位置参数、帮助与版本。
- `libgs::app` 提供路径、工作目录、环境和用户/主目录能力。
- `libgs::library` 加载动态库与符号；动态库对象必须比取得的符号存活更久。

线程同步使用 Core 的锁和队列；需要挂起协程时使用[协程](coroutines.md)同步原语。
`atomic_mutex` 与 `atomic_shared_mutex` 默认使用 balanced 策略。low-latency
策略会持续自旋，只适合受控线程上短小且有界的临界区。

标准库支持时，`libgs::jthread`、`stop_token`、`stop_source` 和
`stop_callback` 直接映射到对应的标准库类型；否则 Core 基于 `std::thread`
提供相同的协作停止与析构自动汇合接口。

## 示例

[Core 示例索引](../../examples/README.md#core)包含执行、value、INI、算法、队列、
路径、参数解析和动态库程序。
