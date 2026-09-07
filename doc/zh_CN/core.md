# 核心运行时与通用设施

语言：[English](../en/core.md) | 简体中文

Core 模块提供所有其他 LibGS 模块共用的执行模型和通用设施。链接时使用
`gs.core`。

## 头文件索引

| 头文件 | 主要功能 |
| --- | --- |
| `<libgs/core/execution.h>` | 默认事件循环、dispatch、post、定时器、休眠和完成操作初始化 |
| `<libgs/core/value.h>` | 基于字符串存储的类型化 value 与转换 |
| `<libgs/core/url.h>` | 协议无关的层次化 URL 解析、查询参数和相对引用解析 |
| `<libgs/core/ini.h>` | 支持同步和异步持久化的内存 INI 数据 |
| `<libgs/core/args_parser.h>` | 命令行选项组、标志、版本和帮助处理 |
| `<libgs/core/algorithm.h>` | UUID、SHA-1、通配符匹配、百分号编码和数学工具 |
| `<libgs/core/lock_free_queue.h>` | 链式和环形无锁队列 |
| `<libgs/core/shared_mutex.h>` | 标准共享互斥量和自旋共享互斥量别名 |
| `<libgs/core/system.h>` | 应用路径、环境变量、动态库和 CPU 工具 |
| `<libgs/core/mime_type.h>` | MIME 查询、二进制/文本检测和文本编码检查 |
| `<libgs/core/string_list.h>` 及相关头文件 | 面向字符类型的字符串 list、set、vector 和 deque |

`<libgs/core.h>` 是常用 Core API 的聚合头。更专门的功能可以直接包含其具体
头文件。

## 默认执行运行时

对于没有自行提供执行器的应用，LibGS 提供一个默认 `asio::io_context`：

- `libgs::io_context()` 返回该 context；
- `libgs::get_executor()` 返回其执行器；
- `libgs::exec()` 运行事件循环并阻塞；
- `libgs::exit(code)` 请求退出并设置返回码；
- `libgs::is_run()` 返回默认事件循环是否正在运行。

主要调度操作如下：

| 操作 | 行为 |
| --- | --- |
| `dispatch` | 已在目标上下文中时立即执行，否则将任务排队 |
| `post` | 始终将任务加入目标上下文队列 |
| 延迟 `post` | 在相对或绝对时间执行任务，并返回取消函数 |
| `start_timer` | 启动重复任务并返回取消函数 |
| `local_dispatch` | 通过临时的本地执行上下文运行任务 |
| `sleep_for`、`sleep_until` | 根据重载和 token 进行同步或异步等待 |

```cpp
#include <libgs/core/execution.h>

#include <chrono>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;

    libgs::dispatch([]
    {
        std::cout << "dispatched\n";
    });

    auto cancel_timer = libgs::start_timer(250ms, []
    {
        std::cout << "tick\n";
    });

    libgs::post(1s, [cancel_timer = std::move(cancel_timer)]() mutable
    {
        cancel_timer();
        libgs::exit();
    });

    return libgs::exec();
}
```

异步任务完成之前，必须保证捕获的数据仍然有效。对于 buffer 和 detached 操作
捕获的引用尤其需要注意这一点。

## 外部执行器

很多调度函数的第一个参数可以是兼容的执行器、scheduler 或 execution context。
HTTP、process、INI、observer 和软总线类型也提供支持执行器的变体。当应用
自行管理事件循环或需要将任务分离到不同上下文时，应使用这些重载。

## Value 与可选转换

`libgs::value` 使用文本存储数据，并提供格式化、字符分类和类型转换工具，此外
还提供宽字符和 UTF 字符类型的变体。

```cpp
#include <libgs/core/value.h>

int main()
{
    libgs::value port = 8080;
    auto parsed_port = port.to_uint();

    libgs::value message("request {} completed", 42);
    auto text = message.to_string();

    return parsed_port && !text.empty() ? 0 : 1;
}
```

转换操作返回 LibGS optional。面对任意输入时，应检查结果或使用 optional 的
默认值工具，不要假定输入一定有效。

## URL

`libgs::url` 是协议无关的层次化 URL 类型，可解析 scheme、host、port、path 和
查询参数，也可解析相对引用。默认值为本地 URL `local:///`；HTTP scheme 的约束
由 HTTP 客户端负责，而不是由 Core 类型强制施加。

```cpp
#include <libgs/core/url.h>

libgs::url source("file:///tmp/report.txt");
libgs::url endpoint("https://example.test/api?page=1");

auto next = libgs::url::resolve(endpoint, "../status");
```

## INI 数据

`libgs::ini` 支持 group/key 访问、迭代、整体数据替换和文件持久化。加载与同步
操作接受 LibGS completion token。

```cpp
#include <libgs/core/ini.h>

int main()
{
    libgs::ini config("./app.ini");
    config.load_or();

    config.write("server/host", "127.0.0.1");
    config.write("server/port", 8080);
    config.sync();

    auto host = config.read("server/host");
    return host ? 0 : 1;
}
```

源文件不存在时，`load_or` 会保留空数据集，而 `load` 会报告文件错误。可以通过
`set_sync_period` 和 `set_sync_on_delete` 配置周期性自动同步和析构时同步。

## 命令行解析

`libgs::cmdline::args_parser` 支持带值的选项组、组合标志、帮助输出、版本输出、
别名和未匹配参数收集。规则可以包含逗号分隔的别名，例如 `-f,--file`。

完整示例请参阅
[`examples/core/args_parser.cpp`](../../examples/core/args_parser.cpp)。

## 算法与标识符

算法聚合头包含：

- UUID v4、v5、v6 和 v7 生成；
- 支持十六进制和 Base64 输出的增量 SHA-1；
- 使用 `*` 和 `?` 的通配符匹配；
- 百分号编码与解码；
- 算术平均值工具。

```cpp
#include <libgs/core/algorithm.h>

auto id = libgs::uuid::generate();
auto match_weight = libgs::wildcard_match("lib*.so*", "libexample.so.1");

libgs::sha1 digest("payload");
auto hex = digest.finalize().hex();
```

SHA-1 用于协议兼容等场景。新的密码或签名等安全敏感设计不应选择 SHA-1。

## 容器与同步

Core 模块包含链式和环形无锁队列、字符串专用容器、自旋互斥量和自旋共享互斥量。
这些是线程级设施。对于需要挂起、且不能阻塞执行器线程的代码，请使用
[协程支持](coroutines.md)中介绍的同步原语。

## 系统设施

`libgs::app` 工具提供可执行文件路径、工作目录操作、绝对路径处理、环境变量、
当前用户和主目录查询。大多数操作返回 `sys_expected<T>`，便于以非异常方式处理
平台错误。

`libgs::library` 封装动态库加载和类型化符号查询。调用从动态库中获取的函数时，
必须保证对应的 library 对象仍然存活。

项目为 Windows 和类 Unix 系统提供了不同的平台实现。应始终检查返回的
`sys_expected<T>`，不要假定各平台行为完全相同。

## 相关示例

- [`examples/core/execution.cpp`](../../examples/core/execution.cpp)
- [`examples/core/value.cpp`](../../examples/core/value.cpp)
- [`examples/core/ini.cpp`](../../examples/core/ini.cpp)
- [`examples/core/algorithms.cpp`](../../examples/core/algorithms.cpp)
- [`examples/core/lock_free_queue.cpp`](../../examples/core/lock_free_queue.cpp)
- [`examples/core/app_paths.cpp`](../../examples/core/app_paths.cpp)
- [`examples/core/args_parser.cpp`](../../examples/core/args_parser.cpp)
- [`examples/core/dynamic_library.cpp`](../../examples/core/dynamic_library.cpp)
