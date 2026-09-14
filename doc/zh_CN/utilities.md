# 应用工具

语言：[English](../en/utilities.md) | 简体中文

`gs.utils` 在 `gs.coro` 之上提供应用层服务。

## 公共结构

| 头文件 | 功能 |
| --- | --- |
| `<libgs/utils/logger.h>` | 命名控制台与轮转文件日志 |
| `<libgs/utils/settings.h>` | 命名的 INI 设置与变更信号 |
| `<libgs/utils/signal_slot.h>` | 同步、异步与背压信号投递 |
| `<libgs/utils/observer.h>` | 通过 executor 投递的 ID 寻址回调 |
| `<libgs/utils/modules.h>` | 按依赖顺序初始化应用模块 |
| `<libgs/utils/process.h>` | 子进程生命周期与标准流 I/O |
| `<libgs/utils/sbus.h>` | 可扩展发布/订阅与 Topic 缓存 |

`<libgs/utils.h>` 聚合 logger、settings、modules 与 soft bus。process、
signal/slot、observer 需要直接包含各自头文件。

## 功能

### 日志

`utils::logger` 管理命名实例以及 console、daily、warning、error、critical sink。
配置控制路径、级别、轮转、时间戳与格式。日志宏会捕获 `std::source_location`。
文件 sink 异步工作；需要确认写入完成时调用 `logger::flush()`。

### 设置

`utils::settings` 把 `libgs::ini` 包装成命名实例，并发出 `changed` 与
`loaded` 信号。使用 `get()`/`set()` 读写值，需要直接控制持久化时使用 `ini()`。

### 信号与观察者

`utils::signal<Signature>` 支持三种投递模式：

| 模式 | 行为 |
| --- | --- |
| `sync` | 直接调用 slot 并阻塞调用方 |
| `async` | 把 slot 提交到 executor |
| `backpressure` | 提交到 executor，并阻塞直到投递完成 |

不要从负责执行投递的同一 executor 线程使用 backpressure。Signal 会持有投递
状态和按值参数，但不会延长 view、指针、引用或 slot 捕获对象的生命周期。

`utils::observer` 通过稳定对象 ID 路由带索引的回调签名。Observer 采用共享
所有权，并在析构时注销。

### 模块

`utils::modules` 注册命名初始化器，并按父/子依赖排序。初始化器可以接收命令行
参数，返回 `void` 或 `bool`。`do_init()` 支持同步与异步完成；`sprint()` 输出
依赖图。

### 进程

`utils::process` 与 `utils::wprocess` 支持 start/run、join、detach、terminate、
kill、取消、超时、环境变量/工作目录、单实例锁以及标准输入/输出/错误流。异步
I/O 未完成时必须保持进程对象存活，并检查返回错误以处理平台差异。

### 软总线

软总线把带类型的 publish/subscribe/cache API 与传输实现分离。
`sbus::local_interface`、`local_subscriber` 与 `local_cache` 提供内置进程内
传输。`basic_subscriber<Interface>`、`cache<Subscriber>` 与
`publish<Interface>()` 可让其他传输复用同一 API；仓库不内置分布式传输。

`utils::thread_pool()` 返回 Utilities 模块共享的 Asio 线程池。需要隔离或明确
关闭顺序时，应使用应用自己持有的 executor。

## 示例

- [日志](../../examples/utils/logger.cpp)
- [设置](../../examples/utils/settings.cpp)
- [信号](../../examples/utils/signal_slot.cpp)
- [观察者](../../examples/utils/observer.cpp)
- [模块](../../examples/utils/modules)
- [进程](../../examples/utils/process.cpp)
- [本地软总线](../../examples/utils/soft_bus_local.cpp)
- [自定义软总线传输](../../examples/utils/soft_bus_transport.cpp)
