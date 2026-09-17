# 核心模块

语言：[English](../en/core.md) | 简体中文

`gs.core` 提供所有其他模块共用的运行时和基础设施。

## 公共结构

| 头文件区域 | 功能 |
| --- | --- |
| `<libgs/core/execution.h>` | 默认上下文、executor、调度、定时器、休眠与异步工作 |
| `<libgs/core/value.h>`、`<libgs/core/container.h>` | 带类型转换的字符串值与参数容器 |
| `<libgs/core/url.h>`、`<libgs/core/ini.h>` | URL 处理与 INI 持久化 |
| `<libgs/core/args_parser.h>` | 命令行分组、选项、帮助与版本处理 |
| `<libgs/core/algorithm.h>` | UUID、SHA-1、通配符匹配、编码与数学工具 |
| `<libgs/core/lock_free_queue.h>`、`<libgs/core/shared_mutex.h>` | 线程级队列与锁 |
| `<libgs/core/system.h>` | 应用路径、CPU 信息、环境与动态库 |
| `<libgs/core/mime_type.h>` | MIME 查询与文本/二进制判断 |
| `<libgs/core/cxx/...>` | 兼容类型、concept、trait、格式化与 expected/optional 支持 |

`<libgs/core.h>` 聚合常用 Core 功能。执行功能及部分专用功能应直接包含对应
头文件。

## 执行运行时

默认运行时持有一个进程级 `asio::io_context`：

| API | 作用 |
| --- | --- |
| `io_context()` | 返回默认上下文 |
| `get_executor()` | 返回默认 executor |
| `exec()` | 运行上下文并阻塞 |
| `exit(code)` | 停止上下文并设置返回码 |
| `dispatch()` | 上下文兼容时立即执行，否则入队 |
| `post()` | 提交立即或延时任务 |
| `start_timer()` | 启动周期任务并返回取消函数 |
| `sleep_for()`、`sleep_until()` | 同步或基于 token 的等待 |

```cpp
#include <libgs/core/execution.h>

#include <chrono>
#include <utility>

int main()
{
    using namespace std::chrono_literals;

    auto timer = libgs::start_timer(250ms, []
    {
        // 在默认上下文中周期执行。
    });

    libgs::post(1s, [timer = std::move(timer)]() mutable
    {
        timer();
        libgs::exit();
    });

    return libgs::exec();
}
```

调度函数与许多高层类型也可接受外部 Asio executor。捕获的对象与借用的缓冲区
必须存活到异步操作完成。Associated executor、非内联完成、strand 与共享对象规则
见 [Asio 兼容的 I/O 模型](io-model.md)。

## 数据与配置

- `value` 保存文本，并提供数值等类型的受检转换。
- `url` 解析层次化 URL、查询参数与相对引用；具体协议模块决定允许的 scheme。
- `ini` 提供分组/键访问以及同步或 completion-token 形式的持久化。
- `cmdline::args_parser` 支持别名、带值选项、组合 flag、帮助、版本和未匹配参数。
- 字符串容器别名覆盖 list、vector、set 与 deque，并保持字符类型一致。

## 算法、同步与系统 API

算法头文件提供 UUID v4/v5/v6/v7、SHA-1、百分号编码、通配符匹配与算术工具。
SHA-1 适合协议兼容，不应作为新密码或签名方案。

Core 的锁和无锁队列是线程同步工具。需要挂起等待而不阻塞 executor 线程时，
使用[协程](coroutines.md)原语。

`libgs::app` 提供可执行文件路径、工作目录、环境变量、用户/主目录与绝对路径
工具。`libgs::library` 加载动态库与符号；动态库对象必须比从中取得的符号存活
更久。平台错误通常通过 `sys_expected<T>` 返回。

## 示例

- [执行运行时](../../examples/core/execution.cpp)
- [Value](../../examples/core/value.cpp)
- [INI](../../examples/core/ini.cpp)
- [算法](../../examples/core/algorithms.cpp)
- [无锁队列](../../examples/core/lock_free_queue.cpp)
- [应用路径](../../examples/core/app_paths.cpp)
- [参数解析](../../examples/core/args_parser.cpp)
- [动态库](../../examples/core/dynamic_library.cpp)
