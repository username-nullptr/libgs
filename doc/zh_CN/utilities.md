# 应用工具

语言：[English](../en/utilities.md) | 简体中文

Utilities 模块提供围绕应用事件循环经常需要的可复用服务：日志、持久化设置、
信号、观察者、模块初始化、子进程和发布/订阅状态。链接时使用 `gs.utils`；它
公开依赖 `gs.coro` 和 `gs.core`。

## 头文件索引

| 头文件 | 用途 |
| --- | --- |
| `<libgs/utils/logger.h>` | 命名的控制台 logger 和滚动文件 logger |
| `<libgs/utils/settings.h>` | 命名的 INI settings 实例与变更信号 |
| `<libgs/utils/signal_slot.h>` | 同步、异步和背压 signal/slot 分发 |
| `<libgs/utils/observer.h>` | 在执行器上按 ID 寻址的 observer 回调 |
| `<libgs/utils/modules.h>` | 感知依赖关系的模块注册与初始化 |
| `<libgs/utils/process.h>` | 子进程生命周期和标准流 I/O |
| `<libgs/utils/sbus.h>` | 可扩展软总线、类型化 payload 和 topic 状态缓存 |
| `<libgs/utils.h>` | Logger、settings、modules 和软总线聚合头 |

Process、signal/slot 和 observer API 目前需要直接包含各自的头文件。

## 日志

`libgs::utils::logger` 管理命名 logger 实例。Logger 配置可以控制输出路径、时间戳
模式、换行、控制台和 daily log level，以及 warning、error 和 critical 文件的
滚动限制。

```cpp
#include <libgs/utils/logger.h>

int main()
{
    libgs::utils::logger::config_t config {
        .path = "./logs"
    };

    libgs::utils::logger::instance().set_config(config);

    libgs_utils_log_info("service started on port {}", 8080);
    libgs_utils_clog_warning("network", "retry {}", 3);
}
```

日志宏通过 `std::source_location` 捕获源文件、函数和行号。除非使用
`instance(name, false)`，否则命名实例会在第一次访问时创建。

## 设置

`libgs::utils::settings` 将 `libgs::ini` 封装为命名单例，并提供 `changed` 和
`loaded` 信号。

```cpp
#include <libgs/utils/settings.h>

int main()
{
    auto &settings = libgs::utils::settings::instance();
    settings.load("./app.ini");

    settings.changed.connect(
        [](std::string_view path, const libgs::value &value)
        {
            // 响应设置变更。
        }
    );

    settings
        .set("server/host", "127.0.0.1")
        .set("server/port", 8080)
        .sync();
}
```

可以通过 `get(path)` 或 `{group, key}` pair 读取 optional value。底层 INI 对象
可通过 `ini()` 访问，用于迭代和更高级的持久化配置。

## 信号与槽

`libgs::utils::signal<Signature>` 可以将一个信号连接到多个兼容 callable。Slot
参数可以是 signal 参数的兼容前缀，并可以使用支持的参数转换。

分发模式如下：

| 模式 | 行为 |
| --- | --- |
| `sync` | 直接阻塞调用；signal 实现将其标记为线程不安全 |
| `async` | 通过执行器进行非阻塞分发 |
| `backpressure` | 通过执行器进行线程安全的阻塞分发；在同一个被阻塞执行器上误用可能死锁 |

Signal 支持自由函数、lambda、由 shared pointer 持有的 observer 对象、显式执行器、
协程 slot、断开连接和临时阻塞。

触发操作会持有其内部状态和 slot 快照直至本次分发完成，因此已经返回的 awaitable
不会依赖 signal 对象继续存活。按值传递的参数也由该操作持有。使用 observer 重载时，
实现会在调用前锁定其 weak pointer，并在整个回调（包括协程挂起期间）持有强引用。

Signal 无法管理 slot 自行捕获的外部对象，也无法延长引用、裸指针或 view 所指数据的
生命周期。调用方仍须保证这些非 owning 数据有效；signal 对象的析构也不得与另一个
线程正在进入该对象的成员函数并发发生。

```cpp
#include <libgs/utils/signal_slot.h>

int main()
{
    libgs::utils::signal<void(int)> changed;

    changed.connect([](int value)
    {
        // 同步 slot。
    });

    changed(42);
}
```

应明确选择 async 或 backpressure 分发方式，尤其不要同步等待已经排队到同一个
执行器线程上的任务。

## 观察者

`libgs::utils::observer` 提供按 ID 寻址的回调组。Observer 以共享对象创建，注册
一个或多个带索引的回调签名，并通过其执行器接收静态 `trigger<Index>(id, ...)`
事件。

当生产者知道稳定对象 ID、但不应持有接收方直接指针时，这种模型很有用。
Observer 析构后不会再接收后续分发。

## 感知依赖关系的模块

`libgs::utils::modules` 使用名称注册初始化函数，并按 parent 和 child 依赖关系
排序。Initializer 可以接收命令行参数，并返回 `void` 或 `bool`。

```cpp
#include <libgs/utils/modules.h>

LIBGS_UTILS_MODULE_INIT("storage", []
{
    // 初始化 storage。
});

LIBGS_UTILS_MODULE_INIT(
    "api",
    {.parents = {"storage"}},
    []
    {
        // 在 storage 之后运行。
    }
);
```

所有 translation unit 完成模块注册后，再调用 `modules::do_init(...)`。该操作支持
同步、future、detached 和 callback 形式。初始化失败会报告失败模块、缺失模块和
受依赖关系影响的模块。`modules::sprint()` 返回可打印的注册关系图。

## 进程

`libgs::utils::process` 和 `libgs::utils::wprocess` 使用指定执行器管理子进程。
API 支持：

- start、run、join、detach、terminate、kill 和 cancel；
- 在支持的位置使用同步、超时、callback 和 awaitable 完成方式；
- 标准输入、输出和错误流 I/O；
- 工作目录和环境变量覆盖；
- process state、PID 和 exit code 查询；
- 通过 `set_single` 使用单实例锁文件。

应检查 `sys_expected<T>` 和 error-code 结果来处理平台错误。异步标准流操作结束
之前，必须保证 process 对象仍然存活。
`cancel()` 会结束未完成的等待和标准流操作。`cancel_option::none` 保持子进程
运行且仍可 join；`terminate` 和 `kill` 向子进程发信号，并把回收工作移交给后台
监控对象；`detach` 不向子进程发信号，只解除对象对它的管理。

## 软总线

Sbus 是 **soft bus（软总线）**。它的发布/订阅和缓存 API 不绑定具体传输。
LibGS 默认提供进程内传输 `local_interface`：

- `sbus::publish` 发送 raw、string 或支持的类型化 payload；
- `sbus::local_subscriber` 按 topic 或订阅全部 topic；
- 订阅回调通过 subscriber 的执行器运行；
- 可以按 topic、ID 或整个 subscriber 取消订阅；
- `sbus::local_cache` 保存最新 topic value、发出变更信号，并可以等待变更。

Payload 类型可以通过 `LIBGS_UTILS_SBUS_TYPE` 或 metadata 变体声明稳定 topic。

`basic_subscriber<Interface>`、`cache<Subscriber>` 和 `publish<Interface>` 都可
替换传输层。Interface 需要提供静态 `publish`、按 topic 和全局 `subscribe`，以及
按 topic、订阅 ID 和全部取消的方法；创建 subscriber 时还会调用可选的 `init()`。
例如，DDS 适配器可以在不改变上层 API 的情况下扩展到进程间或分布式通信：

```cpp
namespace sbus = libgs::utils::sbus;

using dds_subscriber = sbus::basic_subscriber<dds_interface>;
using dds_cache = sbus::cache<dds_subscriber>;

dds_subscriber subscriber;
subscriber.subscribe("sensor.state", on_state);
sbus::publish<dds_interface>("sensor.state", state);
```

DDS 只是集成示例，不是库的内置依赖。仓库当前只提供进程内的
`local_interface` 传输。

## Utility 线程池

`libgs::utils::thread_pool()` 返回 Utilities 模块共享的 `asio::thread_pool`。当任务
隔离或关闭顺序比较重要时，应优先使用显式拥有的执行器。

## 相关示例

- [`examples/utils/logger.cpp`](../../examples/utils/logger.cpp)
- [`examples/utils/settings.cpp`](../../examples/utils/settings.cpp)
- [`examples/utils/signal_slot.cpp`](../../examples/utils/signal_slot.cpp)
- [`examples/utils/observer.cpp`](../../examples/utils/observer.cpp)
- [`examples/utils/modules`](../../examples/utils/modules)
- [`examples/utils/process.cpp`](../../examples/utils/process.cpp)
- [`examples/utils/soft_bus_local.cpp`](../../examples/utils/soft_bus_local.cpp)
- [`examples/utils/soft_bus_transport.cpp`](../../examples/utils/soft_bus_transport.cpp)
